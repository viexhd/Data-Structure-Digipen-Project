#pragma once
#include "ring.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec2;
struct Polygon;

// A segment is identified by its starting vertex (edge goes start -> start->next).
struct Segment {
    Vertex* start;
    int     ring_id;
};

// Uniform grid spatial index for accelerating segment intersection queries.
// Replaces O(n) full-ring scans with O(sqrt(n)) average-case lookups.
class SpatialGrid {
public:
    SpatialGrid() : min_x(0), min_y(0), cell_w(1), cell_h(1), cols(1), rows(1),
                    query_epoch(0) {}

    // Build the grid from all edges in a polygon.
    void build(const Polygon& poly);

    // Insert the edge starting at vertex v (v -> v->next) for the given ring.
    void insert(Vertex* v, int ring_id);

    // Remove the edge starting at vertex v (v->next must still be valid).
    void remove(Vertex* v);

    // Query: call visitor(Vertex* start, int ring_id) for each unique segment
    // whose bounding box overlaps the query bbox.
    // Visitor returns true to continue, false to stop early.
    // Uses epoch-based deduplication — zero allocations per query.
    template<typename Func>
    void query(double qx0, double qy0, double qx1, double qy1, Func&& visitor) const {
        int cx0, cy0, cx1, cy1;
        cell_range(qx0, qy0, qx1, qy1, cx0, cy0, cx1, cy1);
        ++query_epoch;
        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                int idx = cell_index(cx, cy);
                for (const auto& seg : cells[idx]) {
                    if (seg.start->last_query_epoch != query_epoch) {
                        seg.start->last_query_epoch = query_epoch;
                        if (!visitor(seg.start, seg.ring_id)) return;
                    }
                }
            }
        }
    }

private:
    double min_x, min_y;
    double cell_w, cell_h;
    int    cols, rows;

    std::vector<std::vector<Segment>> cells;
    mutable unsigned long long query_epoch;

    int cell_index(int cx, int cy) const { return cy * cols + cx; }

    void cell_range(double x0, double y0, double x1, double y1,
                    int& cx0, int& cy0, int& cx1, int& cy1) const {
        cx0 = std::max(0, std::min(cols - 1, static_cast<int>((x0 - min_x) / cell_w)));
        cy0 = std::max(0, std::min(rows - 1, static_cast<int>((y0 - min_y) / cell_h)));
        cx1 = std::max(0, std::min(cols - 1, static_cast<int>((x1 - min_x) / cell_w)));
        cy1 = std::max(0, std::min(rows - 1, static_cast<int>((y1 - min_y) / cell_h)));
    }
};
