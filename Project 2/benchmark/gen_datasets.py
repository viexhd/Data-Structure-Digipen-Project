"""
gen_datasets.py — Generate test datasets for APSC polygon simplification benchmarking.

Usage:
    python gen_datasets.py [--outdir <path>]

Datasets produced
-----------------
  datasets/large_n/     – exterior-only polygons, n ∈ {50,100,200,400,800,1600,3200,6400,12800}
  datasets/many_holes/  – exterior (~300 v) + k holes, k ∈ {1,2,5,10,20}
  datasets/narrow_gap/  – exterior + 1 concentric hole, gap ∈ {5,2,1,0.5,0.1}
  datasets/degenerate/  – near-degenerate shapes: comb, zigzag, elongated, nearly-collinear

CSV format:  ring_id,vertex_id,x,y
Orientation: ring 0 = exterior CCW (+area), rings 1+ = holes CW (−area)
"""

import argparse, math, os
import numpy as np


# ---------------------------------------------------------------------------
# Writers
# ---------------------------------------------------------------------------

def write_polygon(path: str, rings: list[list[tuple[float, float]]]) -> None:
    """Write polygon rings to CSV.  rings[0]=exterior(CCW), rings[1+]=holes(CW)."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write("ring_id,vertex_id,x,y\n")
        for ring_id, verts in enumerate(rings):
            for vid, (x, y) in enumerate(verts):
                f.write(f"{ring_id},{vid},{x:.6f},{y:.6f}\n")


def signed_area(verts: list[tuple[float, float]]) -> float:
    """Shoelace formula – positive = CCW."""
    n = len(verts)
    a = 0.0
    for i in range(n):
        x1, y1 = verts[i]
        x2, y2 = verts[(i + 1) % n]
        a += x1 * y2 - x2 * y1
    return a / 2.0


def ensure_ccw(verts: list[tuple[float, float]]) -> list[tuple[float, float]]:
    return verts if signed_area(verts) > 0 else verts[::-1]


def ensure_cw(verts: list[tuple[float, float]]) -> list[tuple[float, float]]:
    return verts if signed_area(verts) < 0 else verts[::-1]


# ---------------------------------------------------------------------------
# Exterior ring generators
# ---------------------------------------------------------------------------

def smooth_polygon(n: int, cx: float = 0, cy: float = 0,
                   base_r: float = 100.0, roughness: float = 0.15,
                   seed: int = 42) -> list[tuple[float, float]]:
    """
    Smooth, star-like exterior ring with n vertices.
    Radius = base_r * (1 + sum of low-frequency Fourier terms).
    Guaranteed no self-intersections for roughness <= 0.3.
    """
    rng = np.random.default_rng(seed)
    # Use 3 random Fourier modes (low frequency → no self-intersections)
    n_modes = 3
    amps = rng.uniform(0, roughness / n_modes, n_modes)
    phases = rng.uniform(0, 2 * math.pi, n_modes)
    freqs = rng.integers(2, 5, n_modes)

    angles = np.linspace(0, 2 * math.pi, n, endpoint=False)
    radii = base_r * (1.0 + sum(
        amps[k] * np.cos(freqs[k] * angles + phases[k]) for k in range(n_modes)
    ))
    verts = [(cx + r * math.cos(a), cy + r * math.sin(a))
             for r, a in zip(radii, angles)]
    return ensure_ccw(verts)


def circle(n: int, cx: float = 0, cy: float = 0,
           r: float = 100.0) -> list[tuple[float, float]]:
    angles = np.linspace(0, 2 * math.pi, n, endpoint=False)
    verts = [(cx + r * math.cos(a), cy + r * math.sin(a)) for a in angles]
    return ensure_ccw(verts)


def circle_cw(n: int, cx: float = 0, cy: float = 0,
              r: float = 50.0) -> list[tuple[float, float]]:
    """Clockwise circle for use as hole."""
    return ensure_cw(circle(n, cx, cy, r))


# ---------------------------------------------------------------------------
# Special / near-degenerate generators
# ---------------------------------------------------------------------------

def comb_polygon(n_teeth: int, tooth_w: float = 2.0,
                 tooth_h: float = 20.0, base_h: float = 10.0,
                 gap: float = 1.0) -> list[tuple[float, float]]:
    """
    Comb shape: flat base with n_teeth narrow upward teeth.
    Creates many nearly-collinear sequences and near-parallel edges.
    Very challenging for intersection checks (tight tooth gaps).
    """
    pitch = tooth_w + gap
    total_w = n_teeth * pitch - gap + tooth_w  # small margin
    verts: list[tuple[float, float]] = []
    # Bottom-left → bottom-right (base bottom)
    verts.append((0.0, 0.0))
    verts.append((total_w, 0.0))
    # Right side up
    verts.append((total_w, base_h))
    # Teeth from right to left
    for i in range(n_teeth - 1, -1, -1):
        lx = i * pitch
        rx = lx + tooth_w
        # Up the right side of tooth
        verts.append((rx, base_h))
        verts.append((rx, base_h + tooth_h))
        # Down the left side of tooth
        verts.append((lx + tooth_w, base_h + tooth_h))
        verts.append((lx, base_h + tooth_h))
        verts.append((lx, base_h))
    # Close to bottom-left
    verts.append((0.0, base_h))
    return ensure_ccw(verts)


def zigzag_polygon(n_zags: int, width: float = 5.0,
                   amplitude: float = 30.0, thickness: float = 2.0
                   ) -> list[tuple[float, float]]:
    """
    Very thin horizontal zigzag band.
    Top and bottom traces are almost parallel → near-degenerate areal displacements.
    """
    step = width
    top: list[tuple[float, float]] = []
    bot: list[tuple[float, float]] = []
    for i in range(n_zags + 1):
        x = i * step
        y_top = amplitude * (1 - (-1) ** i) / 2  # 0 or amplitude
        top.append((x, y_top + thickness))
        bot.append((x, y_top))
    # Build closed polygon (top trace, then reversed bottom)
    verts = top + list(reversed(bot))
    return ensure_ccw(verts)


def elongated_polygon(n: int, aspect: float = 50.0,
                      base_r: float = 2.0) -> list[tuple[float, float]]:
    """
    Highly elongated ellipse (aspect ratio ~50:1).
    Nearly all vertices along the long sides are nearly collinear.
    Areal displacement per collapse is tiny → many iterations required.
    """
    angles = np.linspace(0, 2 * math.pi, n, endpoint=False)
    verts = [(aspect * base_r * math.cos(a), base_r * math.sin(a)) for a in angles]
    return ensure_ccw(verts)


def nearly_collinear_polygon(n: int, length: float = 200.0,
                              width: float = 10.0,
                              jitter: float = 0.5,
                              seed: int = 7) -> list[tuple[float, float]]:
    """
    Rectangle with n/2 points along each long edge, slightly perturbed.
    Produces near-zero displacements; exercises the floating-point robustness.
    """
    rng = np.random.default_rng(seed)
    half = n // 2
    xs = np.linspace(0, length, half)
    top = [(x, width + rng.uniform(-jitter, jitter)) for x in xs]
    bot = [(x, rng.uniform(-jitter, jitter)) for x in reversed(xs)]
    return ensure_ccw(top + bot)


# ---------------------------------------------------------------------------
# Dataset generators
# ---------------------------------------------------------------------------

def gen_large_n(outdir: str) -> None:
    """Exterior-only smooth polygons, n ∈ log-spaced range."""
    ns = [50, 100, 200, 400, 800, 1600, 3200, 6400, 12800]
    d = os.path.join(outdir, "large_n")
    for n in ns:
        verts = smooth_polygon(n, seed=n)
        fname = os.path.join(d, f"n{n:06d}.csv")
        write_polygon(fname, [verts])
        print(f"  large_n/n{n:06d}.csv  ({n} vertices)")


def gen_many_holes(outdir: str) -> None:
    """Fixed exterior + varying number of small circular holes."""
    n_ext = 300
    ext = smooth_polygon(n_ext, base_r=100.0, seed=0)
    d = os.path.join(outdir, "many_holes")

    ks = [1, 2, 5, 10, 20]
    for k in ks:
        rings = [ext]
        rng = np.random.default_rng(k)
        placed: list[tuple[float, float, float]] = []  # (cx, cy, r)
        attempts = 0
        for _ in range(k):
            r_hole = 8.0
            n_hole = max(8, 24)
            for _ in range(500):
                cx = rng.uniform(-60, 60)
                cy = rng.uniform(-60, 60)
                # Check separation from other holes
                ok = True
                for (ox, oy, or_) in placed:
                    dist = math.hypot(cx - ox, cy - oy)
                    if dist < r_hole + or_ + 5.0:
                        ok = False
                        break
                if ok:
                    placed.append((cx, cy, r_hole))
                    rings.append(circle_cw(n_hole, cx, cy, r_hole))
                    break
        fname = os.path.join(d, f"k{k:03d}_holes.csv")
        total = sum(len(r) for r in rings)
        write_polygon(fname, rings)
        print(f"  many_holes/k{k:03d}_holes.csv  ({len(rings)-1} holes, {total} total vertices)")


def gen_narrow_gap(outdir: str) -> None:
    """
    Exterior circle (r=50) + concentric circular hole, varying the gap.
    Narrow gaps stress the spatial index and cross-ring intersection checks.
    """
    d = os.path.join(outdir, "narrow_gap")
    gaps = [5.0, 2.0, 1.0, 0.5, 0.1]
    r_ext = 50.0
    n_ext = 120
    n_hole = 80
    ext = circle(n_ext, r=r_ext)
    for gap in gaps:
        r_hole = r_ext - gap
        if r_hole < 5.0:
            continue
        hole = circle_cw(n_hole, r=r_hole)
        rings = [ext, hole]
        total = n_ext + n_hole
        tag = f"{gap:.2f}".replace(".", "p")
        fname = os.path.join(d, f"gap{tag}.csv")
        write_polygon(fname, rings)
        print(f"  narrow_gap/gap{tag}.csv  (gap={gap}, {total} vertices)")


def gen_degenerate(outdir: str) -> None:
    """Near-degenerate stress cases."""
    d = os.path.join(outdir, "degenerate")
    cases = {
        "comb_10":         comb_polygon(10),
        "comb_20":         comb_polygon(20),
        "comb_40":         comb_polygon(40),
        "zigzag_50":       zigzag_polygon(50),
        "zigzag_100":      zigzag_polygon(100),
        "elongated_200":   elongated_polygon(200),
        "elongated_1000":  elongated_polygon(1000),
        "nearly_col_200":  nearly_collinear_polygon(200),
        "nearly_col_1000": nearly_collinear_polygon(1000),
    }
    for name, verts in cases.items():
        fname = os.path.join(d, f"{name}.csv")
        write_polygon(fname, [verts])
        print(f"  degenerate/{name}.csv  ({len(verts)} vertices)")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(description="Generate APSC benchmark datasets")
    ap.add_argument("--outdir", default=os.path.join(os.path.dirname(__file__), "datasets"),
                    help="Output root directory (default: ./datasets)")
    args = ap.parse_args()

    print(f"Writing datasets to: {args.outdir}\n")
    print("[1/4] Large-n polygons …")
    gen_large_n(args.outdir)
    print("\n[2/4] Many-holes polygons …")
    gen_many_holes(args.outdir)
    print("\n[3/4] Narrow-gap polygons …")
    gen_narrow_gap(args.outdir)
    print("\n[4/4] Near-degenerate cases …")
    gen_degenerate(args.outdir)
    print("\nDone.")


if __name__ == "__main__":
    main()
