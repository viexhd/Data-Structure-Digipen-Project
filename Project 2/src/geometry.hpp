#pragma once
#include "ring.hpp"
#include <cmath>

// ---- Basic 2D vector helpers -----------------------------------------------

struct Vec2 { double x, y; };

inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline double cross(Vec2 a, Vec2 b)   { return a.x * b.y - a.y * b.x; }
inline double dot(Vec2 a, Vec2 b)     { return a.x * b.x + a.y * b.y; }
inline double len2(Vec2 a)            { return dot(a, a); }

// ---- Area ------------------------------------------------------------------

// Signed area of a ring via the shoelace formula.
// Positive for CCW (exterior ring), negative for CW (interior/hole).
double signed_area(const Ring& ring);

// ---- APSC geometry (Kronenfeld et al. 2020, Section 3) ---------------------

// Given four consecutive vertices A, B, C, D in a ring, compute the Steiner
// point E such that:
//   (a) replacing B,C with E preserves the signed area of the ring exactly, and
//   (b) the areal displacement is minimised among all area-preserving placements.
//
// Implements eq. (1b) and the placement pseudo-code from Kronenfeld et al. (2020).
Vec2 compute_E(Vec2 A, Vec2 B, Vec2 C, Vec2 D);

// Total unsigned area enclosed between path A→B→C→D and the replacement path
// A→E→D.  Handles the self-intersecting case by splitting at the crossing of
// the new and old edges.
double areal_displacement(Vec2 A, Vec2 B, Vec2 C, Vec2 D, Vec2 E);

// ---- Intersection checks ---------------------------------------------------

// Returns true if segments (p1,p2) and (p3,p4) properly intersect
// (share an interior point, excluding shared endpoints).
bool segments_intersect(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4);

// Returns true if inserting E between A and D (replacing B and C) would cause
// any edge in the ring to cross either new edge A→E or E→D.
// This is an O(n) scan.  Replace with a spatial index for large inputs (Person 3).
bool collapse_causes_intersection(const Ring& ring, Vertex* A, Vertex* D, Vec2 E);
