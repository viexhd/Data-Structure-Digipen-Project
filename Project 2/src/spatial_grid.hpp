#pragma once
#include "ring.hpp"
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

struct Vec2;
struct Polygon;

// A segment is identified by its starting vertex (edge goes start -> start->next).
struct Segment
{
    Vertex* start;
    int ring_id;
};

// Uniform grid spatial index for accelerating segment intersection queries.
// Replaces O(n) full-ring scans with O(sqrt(n)) amortised lookups.
class SpatialGrid
{
public:
    SpatialGrid() : min_x(0), min_y(0), cell_w(1), cell_h(1), cols(1), rows(1) {}

    // Build the grid from all edges in a polygon
    void build(const Polygon& poly);

    // Insert the edge starting at vertex v (v -> v->next) for the given ring
    void insert(Vertex* v, int ring_id);

    // Remove the edge starting at vertex v.
    void remove(Vertex* v);

    // Query: collect all segments whose bounding box overlaps the bbox of (p1, p2)
    // May return duplicates if a segment spans multiple cells, caller handles this
    void query(double qx0, double qy0, double qx1, double qy1,
               std::vector<Segment>& results) const;

private:
    double min_x, min_y;
    double cell_w, cell_h;
    int    cols, rows;

    std::vector<std::vector<Segment>> cells;
    std::unordered_map<Vertex*, std::vector<int>> vertex_to_cells;

    int cell_index(int cx, int cy) const
    {
        return cy * cols + cx;
    }

    void cell_range(double x0, double y0, double x1, double y1, int& cx0, int& cy0, int& cx1, int& cy1) const 
    {
        cx0 = std::max(0, std::min(cols - 1, static_cast<int>((x0 - min_x) / cell_w)));
        cy0 = std::max(0, std::min(rows - 1, static_cast<int>((y0 - min_y) / cell_h)));
        cx1 = std::max(0, std::min(cols - 1, static_cast<int>((x1 - min_x) / cell_w)));
        cy1 = std::max(0, std::min(rows - 1, static_cast<int>((y1 - min_y) / cell_h)));
    }
};
