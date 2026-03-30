#include "csv_io.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <map>

Polygon read_polygon(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open())
        throw std::runtime_error("Cannot open file: " + filepath);

    Polygon poly;
    std::string line;

    // Skip header
    if (!std::getline(f, line))
        throw std::runtime_error("Empty file: " + filepath);

    // Collect rings in a map so we handle out-of-order ring_ids gracefully
    std::map<int, std::vector<std::pair<int, std::pair<double,double>>>> tmp;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tok;
        int ring_id, vertex_id;
        double x, y;
        try {
            std::getline(ss, tok, ','); ring_id   = std::stoi(tok);
            std::getline(ss, tok, ','); vertex_id = std::stoi(tok);
            std::getline(ss, tok, ','); x         = std::stod(tok);
            std::getline(ss, tok, ','); y         = std::stod(tok);
        } catch (...) {
            throw std::runtime_error("Malformed CSV row: " + line);
        }
        tmp[ring_id].push_back({vertex_id, {x, y}});
    }

    for (auto& [rid, verts] : tmp) {
        // Sort by vertex_id to be safe
        std::sort(verts.begin(), verts.end(),
                  [](auto& a, auto& b){ return a.first < b.first; });

        auto ring = std::make_unique<Ring>(rid);
        for (auto& [vid, xy] : verts)
            ring->push_back(xy.first, xy.second, vid);

        // Ensure ring list is big enough
        while (static_cast<int>(poly.rings.size()) <= rid)
            poly.rings.push_back(nullptr);
        poly.rings[rid] = std::move(ring);
    }

    // Validate no gaps in ring ids
    for (size_t i = 0; i < poly.rings.size(); ++i)
        if (!poly.rings[i])
            throw std::runtime_error("Missing ring_id " + std::to_string(i));

    return poly;
}

void write_polygon(const Polygon& poly,
                   double area_in,
                   double area_out,
                   double total_displacement) {
    std::cout << "ring_id,vertex_id,x,y\n";
    for (const auto& ring : poly.rings) {
        int vid = 0;
        const Vertex* v = ring->head;
        do {
            // Use %g-style but with enough precision; match the example output style
            std::cout << ring->ring_id << ',' << vid++ << ','
                      << v->x << ',' << v->y << '\n';
            v = v->next;
        } while (v != ring->head);
    }
    // Summary lines in scientific notation
    std::printf("Total signed area in input: %.6e\n",  area_in);
    std::printf("Total signed area in output: %.6e\n", area_out);
    std::printf("Total areal displacement: %.6e\n",    total_displacement);
}
