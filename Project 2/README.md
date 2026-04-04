# Project 2 — Area- and Topology-Preserving Polygon Simplification

CSD2183 Data Structures | DigiPen Institute of Technology

## Build

Requires g++ with C++17 support (macOS, Linux, or WSL on Windows).

> **Note:** All source files (`apsc.cpp`, `csv_io.cpp`, `geometry.cpp`, `main.cpp`,
> `spatial_grid.cpp`) live in the **repository root**, not a `src/` subdirectory.
> The Makefile must reflect this — if your Makefile sets `SRC_DIR := src`, update it
> to `SRC_DIR := .` (or remove the variable and use `$(wildcard *.cpp)` directly).

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
./simplify test_cases/input_rectangle_with_two_holes.csv 11
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

### C++ (build)

No third-party C++ libraries are required. The implementation uses only the C++17
standard library. The spatial index is a custom uniform grid built from scratch.

### Python (benchmarking only)

The benchmarking scripts (`gen_datasets.py`, `run_benchmarks.py`, `plot_results.py`)
require Python 3.9+ and the following packages:

```bash
pip install numpy matplotlib scipy psutil
```

| Package      | Used by                | Purpose                               |
|---|---|---|
| `numpy`      | `gen_datasets.py`      | Polygon generation (Fourier modes)    |
| `matplotlib` | `plot_results.py`      | Benchmark figures                     |
| `scipy`      | `plot_results.py`      | Curve fitting (`curve_fit`)           |
| `psutil`     | `run_benchmarks.py`    | Peak RSS polling (native Linux/macOS) |

> **WSL users:** Peak memory is measured via `/usr/bin/time -v` automatically when
> running on Windows + WSL. `psutil` is still required for the import but the
> `time -v` path is preferred and more accurate.

## Test Results

Reference test cases are in `test_cases/`. Input CSVs and expected output `.txt` files
must both be present for `run_tests.sh` to run a case.

| Test case | Input vertices | Holes | Target | Area preserved | Topology valid |
|---|---|---|---|---|---|
| input_rectangle_with_two_holes.csv | 12 | 2 | 11 | Yes | Yes |
| input_cushion_with_hexagonal_hole.csv | 22 | 1 | 13 | Yes | Yes |
| input_blob_with_two_holes.csv | 36 | 2 | 17 | Yes | Yes |
| input_wavy_with_three_holes.csv | 43 | 3 | 21 | Yes | Yes |
| input_lake_with_two_islands.csv | 81 | 2 | 17 | Yes | Yes |
| input_original_01.csv | 1,860 | 0 | 99 | Yes | Yes |
| input_original_02.csv | 8,605 | 0 | 99 | Yes | Yes |
| input_original_03.csv | 74,559 | 0 | 99 | Yes | Yes |
| input_original_04.csv | 6,733 | 0 | 99 | Yes | Yes |
| input_original_05.csv | 6,230 | 0 | 99 | Yes | Yes |
| input_original_06.csv | 14,122 | 0 | 99 | Yes | Yes |
| input_original_07.csv | 10,596 | 0 | 99 | Yes | Yes |
| input_original_08.csv | 6,850 | 0 | 99 | Yes | Yes |
| input_original_09.csv | 409,998 | 0 | 99 | Yes | Yes |
| input_original_10.csv | 9,899 | 0 | 99 | Yes | Yes |

All 15 test cases pass with matching expected output (area preserved, topology valid).

### Running the tests

```bash
bash run_tests.sh
```

On Windows, double-click `run_tests.bat` or run from PowerShell — it invokes
`run_tests.sh` through WSL automatically.

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

The spatial grid index divides the polygon's bounding box into sqrt(n) x sqrt(n) cells.
Each intersection query only checks segments in overlapping cells, reducing per-collapse
cost from O(n) to O(sqrt(n)) average-case.

Timing and peak memory are printed to stderr, e.g.:

```
Simplification: 5000 vertices, 770.5 ms, peak RSS: 16740 KB
```

To benchmark without the spatial grid (naive O(n) scan), build with:

```bash
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -DNDEBUG -DUSE_NAIVE"
```

#### Benchmark: Grid vs Naive O(n) Scan

All benchmarks run on WSL2 (Ubuntu 24.04), single-threaded, compiled with `-O2 -DNDEBUG`.
Target = n/2 vertices. All inputs are smooth exterior-only polygons generated by
`gen_datasets.py` (`smooth_polygon` with Fourier-perturbed circle).

| Input | Vertices | Grid time (ms) | Grid mem (MB) | Naive time (ms) | Naive mem (MB) | Speedup |
|---|---|---|---|---|---|---|
| n001600.csv | 1,600 | 3.3 | 3.8 | 104 | 3.6 | 32x |
| n006400.csv | 6,400 | 13.2 | 4.5 | 1,749 | 4.5 | 133x |
| n012800.csv | 12,800 | 33.4 | 6.8 | 5,847 | 6.4 | 175x |
| n050000.csv | 50,000 | 192 | 17.2 | 105,474 | 16.2 | 549x |
| n100000.csv | 100,000 | 473 | 32.2 | 340,122 | 29.4 | 719x |

#### Peak Memory vs Input Size

| Vertices | Grid peak RSS (MB) | Naive peak RSS (MB) |
|---|---|---|
| 1,600 | 3.8 | 3.6 |
| 6,400 | 4.5 | 4.5 |
| 12,800 | 6.8 | 6.4 |
| 50,000 | 17.2 | 16.2 |
| 100,000 | 32.2 | 29.4 |

#### Scaling Analysis

**Naive scan:** The O(n) per-collapse intersection check produces O(n^2) total time
(n/2 collapses x O(n) each). The ~8x input increase from 1.6k to 12.8k yields ~56x
time increase (104 to 5,847 ms), consistent with O(n^2). Fitting: T_naive ~ 3.4e-8 x n^2.

**Grid-accelerated:** The grid reduces per-collapse cost to O(sqrt(n)) average-case,
yielding O(n x sqrt(n)) = O(n^1.5) total. The grid uses epoch-based deduplication and
early-exit callbacks, avoiding per-query heap allocations. The speedup increases with
input size (32x at 1.6k to 719x at 100k), confirming that the grid's advantage grows
as the naive O(n^2) cost dominates.

**Memory:** Both grid and naive versions use comparable memory (~32 MB at 100k), scaling
linearly with input size. The grid adds minimal overhead on smooth polygons with uniform
edge lengths.

## Benchmarking

The benchmarking scripts are in the **repository root** (not a `benchmark/` subdirectory).
They generate synthetic test datasets, measure wall time and peak RSS, produce the three
required plots, and fit scaling curves.

### Prerequisites

```bash
pip install numpy matplotlib scipy psutil
```

### Workflow

```bash
# 1. Generate synthetic test datasets
python gen_datasets.py

# 2. Run all experiments (5 reps each, uses WSL automatically on Windows)
python run_benchmarks.py --reps 5

# 3. Produce figures and fit scaling curves
python plot_results.py
```

> **Windows note:** The binary must be compiled inside WSL before running the harness.
> If the binary was replaced (e.g. after `git pull`), rebuild first:
> ```bash
> wsl -e bash -c "cd '/mnt/c/<path-to-project>' && make clean && make"
> ```

Results are written to `results/` (JSON) and `figures/` (PDF + PNG).

### Synthetic Datasets

| Category | Description | n range |
|----------|-------------|---------|
| `large_n/` | Smooth exterior-only polygon (Fourier-perturbed circle) | 50 - 12,800 |
| `many_holes/` | Fixed exterior + k = 1, 2, 5, 10, 20 interior holes | 324 - 780 total |
| `narrow_gap/` | Concentric rings, gap in {5, 2, 1, 0.5, 0.1} | 200 |
| `degenerate/` | Comb, zigzag, elongated (50:1 aspect), nearly-collinear | 54 - 2,000 |

### Measured Scaling (grid-accelerated, WSL2 Ubuntu 22.04, GCC 11.4, -O2, target = n/2)

Time is the median internal wall time over 5 runs; memory is peak RSS from `/usr/bin/time -v`.

| n | Time (ms) | Peak RSS (KiB) |
|---|-----------|----------------|
| 50 | 0.044 | 3,584 |
| 100 | 0.081 | 3,584 |
| 200 | 0.153 | 3,840 |
| 400 | 0.297 | 3,840 |
| 800 | 0.670 | 3,840 |
| 1,600 | 1.426 | 4,096 |
| 3,200 | 3.345 | 4,608 |
| 6,400 | 8.020 | 5,120 |
| 12,800 | 21.402 | 7,976 |
| 50,000 | 192 | 17,632 |
| 100,000 | 473 | 32,956 |

Curve-fit R^2 (scipy `curve_fit` over all 11 data points above):

| Model | Time R^2 | Memory R^2 |
|-------|---------|-----------|
| O(n sqrt(n)) | **0.9976** <- best | 0.9808 |
| O(n log n) | 0.9940 | 0.9984 |
| O(n) | 0.9880 | **0.9989** <- best |
| O(n^2) | 0.9782 | 0.9463 |

Running time fits **O(n sqrt(n))** best, consistent with O(n) collapses x O(sqrt(n))
spatial-grid queries each. Memory scales **linearly with input size (O(n))**, dominated
by vertex storage and priority queue entries.

### Areal Displacement vs. Target Count

For a fixed 1,600-vertex polygon, varying the target:

| Target (% of n) | Areal displacement |
|---|---|
| 90% (1,440 v) | 1.79e-02 |
| 80% (1,280 v) | 4.02e-02 |
| 70% (1,120 v) | 6.77e-02 |
| 60% (960 v) | 9.94e-02 |
| 50% (800 v) | 1.78e-01 |
| 40% (640 v) | 2.93e-01 |
| 30% (480 v) | 5.36e-01 |
| 20% (320 v) | 9.76e-01 |
| 10% (160 v) | 4.86e+00 |

Displacement rises steeply below ~40% of the original count. Above that threshold
simplification is nearly lossless. Area is preserved exactly in all cases.

The filled-in experimental evaluation write-up is at `eval_section.md`.
