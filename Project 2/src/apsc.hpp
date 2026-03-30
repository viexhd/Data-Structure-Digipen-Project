#pragma once
#include "polygon.hpp"
#include <queue>
#include <functional>
#include <cmath>

// ---- Collapse candidate ----------------------------------------------------

// Represents the candidate collapse of the 4-vertex sub-chain A→B→C→D → A→E→D.
struct Candidate {
    double displacement;  // areal displacement cost (lower = better)
    double Ex, Ey;        // position of replacement vertex E
    int    ring_id;
    Vertex* A;
    Vertex* B;            // first vertex to be removed
    Vertex* C;            // second vertex to be removed
    Vertex* D;

    // For the priority queue: smallest displacement first
    bool operator>(const Candidate& o) const {
        const double eps = 1e-12;
        double diff = displacement - o.displacement;
        if (std::fabs(diff) > eps) return diff > 0.0;
        if (ring_id != o.ring_id) return ring_id > o.ring_id;
        auto id = [](Vertex* v){ return v ? v->original_id : -1; };
        int aA = id(A), aB = id(B), aC = id(C), aD = id(D);
        int bA = id(o.A), bB = id(o.B), bC = id(o.C), bD = id(o.D);
        if (aA != bA) return aA > bA;
        if (aB != bB) return aB > bB;
        if (aC != bC) return aC > bC;
        if (aD != bD) return aD > bD;
        return A > o.A;
    }
};

// ---- APSC algorithm --------------------------------------------------------

// Simplify `poly` in-place until total vertex count <= target_n (or no more
// valid collapses exist).
//
// Returns the total accumulated areal displacement.
//
// Algorithm outline (Kronenfeld et al. 2020):
//  1. For every consecutive 4-tuple (A,B,C,D) across all rings, compute E and
//     the candidate displacement; push into a min-heap.
//  2. Pop the minimum-cost candidate.
//  3. Validate: check the candidate is not stale (B/C still exist in the ring)
//     and that inserting E does not cause any ring to self-intersect or cross
//     another ring.
//  4. If valid, perform the collapse: remove B and C, insert E between A and D.
//  5. Recompute the ~4 new candidates involving the updated neighbourhood.
//  6. Repeat from step 2.
double simplify(Polygon& poly, size_t target_n);
