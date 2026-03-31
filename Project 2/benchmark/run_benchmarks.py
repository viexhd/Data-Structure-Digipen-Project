"""
run_benchmarks.py -- Benchmarking harness for APSC polygon simplification.

Measures:
  - Wall time  : parsed from the program's own "Simplification: N vertices, T ms"
                 line (avoids WSL startup overhead on Windows)
  - Peak RSS   : from /usr/bin/time -v (Linux/WSL) or psutil polling (native)

Output files (written to benchmark/results/):
    time_memory_vs_n.json
    displacement_vs_target.json
    many_holes.json
    narrow_gap.json

Usage (from project root):
    python benchmark/run_benchmarks.py [--exe ./simplify] [--reps 5]
"""

import argparse, json, os, re, subprocess, sys, threading, time
import psutil

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR  = os.path.join(SCRIPT_DIR, "results")
DATASETS_DIR = os.path.join(SCRIPT_DIR, "datasets")

# ---------------------------------------------------------------------------
# Windows/WSL detection helpers
# ---------------------------------------------------------------------------

def is_elf(path: str) -> bool:
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def win_to_wsl(path: str) -> str:
    """C:\\foo\\bar  ->  /mnt/c/foo/bar"""
    p = path.replace("\\", "/")
    if len(p) >= 2 and p[1] == ":":
        drive = p[0].lower()
        rest = p[2:].lstrip("/")
        return f"/mnt/{drive}/{rest}"
    return p


USE_WSL = sys.platform == "win32"  # overridden in main() if exe is native

# ---------------------------------------------------------------------------
# Parse helpers
# ---------------------------------------------------------------------------

def count_vertices(csv_path: str) -> int:
    with open(csv_path) as f:
        return sum(1 for line in f
                   if line.strip() and not line.startswith("ring_id"))


def parse_internal_time_ms(stdout: str) -> float | None:
    """Extract ms from 'Simplification: N vertices, T ms'."""
    m = re.search(r"Simplification:\s*\d+\s*vertices,\s*([\d.eE+\-]+)\s*ms", stdout)
    return float(m.group(1)) if m else None


def parse_displacement(stdout: str) -> float | None:
    m = re.search(r"Total areal displacement:\s*([\d.eE+\-]+)", stdout)
    return float(m.group(1)) if m else None


def parse_peak_rss_kb(time_stderr: str) -> int | None:
    """Parse 'Maximum resident set size (kbytes): N' from /usr/bin/time -v output."""
    m = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", time_stderr)
    return int(m.group(1)) if m else None

# ---------------------------------------------------------------------------
# Core runner
# ---------------------------------------------------------------------------

def run_once(exe: str, csv_path: str, target: int,
             timeout: float = 300.0) -> dict:
    """
    Run simplify once.  Returns:
        internal_ms  - time reported by the program itself (ms)
        peak_rss_kb  - peak RSS from /usr/bin/time -v (Linux/WSL) or psutil
        stdout       - captured stdout
        returncode   - exit code
        error        - error string or None
    """
    if USE_WSL and is_elf(exe):
        # Run via WSL with /usr/bin/time -v for memory measurement
        wsl_exe = win_to_wsl(exe)
        wsl_csv = win_to_wsl(csv_path)
        # stdout = simplify output; stderr = /usr/bin/time -v output
        bash_cmd = f'/usr/bin/time -v "{wsl_exe}" "{wsl_csv}" {target}'
        cmd = ["wsl", "-e", "bash", "-c", bash_cmd]
        try:
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, timeout=timeout
            )
            stdout    = result.stdout
            # simplify prints timing to stderr; /usr/bin/time -v also writes there
            combined_err = result.stderr
            rss_kb    = parse_peak_rss_kb(combined_err)
            int_ms    = parse_internal_time_ms(combined_err)   # stderr!
            return {
                "internal_ms": int_ms,
                "peak_rss_kb": rss_kb,
                "stdout":      stdout,
                "returncode":  result.returncode,
                "error":       None if result.returncode == 0
                               else combined_err.strip()
            }
        except subprocess.TimeoutExpired:
            return {"internal_ms": None, "peak_rss_kb": None,
                    "stdout": "", "returncode": -1,
                    "error": f"Timeout ({timeout}s)"}
        except FileNotFoundError as e:
            return {"internal_ms": None, "peak_rss_kb": None,
                    "stdout": "", "returncode": -1, "error": str(e)}
    else:
        # Native run: psutil polling for memory
        peak_rss_kb = 0
        done_evt    = threading.Event()

        def _monitor(pid: int) -> None:
            nonlocal peak_rss_kb
            try:
                p = psutil.Process(pid)
                while not done_evt.is_set():
                    try:
                        rss = p.memory_info().rss // 1024
                        if rss > peak_rss_kb:
                            peak_rss_kb = rss
                    except psutil.NoSuchProcess:
                        break
                    time.sleep(0.005)
            except psutil.NoSuchProcess:
                pass

        try:
            proc = subprocess.Popen(
                [exe, csv_path, str(target)],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            mon = threading.Thread(target=_monitor, args=(proc.pid,), daemon=True)
            mon.start()
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                stdout, stderr = proc.communicate()
                done_evt.set(); mon.join(1.0)
                return {"internal_ms": None, "peak_rss_kb": peak_rss_kb,
                        "stdout": "", "returncode": -1,
                        "error": f"Timeout ({timeout}s)"}
            finally:
                done_evt.set(); mon.join(1.0)
            int_ms = parse_internal_time_ms(stdout)
            return {
                "internal_ms": int_ms,
                "peak_rss_kb": peak_rss_kb,
                "stdout":      stdout,
                "returncode":  proc.returncode,
                "error":       stderr.strip() if proc.returncode != 0 else None
            }
        except FileNotFoundError as e:
            return {"internal_ms": None, "peak_rss_kb": None,
                    "stdout": "", "returncode": -1, "error": str(e)}


def median_runs(exe: str, csv_path: str, target: int,
                reps: int, timeout: float = 300.0) -> dict:
    """reps runs -> median internal_ms, max peak_rss_kb, last stdout."""
    times, rsss, last_out = [], [], ""
    for i in range(reps):
        r = run_once(exe, csv_path, target, timeout)
        if r["returncode"] != 0:
            print(f"    [WARN] run {i+1}/{reps}: {r['error']}")
            continue
        if r["internal_ms"] is not None:
            times.append(r["internal_ms"])
        if r["peak_rss_kb"] is not None:
            rsss.append(r["peak_rss_kb"])
        last_out = r["stdout"]
    if not times:
        return {"internal_ms": None, "peak_rss_kb": None,
                "stdout": last_out, "error": "all runs failed / no timing found"}
    times.sort()
    return {
        "internal_ms": times[len(times) // 2],
        "peak_rss_kb": max(rsss) if rsss else None,
        "stdout":      last_out,
        "error":       None
    }

# ---------------------------------------------------------------------------
# Experiment helpers
# ---------------------------------------------------------------------------

def _run_exp(exe: str, dataset_subdir: str, label_fn,
             target_fn, reps: int, label_key: str) -> list[dict]:
    d = os.path.join(DATASETS_DIR, dataset_subdir)
    if not os.path.isdir(d):
        print(f"  [ERROR] {d} not found. Run gen_datasets.py first.")
        return []
    csvs = sorted([f for f in os.listdir(d) if f.endswith(".csv")])
    results = []
    for fname in csvs:
        path = os.path.join(d, fname)
        n    = count_vertices(path)
        t    = target_fn(n, fname)
        lbl  = label_fn(fname)
        print(f"  {label_key}={lbl}  n={n:6d}  target={t:5d} ...", end="", flush=True)
        r = median_runs(exe, path, t, reps)
        row = {label_key: lbl, "n": n, "target": t,
               "internal_ms": r["internal_ms"],
               "peak_rss_kb": r["peak_rss_kb"],
               "displacement": parse_displacement(r["stdout"]),
               "error": r.get("error")}
        results.append(row)
        if r["internal_ms"] is not None:
            mem_str = f"  {r['peak_rss_kb']} KiB" if r["peak_rss_kb"] else ""
            print(f"  {r['internal_ms']:.3f} ms{mem_str}")
        else:
            print(f"  FAILED ({row['error']})")
    return results

# ---------------------------------------------------------------------------
# Experiment 1: time & memory vs. n
# ---------------------------------------------------------------------------

def exp_time_memory(exe: str, reps: int) -> list[dict]:
    return _run_exp(
        exe, "large_n",
        label_fn  = lambda f: int(re.search(r"\d+", f).group()),
        target_fn = lambda n, _: max(4, n // 2),
        reps      = reps,
        label_key = "n"
    )

# ---------------------------------------------------------------------------
# Experiment 2: displacement vs. target count
# ---------------------------------------------------------------------------

def exp_displacement_vs_target(exe: str) -> list[dict]:
    d = os.path.join(DATASETS_DIR, "large_n")
    if not os.path.isdir(d):
        print("  [ERROR] datasets/large_n not found.")
        return []
    csvs = sorted(
        [os.path.join(d, f) for f in os.listdir(d) if f.endswith(".csv")],
        key=lambda p: int(re.search(r"\d+", os.path.basename(p)).group())
    )
    # Pick first polygon with n >= 1600
    chosen = next((p for p in csvs if count_vertices(p) >= 1600), csvs[-1])
    n = count_vertices(chosen)
    fractions = [0.90, 0.80, 0.70, 0.60, 0.50, 0.40, 0.30, 0.20, 0.10]
    targets = sorted(set(max(4, int(n * f)) for f in fractions))
    print(f"  Fixed polygon: n={n} ({os.path.basename(chosen)})")
    results = []
    for t in targets:
        print(f"  target={t:5d} ...", end="", flush=True)
        r = run_once(exe, chosen, t)
        disp = parse_displacement(r["stdout"])
        row = {"n": n, "target": t, "fraction": round(t / n, 4),
               "internal_ms": r["internal_ms"],
               "displacement": disp,
               "error": r.get("error")}
        results.append(row)
        if r["returncode"] == 0:
            disp_str = f"{disp:.4e}" if disp is not None else "n/a"
            ms_str   = f"{r['internal_ms']:.3f}" if r["internal_ms"] is not None else "n/a"
            print(f"  disp={disp_str}  {ms_str} ms")
        else:
            print(f"  FAILED ({r.get('error')})")
    return results

# ---------------------------------------------------------------------------
# Experiment 3: many-holes stress
# ---------------------------------------------------------------------------

def exp_many_holes(exe: str, reps: int) -> list[dict]:
    return _run_exp(
        exe, "many_holes",
        label_fn  = lambda f: int(re.search(r"k(\d+)", f).group(1)),
        target_fn = lambda n, _: max(4, n // 2),
        reps      = reps,
        label_key = "k_holes"
    )

# ---------------------------------------------------------------------------
# Experiment 4: narrow-gap stress
# ---------------------------------------------------------------------------

def exp_narrow_gap(exe: str, reps: int) -> list[dict]:
    def gap_label(f: str) -> float:
        m = re.search(r"gap(\d+p\d+)", f)
        return float(m.group(1).replace("p", ".")) if m else 0.0
    return _run_exp(
        exe, "narrow_gap",
        label_fn  = gap_label,
        target_fn = lambda n, _: max(4, n // 2),
        reps      = reps,
        label_key = "gap"
    )

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    global USE_WSL
    ap = argparse.ArgumentParser(description="APSC benchmarking harness")
    ap.add_argument("--exe",
                    default=os.path.join(os.path.dirname(SCRIPT_DIR), "simplify"),
                    help="Path to simplify executable")
    ap.add_argument("--reps", type=int, default=5,
                    help="Repetitions per data point (default: 5)")
    ap.add_argument("--skip", nargs="*", default=[],
                    choices=["time_mem", "displacement", "holes", "gap"])
    args = ap.parse_args()

    exe = args.exe
    if not os.path.isfile(exe):
        for cand in [exe, exe + ".exe", exe + ".out"]:
            if os.path.isfile(cand):
                exe = cand; break
        else:
            sys.exit(f"[ERROR] Executable not found: {args.exe}  (build with `make`)")

    USE_WSL = sys.platform == "win32" and is_elf(exe)
    os.makedirs(RESULTS_DIR, exist_ok=True)

    print(f"Executable : {exe}")
    print(f"WSL mode   : {USE_WSL}")
    print(f"Repetitions: {args.reps}")
    print(f"Datasets   : {DATASETS_DIR}")
    print(f"Results    : {RESULTS_DIR}")
    print()

    def save(data, name: str) -> None:
        path = os.path.join(RESULTS_DIR, name)
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
        print(f"  -> saved {path}\n")

    if "time_mem" not in args.skip:
        print("=" * 60)
        print("Experiment 1: Time & Memory vs. n")
        print("=" * 60)
        save(exp_time_memory(exe, args.reps), "time_memory_vs_n.json")

    if "displacement" not in args.skip:
        print("=" * 60)
        print("Experiment 2: Areal displacement vs. target")
        print("=" * 60)
        save(exp_displacement_vs_target(exe), "displacement_vs_target.json")

    if "holes" not in args.skip:
        print("=" * 60)
        print("Experiment 3: Many-holes stress")
        print("=" * 60)
        save(exp_many_holes(exe, args.reps), "many_holes.json")

    if "gap" not in args.skip:
        print("=" * 60)
        print("Experiment 4: Narrow-gap stress")
        print("=" * 60)
        save(exp_narrow_gap(exe, args.reps), "narrow_gap.json")

    print("All experiments complete.  Run plot_results.py to generate figures.")


if __name__ == "__main__":
    main()
