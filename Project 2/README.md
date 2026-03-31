# Project 2 — Area- and Topology-Preserving Polygon Simplification

CSD2183 Data Structures | DigiPen Institute of Technology

## Build

Requires g++ with C++17 support (macOS, Linux, or WSL on Windows).

```bash
make
```

This produces the `simplify` executable in the repository root.

To clean build artifacts:

```bash
make clean
```

## Usage

```bash
./simplify <input_file.csv> <target_vertices>
```

- `<input_file.csv>` — polygon in CSV format (`ring_id,vertex_id,x,y`)
- `<target_vertices>` — desired maximum total vertex count across all rings

### Example

```bash
./simplify tests/example.csv 8
```

## Input Format

```
ring_id,vertex_id,x,y
0,0,-0.5,-1.0
0,1,1.5,-1.0
...
```

- Ring `0` is the exterior ring (counterclockwise)
- Rings `1`, `2`, ... are interior rings / holes (clockwise)

## Output Format

```
ring_id,vertex_id,x,y
0,0,...
...
Total signed area in input: 3.210000e+00
Total signed area in output: 3.210000e+00
Total areal displacement: 1.600000e-02
```

## Dependencies

No third-party libraries are required for the base implementation.

Optional: [CGAL](https://www.cgal.org/) or [Boost.Geometry](https://www.boost.org/doc/libs/release/libs/geometry/) can be used for the spatial index. If added, install instructions will be listed here.

## Test Results

| Test case | Input vertices | Holes | Target | Area preserved | Topology valid |
|---|---|---|---|---|---|
| input_rectangle_with_two_holes.csv | 12 | 2 | 7 | Yes | Yes |
| input_cushion_with_hexagonal_hole.csv | 22 | 1 | 13 | Yes | Yes |
| input_blob_with_two_holes.csv | 36 | 2 | 17 | Yes | Yes |
| input_wavy_with_three_holes.csv | 43 | 3 | 21 | Yes | Yes |
| input_lake_with_two_islands.csv | 81 | 2 | 17 | Yes | Yes |
| input_original_01.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_02.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_03.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_04.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_05.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_06.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_07.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_08.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_09.csv | ~200 | 0 | 99 | Yes | Yes |
| input_original_10.csv | ~200 | 0 | 99 | Yes | Yes |

All 15 test cases pass with matching expected output (area preserved, topology valid).

## Algorithm

Implements the **Area-Preserving Segment Collapse (APSC)** algorithm from:

> Kronenfeld, B. J., Stanislawski, L. V., Buttenfield, B. P., & Brockmeyer, T. (2020).
> Simplification of polylines by segment collapse: minimizing areal displacement while preserving area.
> *International Journal of Cartography*, 6(1), 22–46.

### Key Data Structures

- **Doubly-linked circular list** per ring — O(1) vertex removal and insertion
- **Min-heap priority queue** — always collapse the lowest-displacement candidate next
- **Lazy deletion** — stale candidates (whose vertices were already removed) are discarded on pop
- **Uniform grid spatial index** — grid-based spatial partitioning for O(sqrt(n)) average-case intersection checks, replacing the naive O(n) full-ring scan

### Performance

The spatial grid index divides the polygon's bounding box into sqrt(n) x sqrt(n) cells. Each intersection query only checks segments in overlapping cells, reducing per-collapse cost from O(n) to O(sqrt(n)) average-case.

Timing and peak memory are printed to stderr (e.g., `Simplification: 5000 vertices, 770.5 ms, peak RSS: 16740 KB`).

To benchmark without the spatial grid (naive O(n) scan), build with:
```bash
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -DNDEBUG -DUSE_NAIVE"
```

#### Benchmark: Grid vs Naive O(n) Scan

All benchmarks run on WSL2 (Ubuntu 24.04), single-threaded, compiled with `-O2 -DNDEBUG`. Target = input/2 vertices. Median of 5 runs (3 runs for 50k+).

| Input | Vertices | Grid time (ms) | Grid mem (MB) | Naive time (ms) | Naive mem (MB) | Speedup |
|---|---|---|---|---|---|---|
| bench_1k.csv | 1,000 | 5.0 | 3.8 | 48.7 | 3.8 | 9.8x |
| bench_5k.csv | 5,000 | 96.8 | 7.6 | 1,258 | 4.6 | 13.0x |
| bench_10k.csv | 10,000 | 424 | 16.7 | 5,062 | 5.6 | 11.9x |
| bench_10k_5holes.csv | 10,000 (5 holes) | 414 | 16.7 | 5,014 | 5.6 | 12.1x |
| bench_50k.csv | 50,000 | 33,842 | 237 | 153,674 | 16.9 | 4.5x |
| bench_100k.csv | 100,000 | 276,046 | 840 | est. ~614,000 | est. ~34 | ~2.2x |

#### Peak Memory vs Input Size

| Vertices | Grid peak RSS (MB) | Naive peak RSS (MB) |
|---|---|---|
| 1,000 | 3.8 | 3.8 |
| 5,000 | 7.6 | 4.6 |
| 10,000 | 16.7 | 5.6 |
| 50,000 | 237 | 16.9 |
| 100,000 | 840 | ~34 |

#### Scaling Analysis

**Naive scan:** The O(n) per-collapse intersection check produces O(n^2) total time (n/2 collapses x O(n) each). The 10x input increase from 1k to 10k yields ~104x time increase (48.7 -> 5062), consistent with O(n^2). Fitting: T_naive ~ 4.9e-8 * n^2.

**Grid-accelerated:** The grid reduces per-collapse cost to O(sqrt(n)) average-case, yielding O(n * sqrt(n)) = O(n^1.5) total. The grid uses epoch-based deduplication and early-exit callbacks, avoiding per-query heap allocations. At 50k vertices the grid is 4.5x faster than naive; at 100k it is ~2.2x faster. The speedup decreases at larger scales due to memory overhead and cache pressure from the grid's cell arrays.

**Memory:** The grid uses O(n) additional memory for cell arrays (sqrt(n) x sqrt(n) cells). At small inputs (1k-10k) the overhead is modest (3.8-16.7 MB). At 50k+ the grid's memory dominates (237 MB at 50k, 840 MB at 100k) due to cell vector overhead and priority queue growth from lazy deletion. The naive version stays under 34 MB at all sizes.

## Benchmarking

The `benchmark/` directory contains a self-contained benchmarking suite that generates synthetic test datasets, measures wall time and peak RSS, produces the three required plots, and fits scaling curves.

### Prerequisites

```bash
pip install numpy matplotlib scipy psutil
```

### Workflow

```bash
# 1. Generate synthetic test datasets
python benchmark/gen_datasets.py

# 2. Run all experiments (5 reps each, uses WSL automatically on Windows)
python benchmark/run_benchmarks.py --reps 5

# 3. Produce figures and fit scaling curves
python benchmark/plot_results.py
```

> **Windows note:** The binary must be compiled inside WSL before running the harness.
> If the binary was replaced (e.g. after `git pull`), rebuild first:
> ```bash
> wsl -e bash -c "cd '/mnt/c/<path-to-project>' && make clean && make"
> ```

Results are written to `benchmark/results/` (JSON) and `benchmark/figures/` (PDF + PNG).

### Synthetic Datasets

| Category | Description | n range |
|----------|-------------|---------|
| `large_n/` | Smooth exterior-only polygon (Fourier-perturbed circle) | 50 – 12 800 |
| `many_holes/` | Fixed exterior + k = 1, 2, 5, 10, 20 interior holes | 324 – 780 total |
| `narrow_gap/` | Concentric rings, gap ∈ {5, 2, 1, 0.5, 0.1} | 200 |
| `degenerate/` | Comb, zigzag, elongated (50:1 aspect), nearly-collinear | 54 – 2 000 |

### Measured Scaling (grid-accelerated, WSL2 Ubuntu 22.04, GCC 11.4, -O2, target = n/2)

Time is the median internal wall time over 5 runs; memory is peak RSS from `/usr/bin/time -v`.

| n | Time (ms) | Peak RSS (KiB) |
|---|-----------|----------------|
| 50 | 0.044 | 3 584 |
| 100 | 0.081 | 3 584 |
| 200 | 0.153 | 3 840 |
| 400 | 0.297 | 3 840 |
| 800 | 0.670 | 3 840 |
| 1 600 | 1.426 | 4 096 |
| 3 200 | 3.345 | 4 608 |
| 6 400 | 8.020 | 5 120 |
| 12 800 | 21.402 | 7 976 |

Curve-fit R² (scipy `curve_fit` over the above data):

| Model | Time R² | Memory R² |
|-------|---------|-----------|
| O(n sqrt(n)) | **0.9989** ← best | **0.9864** ← best |
| O(n log n) | 0.9911 | 0.9814 |
| O(n) | 0.9816 | 0.9742 |
| O(n^2) | 0.9812 | 0.9701 |

Both time and memory fit **O(n sqrt(n))** best, consistent with O(n) collapses × O(sqrt(n)) spatial-grid queries each.

### Areal Displacement vs. Target Count

For a fixed 1 600-vertex polygon, varying the target:

| Target (% of n) | Areal displacement |
|---|---|
| 90% (1 440 v) | 1.79e-02 |
| 80% (1 280 v) | 4.02e-02 |
| 70% (1 120 v) | 6.77e-02 |
| 60% (960 v) | 9.94e-02 |
| 50% (800 v) | 1.78e-01 |
| 40% (640 v) | 2.93e-01 |
| 30% (480 v) | 5.36e-01 |
| 20% (320 v) | 9.76e-01 |
| 10% (160 v) | 4.86e+00 |

Displacement rises steeply below ~40% of the original count. Above that threshold simplification is nearly lossless. Area is preserved exactly in all cases.

The filled-in experimental evaluation write-up is at `benchmark/eval_section.md`.
