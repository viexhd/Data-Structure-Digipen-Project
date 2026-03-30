#include "apsc.hpp"
#include "geometry.hpp"
#include <queue>

// ---- Helpers ---------------------------------------------------------------

static Candidate make_candidate(int ring_id, Vertex* A, Vertex* B, Vertex* C, Vertex* D) {
    Vec2 vA{A->x, A->y}, vB{B->x, B->y}, vC{C->x, C->y}, vD{D->x, D->y};
    Vec2 E   = compute_E(vA, vB, vC, vD);
    double d = areal_displacement(vA, vB, vC, vD, E);
    return Candidate{d, E.x, E.y, ring_id, A, B, C, D};
}

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

// A candidate is stale if any of its vertices have been tombstoned or if the
// adjacency A→B→C→D no longer holds in the ring.
static bool is_valid(const Candidate& c) {
    if (c.A->removed || c.B->removed || c.C->removed || c.D->removed)
        return false;
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

    for (auto& ring : poly.rings)
        enqueue_ring(pq, *ring);

    double total_displacement = 0.0;

    while (poly.total_vertices() > target_n && !pq.empty()) {
        Candidate c = pq.top();
        pq.pop();

        // Discard stale entries (lazy deletion)
        if (!is_valid(c)) continue;

        Ring* ring = poly.rings[c.ring_id].get();

        // A ring must keep >= 3 vertices
        if (ring->size <= 4) continue;

        Vec2 E{c.Ex, c.Ey};

        // Topology check: would the two new edges intersect any existing edge?
        // TODO (Person 2): extend this to also check against other rings.
        if (collapse_causes_intersection(*ring, c.A, c.D, E)) continue;

        // Perform the collapse: unlink B and C, insert E between A and D.
        ring->remove(c.B);   // sets B->removed = true
        ring->remove(c.C);   // sets C->removed = true
        Vertex* E_vtx = ring->insert_after(c.A, c.Ex, c.Ey);

        total_displacement += c.displacement;

        // Recompute candidates for the updated neighbourhood of E_vtx.
        // Walk back 3 steps to cover all new 4-tuples that include E_vtx.
        if (ring->size >= 4) {
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

        // Periodically free tombstoned vertices to avoid unbounded memory growth.
        ring->flush_garbage();
    }

    // Final cleanup
    for (auto& ring : poly.rings)
        ring->flush_garbage();

    return total_displacement;
}
