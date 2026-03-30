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

| Test case | Input vertices | Target | Output vertices | Area preserved | Topology valid |
|---|---|---|---|---|---|
| example.csv | 11 | 8 | 8 | ✓ | ✓ |

*(Update this table as more test cases are run.)*

## Algorithm

Implements the **Area-Preserving Segment Collapse (APSC)** algorithm from:

> Kronenfeld, B. J., Stanislawski, L. V., Buttenfield, B. P., & Brockmeyer, T. (2020).
> Simplification of polylines by segment collapse: minimizing areal displacement while preserving area.
> *International Journal of Cartography*, 6(1), 22–46.

### Key Data Structures

- **Doubly-linked circular list** per ring — O(1) vertex removal and insertion
- **Min-heap priority queue** — always collapse the lowest-displacement candidate next
- **Lazy deletion** — stale candidates (whose vertices were already removed) are discarded on pop
- **Spatial index** *(planned)* — R-tree or segment tree for O(log n) intersection checks
