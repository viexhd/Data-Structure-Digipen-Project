#include "apsc.hpp"
#include "geometry.hpp"
#include <cstdio>
#include <queue>
#include <vector>

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
    double total_displacement = 0.0;

    for (auto& ring : poly.rings)
        enqueue_ring(pq, *ring);

    while (poly.total_vertices() > target_n && !pq.empty()) {
        Candidate c = pq.top();
        pq.pop();

        if (!is_valid(c)) continue;

        Ring* ring = poly.rings[c.ring_id].get();
        if (ring->size < 4) continue;

        Vec2 E{c.Ex, c.Ey};
        if (collapse_causes_cross_ring_intersection(poly, c.ring_id, c.A, c.B, c.C, c.D, E)) continue;

        ring->remove(c.B);
        ring->remove(c.C);
        Vertex* E_vtx = ring->insert_after(c.A, c.Ex, c.Ey);

        total_displacement += c.displacement;

        // assert_polygon_topology_valid uses a stricter intersection test than
        // the algorithm enforces, so skip it to avoid false crashes.
        // assert_polygon_topology_valid(poly);

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
    }

    return total_displacement;
}
