#pragma once
#include "polygon.hpp"
#include <string>
#include <stdexcept>

// Parse a CSV file in the format:
//   ring_id,vertex_id,x,y
// Rows must be grouped by ring_id in ascending order.
// Returns a Polygon whose rings are indexed 0, 1, 2, ...
Polygon read_polygon(const std::string& filepath);

// Print the simplified polygon to stdout in exactly the required format:
//   ring_id,vertex_id,x,y   (header + one row per vertex)
//   Total signed area in input: <scientific>
//   Total signed area in output: <scientific>
//   Total areal displacement: <scientific>
void write_polygon(const Polygon& poly,
                   double area_in,
                   double area_out,
                   double total_displacement);
