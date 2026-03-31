"""
plot_results.py -- Generate the 3 required benchmark plots + curve fitting.

Reads JSON results produced by run_benchmarks.py and writes:
    figures/fig1_time_vs_n.pdf
    figures/fig2_memory_vs_n.pdf
    figures/fig3_displacement_vs_target.pdf
    figures/fig4_stress_tests.pdf     (bonus: holes & narrow-gap)
    figures/eval_numbers.json         (best-fit R^2 values)

Each timing/memory figure includes scatter data + four fitted scaling curves
with R^2 annotations; the best-fit curve is highlighted.

Usage:
    python benchmark/plot_results.py [--results <dir>] [--outdir <dir>]
"""

import argparse, json, os, sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from scipy.optimize import curve_fit

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, "results")
FIGURES_DIR = os.path.join(SCRIPT_DIR, "figures")

# ---------------------------------------------------------------------------
# Matplotlib style
# ---------------------------------------------------------------------------

plt.rcParams.update({
    "figure.dpi": 150,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.alpha": 0.35,
    "font.size": 10,
    "axes.labelsize": 11,
    "axes.titlesize": 12,
    "legend.fontsize": 9,
    "lines.markersize": 6,
})

COLORS = {
    "data":        "#2c7bb6",
    "O(n)":        "#d7191c",
    "O(n log n)":  "#f58a1f",
    "O(n sqrt n)": "#1a9641",
    "O(n^2)":      "#984ea3",
}

# ---------------------------------------------------------------------------
# Scaling models  (a * f(x) + b)
# ---------------------------------------------------------------------------

def f_n(x, a, b):        return a * x + b
def f_nlogn(x, a, b):    return a * x * np.log(x) + b
def f_nsqrtn(x, a, b):   return a * x * np.sqrt(x) + b
def f_n2(x, a, b):       return a * x ** 2 + b

MODELS = {
    "O(n)":        f_n,
    "O(n log n)":  f_nlogn,
    "O(n sqrt n)": f_nsqrtn,
    "O(n^2)":      f_n2,
}

LABELS = {
    "O(n)":        r"$O(n)$",
    "O(n log n)":  r"$O(n \log n)$",
    "O(n sqrt n)": r"$O(n\sqrt{n})$",
    "O(n^2)":      r"$O(n^2)$",
}


def fit_all(ns: np.ndarray, ys: np.ndarray) -> dict:
    valid = np.isfinite(ys) & (ys > 0) & (ns > 1)
    ns_, ys_ = ns[valid], ys[valid]
    result = {}
    for name, func in MODELS.items():
        try:
            p0 = [ys_.max() / ns_.max(), 0.0]
            popt, _ = curve_fit(func, ns_, ys_, p0=p0, maxfev=20000)
            y_hat = func(ns_, *popt)
            ss_res = float(np.sum((ys_ - y_hat) ** 2))
            ss_tot = float(np.sum((ys_ - ys_.mean()) ** 2))
            r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else 0.0
            result[name] = {"popt": popt.tolist(), "r2": r2}
        except Exception:
            result[name] = {"popt": None, "r2": -1e9}
    return result


def best_fit(fits: dict) -> str:
    return max(fits, key=lambda k: fits[k]["r2"])


# ---------------------------------------------------------------------------
# Generic scaling plot
# ---------------------------------------------------------------------------

def plot_scaling(ax, ns: np.ndarray, ys: np.ndarray,
                 ylabel: str, title: str, unit_label: str = "") -> dict:
    fits = fit_all(ns, ys)
    best = best_fit(fits)
    ns_dense = np.linspace(ns.min(), ns.max(), 500)

    for name, func in MODELS.items():
        info = fits[name]
        if info["popt"] is None:
            continue
        y_dense = func(ns_dense, *info["popt"])
        is_best = (name == best)
        color = "#000000" if is_best else COLORS[name]
        lw    = 2.2 if is_best else 1.0
        ls    = "-" if is_best else "--"
        alpha = 1.0 if is_best else 0.55
        r2    = info["r2"]
        lbl   = f"{LABELS[name]}  $R^2={r2:.4f}$"
        if is_best:
            lbl += "  [best]"
        ax.plot(ns_dense, y_dense, color=color, lw=lw, ls=ls, alpha=alpha, label=lbl)

    ax.scatter(ns, ys, color=COLORS["data"], zorder=5,
               label="measured", edgecolors="white", linewidths=0.5)
    ax.set_xlabel("Input vertex count  $n$")
    ax.set_ylabel(f"{ylabel}" + (f"  [{unit_label}]" if unit_label else ""))
    ax.set_title(title)
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.legend(loc="upper left", framealpha=0.9)
    return fits


# ---------------------------------------------------------------------------
# Figure 1: Time vs. n
# ---------------------------------------------------------------------------

def fig_time_vs_n(data: list, outdir: str) -> dict:
    rows = [d for d in data if d.get("internal_ms") is not None]
    if not rows:
        print("  [WARN] No valid time data.")
        return {}
    ns = np.array([r["n"] for r in rows], dtype=float)
    ts = np.array([r["internal_ms"] for r in rows])

    fig, ax = plt.subplots(figsize=(7, 4.5))
    fits = plot_scaling(ax, ns, ts, "Wall-clock time",
                        "APSC simplification time vs. input size", "ms")
    fig.tight_layout()
    for ext in (".pdf", ".png"):
        p = os.path.join(outdir, f"fig1_time_vs_n{ext}")
        fig.savefig(p, dpi=150)
        print(f"  -> {p}")
    plt.close(fig)
    return fits


# ---------------------------------------------------------------------------
# Figure 2: Memory vs. n
# ---------------------------------------------------------------------------

def fig_memory_vs_n(data: list, outdir: str) -> dict:
    rows = [d for d in data if d.get("peak_rss_kb") is not None]
    if not rows:
        print("  [WARN] No valid memory data.")
        return {}
    ns  = np.array([r["n"] for r in rows], dtype=float)
    mem = np.array([r["peak_rss_kb"] / 1024.0 for r in rows])

    fig, ax = plt.subplots(figsize=(7, 4.5))
    fits = plot_scaling(ax, ns, mem, "Peak RSS",
                        "APSC peak memory usage vs. input size", "MiB")
    fig.tight_layout()
    for ext in (".pdf", ".png"):
        p = os.path.join(outdir, f"fig2_memory_vs_n{ext}")
        fig.savefig(p, dpi=150)
        print(f"  -> {p}")
    plt.close(fig)
    return fits


# ---------------------------------------------------------------------------
# Figure 3: Areal displacement vs. target count
# ---------------------------------------------------------------------------

def fig_displacement_vs_target(data: list, outdir: str) -> None:
    rows = [d for d in data
            if d.get("displacement") is not None and d.get("target") is not None]
    if not rows:
        print("  [WARN] No valid displacement data.")
        return
    rows.sort(key=lambda d: d["target"])
    targets = np.array([d["target"] for d in rows], dtype=float)
    disps   = np.array([d["displacement"] for d in rows])
    n_orig  = float(rows[0].get("n", targets.max()))

    fracs_removed = (n_orig - targets) / n_orig * 100.0

    fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))

    # Left: absolute target count
    ax = axes[0]
    ax.plot(targets, disps, "o-", color=COLORS["data"],
            markeredgecolor="white", markeredgewidth=0.5)
    ax.set_xlabel("Target vertex count")
    ax.set_ylabel("Total areal displacement")
    ax.set_title(f"Areal displacement vs. target count  (n={int(n_orig):,})")
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.invert_xaxis()

    # Right: percent removed
    ax2 = axes[1]
    ax2.plot(fracs_removed, disps, "s-", color="#e66101",
             markeredgecolor="white", markeredgewidth=0.5)
    ax2.set_xlabel("Vertices removed  [%]")
    ax2.set_ylabel("Total areal displacement")
    ax2.set_title("Areal displacement vs. simplification ratio")

    # Annotate knee using distance-to-ideal-corner heuristic
    if len(disps) >= 3:
        x_rng = fracs_removed.max() - fracs_removed.min()
        y_rng = disps.max() - disps.min()
        if x_rng > 0 and y_rng > 0:
            xn = (fracs_removed - fracs_removed.min()) / x_rng
            yn = (disps - disps.min()) / y_rng
            dist = np.sqrt((xn - 1.0) ** 2 + (yn - 0.0) ** 2)
            knee = int(np.argmin(dist))
            ax2.annotate(f"Knee\n({fracs_removed[knee]:.0f}% removed)",
                         xy=(fracs_removed[knee], disps[knee]),
                         xytext=(fracs_removed[knee] + 5, disps[knee] * 1.3),
                         arrowprops=dict(arrowstyle="->", color="black"),
                         fontsize=8)

    fig.tight_layout()
    for ext in (".pdf", ".png"):
        p = os.path.join(outdir, f"fig3_displacement_vs_target{ext}")
        fig.savefig(p, dpi=150)
        print(f"  -> {p}")
    plt.close(fig)


# ---------------------------------------------------------------------------
# Figure 4 (bonus): Stress tests
# ---------------------------------------------------------------------------

def fig_stress(holes_data: list, gap_data: list, outdir: str) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))

    if holes_data:
        holes_data = sorted(holes_data, key=lambda d: d.get("k_holes", 0))
        ks  = [d["k_holes"] for d in holes_data if d.get("internal_ms")]
        tms = [d["internal_ms"] for d in holes_data if d.get("internal_ms")]
        if ks:
            axes[0].plot(ks, tms, "o-", color=COLORS["data"],
                         markeredgecolor="white")
            axes[0].set_xlabel("Number of holes  $k$")
            axes[0].set_ylabel("Time  [ms]")
            axes[0].set_title("Simplification time vs. number of holes")
            axes[0].xaxis.set_major_locator(mticker.MaxNLocator(integer=True))

    if gap_data:
        gd = sorted([d for d in gap_data if d.get("gap") and d.get("internal_ms")],
                    key=lambda d: d["gap"])
        if gd:
            gaps = [d["gap"] for d in gd]
            tms2 = [d["internal_ms"] for d in gd]
            axes[1].plot(gaps, tms2, "s-", color="#e66101",
                         markeredgecolor="white")
            axes[1].set_xlabel("Gap between exterior ring and hole")
            axes[1].set_ylabel("Time  [ms]")
            axes[1].set_title("Simplification time vs. gap width")
            axes[1].invert_xaxis()

    fig.tight_layout()
    for ext in (".pdf", ".png"):
        p = os.path.join(outdir, f"fig4_stress_tests{ext}")
        fig.savefig(p, dpi=150)
        print(f"  -> {p}")
    plt.close(fig)


# ---------------------------------------------------------------------------
# Summary table + eval_numbers.json
# ---------------------------------------------------------------------------

def write_summary(time_fits: dict, mem_fits: dict, outdir: str) -> dict:
    summary = {}
    for label, fits in [("time", time_fits), ("memory", mem_fits)]:
        if not fits:
            continue
        best = max(fits, key=lambda k: fits[k]["r2"])
        summary[label] = {
            "best_model": best,
            "r2":         fits[best]["r2"],
            "all": {k: {"r2": v["r2"],
                        "a": v["popt"][0] if v["popt"] else None}
                    for k, v in fits.items()}
        }
        print(f"\n  {label.upper()} scaling fit results:")
        for m, info in sorted(fits.items(), key=lambda x: -x[1]["r2"]):
            star = " <- best" if m == best else ""
            print(f"    {m:16s}  R^2={info['r2']:.6f}{star}")

    path = os.path.join(outdir, "eval_numbers.json")
    with open(path, "w") as f:
        json.dump(summary, f, indent=2)
    print(f"\n  -> {path}")
    return summary


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def load_json(path: str) -> list:
    if not os.path.isfile(path):
        print(f"  [SKIP] {path} not found")
        return []
    with open(path) as f:
        return json.load(f)


def main() -> None:
    ap = argparse.ArgumentParser(description="Plot APSC benchmark results")
    ap.add_argument("--results", default=RESULTS_DIR)
    ap.add_argument("--outdir",  default=FIGURES_DIR)
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)

    tm_data    = load_json(os.path.join(args.results, "time_memory_vs_n.json"))
    disp_data  = load_json(os.path.join(args.results, "displacement_vs_target.json"))
    holes_data = load_json(os.path.join(args.results, "many_holes.json"))
    gap_data   = load_json(os.path.join(args.results, "narrow_gap.json"))

    print("[Figure 1] Time vs. n")
    time_fits = fig_time_vs_n(tm_data, args.outdir)

    print("[Figure 2] Memory vs. n")
    mem_fits = fig_memory_vs_n(tm_data, args.outdir)

    print("[Figure 3] Displacement vs. target count")
    fig_displacement_vs_target(disp_data, args.outdir)

    print("[Figure 4] Stress tests")
    fig_stress(holes_data, gap_data, args.outdir)

    print("\n[Summary] Fit results")
    write_summary(time_fits, mem_fits, args.outdir)

    print("\nAll figures saved to:", args.outdir)


if __name__ == "__main__":
    main()
