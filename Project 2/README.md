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

Timing output is printed to stderr during execution (e.g., `Simplification: 99 vertices, 12.3 ms`).
