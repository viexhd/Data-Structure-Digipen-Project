#include "spatial_grid.hpp"
#include "polygon.hpp"
#include <limits>
#include <cmath>

void SpatialGrid::build(const Polygon& poly) {
    double lo_x =  std::numeric_limits<double>::max();
    double lo_y =  std::numeric_limits<double>::max();
    double hi_x = -std::numeric_limits<double>::max();
    double hi_y = -std::numeric_limits<double>::max();
    size_t total = 0;

    for (const auto& ring : poly.rings) {
        if (!ring || !ring->head) continue;
        const Vertex* v = ring->head;
        do {
            if (v->x < lo_x) lo_x = v->x;
            if (v->x > hi_x) hi_x = v->x;
            if (v->y < lo_y) lo_y = v->y;
            if (v->y > hi_y) hi_y = v->y;
            ++total;
            v = v->next;
        } while (v != ring->head);
    }

    if (total == 0) return;

    const double eps = 1.0;
    min_x = lo_x - eps;
    min_y = lo_y - eps;
    double width  = (hi_x - lo_x) + 2.0 * eps;
    double height = (hi_y - lo_y) + 2.0 * eps;

    int grid_dim = std::max(1, static_cast<int>(std::sqrt(static_cast<double>(total))));
    cols = grid_dim;
    rows = grid_dim;
    cell_w = width / cols;
    cell_h = height / rows;
    if (cell_w < 1e-12) cell_w = 1.0;
    if (cell_h < 1e-12) cell_h = 1.0;

    cells.clear();
    cells.resize(static_cast<size_t>(cols) * rows);
    query_epoch = 0;

    for (const auto& ring : poly.rings) {
        if (!ring || !ring->head || ring->size < 2) continue;
        Vertex* v = ring->head;
        do {
            insert(v, ring->ring_id);
            v = v->next;
        } while (v != ring->head);
    }
}

void SpatialGrid::insert(Vertex* v, int ring_id) {
    Vertex* w = v->next;
    double x0 = std::min(v->x, w->x);
    double y0 = std::min(v->y, w->y);
    double x1 = std::max(v->x, w->x);
    double y1 = std::max(v->y, w->y);

    int cx0, cy0, cx1, cy1;
    cell_range(x0, y0, x1, y1, cx0, cy0, cx1, cy1);

    for (int cy = cy0; cy <= cy1; ++cy) {
        for (int cx = cx0; cx <= cx1; ++cx) {
            int idx = cell_index(cx, cy);
            cells[idx].push_back({v, ring_id});
        }
    }
}

void SpatialGrid::remove(Vertex* v) {
    // Recompute which cells this edge occupies (v->next still valid before ring modification)
    Vertex* w = v->next;
    double x0 = std::min(v->x, w->x);
    double y0 = std::min(v->y, w->y);
    double x1 = std::max(v->x, w->x);
    double y1 = std::max(v->y, w->y);

    int cx0, cy0, cx1, cy1;
    cell_range(x0, y0, x1, y1, cx0, cy0, cx1, cy1);

    for (int cy = cy0; cy <= cy1; ++cy) {
        for (int cx = cx0; cx <= cx1; ++cx) {
            int idx = cell_index(cx, cy);
            auto& cell = cells[idx];
            for (size_t i = 0; i < cell.size(); ++i) {
                if (cell[i].start == v) {
                    cell[i] = cell.back();
                    cell.pop_back();
                    break;
                }
            }
        }
    }
}
