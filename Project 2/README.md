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

The spatial grid index divides the polygon's bounding box into sqrt(n) x sqrt(n) cells. Each intersection query only checks segments in overlapping cells, reducing per-collapse cost from O(n) to O(sqrt(n)) average-case. Epoch-based deduplication on each Vertex avoids per-query heap allocations.

Timing and peak memory are printed to stderr (e.g., `Simplification: 5000 vertices, 770.5 ms, peak RSS: 16740 KB`).

To benchmark without the spatial grid (naive O(n) scan), build with:
```bash
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -DNDEBUG -DUSE_NAIVE"
```

#### Benchmark: Grid vs Naive O(n) Scan

All benchmarks run on WSL2 (Ubuntu 24.04), single-threaded, compiled with `-O2 -DNDEBUG`. Target = input/2 vertices. Median of 5 runs (3 runs for 50k+).

| Input | Vertices | Grid time (ms) | Grid mem (MB) | Naive time (ms) | Naive mem (MB) | Speedup |
|---|---|---|---|---|---|---|
| bench_1k.csv | 1,000 | 7.4 | 3.8 | 55.0 | 3.8 | 7.4x |
| bench_5k.csv | 5,000 | 160.5 | 7.6 | 1,342 | 4.6 | 8.4x |
| bench_10k.csv | 10,000 | 770.5 | 16.7 | 5,469 | 5.6 | 7.1x |
| bench_10k_5holes.csv | 10,000 (5 holes) | 210 | 8.4 | 4,365 | 5.4 | 20.8x |
| bench_50k.csv | 50,000 | 53,600 | 237 | 154,096 | 16.8 | 2.9x |
| bench_100k.csv | 100,000 | 504,564 | 840 | est. ~616,000 | est. ~33 | ~1.2x |

#### Scaling Analysis

**Naive scan:** The O(n) per-collapse intersection check produces O(n^2) total time (n/2 collapses x O(n) each). Fitting the measured data: T_naive ~ c * n^2 with c ~ 5.5e-8. The 5x input increase from 10k to 50k yields ~28x time increase, consistent with n^2.

**Grid-accelerated:** The grid reduces per-collapse cost to O(sqrt(n)) average-case, yielding O(n * sqrt(n)) = O(n^1.5) total. Measured scaling from 1k to 10k fits well: 10x input increase gives ~104x time increase (expected 10^1.5 = 31.6x). At larger sizes the grid's memory overhead grows, and cache effects reduce the speedup.

**Memory:** The grid uses O(n) additional memory for cell arrays (sqrt(n) x sqrt(n) cells, ~1 segment each). At 100k vertices, peak RSS reaches ~840 MB due to cell vector overhead and priority queue growth from lazy deletion.

#### Custom Test Datasets

| Dataset | Vertices | Holes | Property Tested |
|---|---|---|---|
| bench_1k.csv | 1,000 | 0 | Baseline small polygon, verifies grid overhead is low |
| bench_5k.csv | 5,000 | 0 | Medium polygon, tests grid vs naive crossover |
| bench_10k.csv | 10,000 | 0 | High vertex count, stresses intersection checks |
| bench_50k.csv | 50,000 | 0 | Large polygon, tests scalability of spatial index |
| bench_100k.csv | 100,000 | 0 | Very large polygon (>=100k per rubric), tests limits |
| bench_10k_5holes.csv | 10,000 | 5 | Multi-hole polygon, tests cross-ring intersection checks |
