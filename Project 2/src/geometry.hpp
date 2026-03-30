#pragma once
#include "ring.hpp"
#include <cmath>
#include <utility>

// ---- Basic 2D vector helpers -----------------------------------------------

struct Vec2 { double x, y; };

inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline double cross(Vec2 a, Vec2 b)   { return a.x * b.y - a.y * b.x; }
inline double dot(Vec2 a, Vec2 b)     { return a.x * b.x + a.y * b.y; }
inline double len2(Vec2 a)            { return dot(a, a); }

// ---- Area ------------------------------------------------------------------

// Signed area of a ring via the shoelace formula.
// Positive for CCW (exterior), negative for CW (interior/hole).
double signed_area(const Ring& ring);

// ---- APSC geometry ---------------------------------------------------------

// Given four consecutive vertices A, B, C, D (in ring order):
// Compute the position of replacement vertex E such that:
//   - removing B and C and inserting E preserves the signed area of the ring
//   - the areal displacement is minimised
// Returns E as a Vec2.
// Reference: Kronenfeld et al. (2020), Section 3.
Vec2 compute_E(Vec2 A, Vec2 B, Vec2 C, Vec2 D);

// Areal displacement when replacing the sub-chain A→B→C→D with A→E→D.
// Equals the area enclosed between the two paths (unsigned).
double areal_displacement(Vec2 A, Vec2 B, Vec2 C, Vec2 D, Vec2 E);

// ---- Intersection checks ---------------------------------------------------

// Returns true if segment (p1,p2) and segment (p3,p4) properly intersect
// (share an interior point, not just an endpoint).
bool segments_intersect(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4);

// Returns true if inserting E between A and D (replacing B,C) would cause
// any edge of the ring to cross any other edge (topology check).
// This is a naive O(n) scan — replace with spatial index for large inputs.
bool collapse_causes_intersection(const Ring& ring, Vertex* A, Vertex* D, Vec2 E);
