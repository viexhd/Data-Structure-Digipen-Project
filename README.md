# Data Structures Projects (DigiPen CSD2183)

Two C++17 projects from DigiPen's **CSD2183 Data Structures** course. Each one is built around picking the right data structure for the job and measuring the result.

| | Project | Main data structures | Folder |
|---|---|---|---|
| 1 | **Game Inventory Management System**: a console inventory manager for a game | Singly linked list, merge sort | [`Project 1/`](Project%201/) |
| 2 | **Area- and Topology-Preserving Polygon Simplification**: reduces a polygon's vertex count while keeping its exact area and valid topology | Circular doubly linked lists, min-heap with lazy deletion, uniform spatial grid | [`Project 2/`](Project%202/) |

---

## Project 1: Game Inventory Management System

A menu-driven console app that stores game items (name, rarity, value, quantity) in a **singly linked list**. It starts with six sample items so you can try every feature straight away.

**Features**

- Add, remove (by ID), search (by name) and update the quantity of items. An item is removed when its quantity reaches 0.
- Sort by name, value, rarity or quantity with a linked-list **merge sort**.
- Five rarity tiers: Common, Uncommon, Rare, Epic, Legendary.
- A formatted table view with total inventory value.

**Complexity**

| Operation | Time |
|---|---|
| Add item (at head) | O(1) |
| Remove / search item | O(n) |
| Sort (merge sort) | O(n log n) |
| Display | O(n) |

**Build and run**

```bash
cd "Project 1"
g++ -std=c++17 -o game_inventory game_inventory.cpp
./game_inventory            # game_inventory.exe on Windows
```

With MSVC: `cl /EHsc /std:c++17 game_inventory.cpp`

The full user guide is in [`Project 1/Readme.txt`](Project%201/Readme.txt).

---

## Project 2: Polygon Simplification (APSC)

A command-line tool that simplifies polygons, including polygons with holes, down to a target vertex count. It implements the **Area-Preserving Segment Collapse** algorithm (Kronenfeld et al., 2020). Every collapse keeps the total signed area exactly the same, never makes rings intersect, and always picks the collapse that moves the shape the least.

**How it works**

- **Circular doubly linked list** per ring, for O(1) vertex removal and insertion.
- **Min-heap priority queue** of candidate collapses, ordered by areal displacement, so the cheapest collapse always goes next.
- **Lazy deletion**: a candidate whose vertices have already been removed is thrown away when it is popped.
- **Uniform spatial grid** (built from scratch) for intersection checks. Each check costs O(√n) on average instead of O(n).

**Results**

- All 15 reference test cases pass, from a 12-vertex rectangle with holes up to a polygon with about 410,000 vertices.
- At 100,000 vertices the grid version takes **473 ms**, against **340 s** for a naive O(n) scan: about **719× faster**.
- Measured running time fits **O(n√n)** best (R² = 0.998). Memory grows linearly.

<p align="center">
  <img src="Project%202/benchmark/figures/fig1_time_vs_n.png" alt="Running time against input size for the polygon simplifier" width="600">
</p>

**Build and run** (g++ with C++17: Linux, macOS or WSL)

```bash
cd "Project 2"
make
./simplify test_cases/input_blob_with_two_holes.csv 17
bash run_tests.sh           # runs all reference test cases
```

Input and output are CSV files of `ring_id,vertex_id,x,y`. Ring 0 is the outer boundary and the other rings are holes. The program also prints the input area, output area and total areal displacement.

The benchmark scripts (Python, in `Project 2/benchmark/`) generate synthetic datasets, measure time and peak memory, and plot the figures. The full write-up, including benchmark tables and the curve fits, is in [`Project 2/README.md`](Project%202/README.md) and [`Project 2/benchmark/eval_section.md`](Project%202/benchmark/eval_section.md). Slides and a recorded walkthrough are in [`Project 2/presentation/`](Project%202/presentation/).

---

## Repository layout

```
Project 1/
    game_inventory.cpp      Linked-list inventory manager (single file)
    Readme.txt              User guide
Project 2/
    src/                    APSC simplifier (geometry, spatial grid, CSV I/O)
    test_cases/             Reference inputs and expected outputs
    benchmark/              Dataset generator, benchmark runner, plots, results
    presentation/           Slides and video
    Makefile, run_tests.sh, run_tests.bat
```

## Requirements

- A C++17 compiler (g++ or MSVC). Neither project uses third-party C++ libraries.
- For Project 2's benchmarks only: Python 3.9+ with `numpy`, `matplotlib`, `scipy` and `psutil`.
