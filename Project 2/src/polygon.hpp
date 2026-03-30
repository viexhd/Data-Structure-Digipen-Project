#pragma once
#include "ring.hpp"
#include <vector>
#include <memory>

// A polygon with one exterior ring (ring_id 0) and zero or more interior rings (holes).
struct Polygon {
    std::vector<std::unique_ptr<Ring>> rings;

    // Total vertex count across all rings.
    size_t total_vertices() const {
        size_t n = 0;
        for (const auto& r : rings) n += r->size;
        return n;
    }

    // Number of rings (1 exterior + N holes).
    size_t ring_count() const { return rings.size(); }
};
