#include "apsc.hpp"
#include "geometry.hpp"
#include <queue>
#include <unordered_set>
#include <cstdio>

// ---- Helpers ---------------------------------------------------------------

// Build one Candidate for the sub-chain A→B→C→D in the given ring.
static Candidate make_candidate(int ring_id, Vertex* A, Vertex* B, Vertex* C, Vertex* D) {
    Vec2 vA{A->x, A->y}, vB{B->x, B->y}, vC{C->x, C->y}, vD{D->x, D->y};
    Vec2 E   = compute_E(vA, vB, vC, vD);
    double d = areal_displacement(vA, vB, vC, vD, E);
    return Candidate{d, E.x, E.y, ring_id, A, B, C, D};
}

// Push all 4-tuples in a ring into the heap.
static void enqueue_ring(
    std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>>& pq,
    Ring& ring)
{
    if (ring.size < 4) return;
    Vertex* A = ring.head;
    do {
        Vertex* B = A->next;
        Vertex* C = B->next;
        Vertex* D = C->next;
        pq.push(make_candidate(ring.ring_id, A, B, C, D));
        A = A->next;
    } while (A != ring.head);
}

// Stale-check: a candidate is stale if B or C have been removed from their ring.
// We detect removal lazily by checking whether B->prev->next == B (i.e., B is
// still linked into the list) and similarly for C.
// Because we use raw pointers, the simplest check is to confirm B->prev->next == B.
static bool is_valid(const Candidate& c) {
    // Check B is still in a ring
    if (c.B->prev->next != c.B) return false;
    if (c.B->next->prev != c.B) return false;
    // Check C is still in a ring
    if (c.C->prev->next != c.C) return false;
    if (c.C->next->prev != c.C) return false;
    // Check adjacency: ring order must still be A→B→C→D
    if (c.A->next != c.B) return false;
    if (c.B->next != c.C) return false;
    if (c.C->next != c.D) return false;
    return true;
}

// ---- Main simplification loop ----------------------------------------------

double simplify(Polygon& poly, size_t target_n) {
    using MinHeap = std::priority_queue<Candidate,
                                        std::vector<Candidate>,
                                        std::greater<Candidate>>;
    MinHeap pq;

    // Seed the priority queue with every valid 4-tuple
    for (auto& ring : poly.rings)
        enqueue_ring(pq, *ring);

    double total_displacement = 0.0;

    while (poly.total_vertices() > target_n && !pq.empty()) {
        Candidate c = pq.top();
        pq.pop();

        // Discard stale candidates (lazy deletion)
        if (!is_valid(c)) continue;

        // Find the ring this candidate belongs to
        Ring* ring = poly.rings[c.ring_id].get();

        // A ring must keep >= 3 vertices to remain valid
        if (ring->size <= 4) continue;  // removing 2 would leave < 3

        Vec2 E{c.Ex, c.Ey};

        // Topology check: would inserting E cause any intersection?
        // TODO: also check against other rings (cross-ring intersection).
        if (collapse_causes_intersection(*ring, c.A, c.D, E)) continue;

        // Perform the collapse: remove B and C, insert E after A
        ring->remove(c.B);
        ring->remove(c.C);
        Vertex* E_vtx = ring->insert_after(c.A, c.Ex, c.Ey);

        total_displacement += c.displacement;

        // Recompute candidates for the new neighbourhood of E_vtx.
        // The affected 4-tuples start up to 3 positions before E_vtx.
        if (ring->size >= 4) {
            // Walk back 3 steps to find the new starting A for recomputation
            Vertex* start = E_vtx->prev->prev->prev;
            for (int i = 0; i < 4; ++i) {
                Vertex* A = start;
                Vertex* B = A->next;
                Vertex* C = B->next;
                Vertex* D = C->next;
                pq.push(make_candidate(ring->ring_id, A, B, C, D));
                start = start->next;
            }
        }
    }

    return total_displacement;
}
