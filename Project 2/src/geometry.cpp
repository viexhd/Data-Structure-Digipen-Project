#include "geometry.hpp"
#include "polygon.hpp"
#include "spatial_grid.hpp"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>
#include <unordered_set>

// ---- Area ------------------------------------------------------------------

double signed_area(const Ring& ring) {
    if (!ring.head || ring.size < 3) return 0.0;
    double area = 0.0;
    const Vertex* v = ring.head;
    do {
        area += v->x * v->next->y - v->next->x * v->y;
        v = v->next;
    } while (v != ring.head);
    return area * 0.5;
}

// ---- Internal helpers ------------------------------------------------------

// Signed cross product of vectors (D-A) and (P-A).
// Positive => P is to the LEFT of the directed line A→D.
// Negative => P is to the RIGHT.
static double cross_AD_AP(Vec2 A, Vec2 D, Vec2 P) {
    return (D.x - A.x) * (P.y - A.y) - (D.y - A.y) * (P.x - A.x);
}

// Intersection of the line  a*x + b*y + c = 0  with the infinite line through P1 and P2.
// Returns the intersection point. If lines are parallel, projects the midpoint of P1P2
// onto E* (fallback — should rarely trigger for valid input).
static Vec2 intersect_Estar_with_line(double a, double b, double c, Vec2 P1, Vec2 P2) {
    double dx    = P2.x - P1.x;
    double dy    = P2.y - P1.y;
    double denom = a * dx + b * dy;
    if (std::abs(denom) < 1e-15) {
        // Lines are parallel: project midpoint onto E*
        Vec2   mid = {(P1.x + P2.x) * 0.5, (P1.y + P2.y) * 0.5};
        double n2  = a * a + b * b;
        double val = a * mid.x + b * mid.y + c;
        return {mid.x - a * val / n2, mid.y - b * val / n2};
    }
    double t = -(a * P1.x + b * P1.y + c) / denom;
    return {P1.x + t * dx, P1.y + t * dy};
}

// ---- compute_E -------------------------------------------------------------

// Implements the APSC placement function from Kronenfeld et al. (2020), Section 3.
//
// Derivation:
//   The area-preserving constraint (eq. 1b) defines a line E*:
//     a*xE + b*yE + c = 0
//   where
//     a = yD - yA
//     b = xA - xD
//     c = -yB*xA + (yA-yC)*xB + (yB-yD)*xC + yC*xD
//
//   E* is parallel to AD.  Any E on E* exactly preserves the ring's signed area.
//
//   Among all points on E*, the one that minimises areal displacement is:
//     - If B and C are on the SAME side of AD:
//         take the intersection of E* with AB (if B is farther from AD),
//         or with CD (if C is farther from AD).
//     - If B and C are on OPPOSITE sides of AD:
//         take the intersection of E* with AB (if B is on the same side as E*),
//         or with CD otherwise.
//   (Kronenfeld et al. 2020, Section 3, Figures 3-4 and the placement pseudo-code.)
Vec2 compute_E(Vec2 A, Vec2 B, Vec2 C, Vec2 D) {
    // --- Compute E* line: a*x + b*y + c = 0 ---
    double a =  D.y - A.y;
    double b =  A.x - D.x;
    double c = -B.y * A.x
             + (A.y - C.y) * B.x
             + (B.y - D.y) * C.x
             +  C.y * D.x;

    // Degenerate: A == D (zero-length base)
    double ad2 = (D.x - A.x) * (D.x - A.x) + (D.y - A.y) * (D.y - A.y);
    if (ad2 < 1e-18) {
        return {(B.x + C.x) * 0.5, (B.y + C.y) * 0.5};
    }

    // Degenerate: E* coincides with line AD (every point on AD preserves area)
    // Detected by checking that A lies on E*.
    double val_A = a * A.x + b * A.y + c;
    double scale = std::abs(a) + std::abs(b) + 1.0;
    if (std::abs(val_A) < 1e-10 * scale) {
        // Any point on AD is optimal; return midpoint.
        return {(A.x + D.x) * 0.5, (A.y + D.y) * 0.5};
    }

    double crossB = cross_AD_AP(A, D, B);
    double crossC = cross_AD_AP(A, D, C);

    bool same_side = (crossB * crossC >= 0.0);
    bool use_AB;

    if (same_side) {
        use_AB = (std::abs(crossB) >= std::abs(crossC));
    } else {
        Vec2 Estar_pt;
        if (std::abs(b) >= std::abs(a)) {
            Estar_pt = {0.0, -c / b};
        } else {
            Estar_pt = {-c / a, 0.0};
        }
        double crossEstar = cross_AD_AP(A, D, Estar_pt);
        use_AB = ((crossB >= 0.0) == (crossEstar >= 0.0));
    }

    return use_AB
        ? intersect_Estar_with_line(a, b, c, A, B)
        : intersect_Estar_with_line(a, b, c, C, D);
}

// ---- areal_displacement ----------------------------------------------------

// Signed area of triangle P1, P2, P3 (positive if CCW).
static double tri_area(Vec2 P1, Vec2 P2, Vec2 P3) {
    return 0.5 * ((P2.x - P1.x) * (P3.y - P1.y)
                - (P3.x - P1.x) * (P2.y - P1.y));
}

static double poly_area(const std::vector<Vec2>& pts) {
    if (pts.size() < 3) return 0.0;
    long double acc = 0.0L;
    for (size_t i = 0, n = pts.size(); i < n; ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1) % n];
        acc += static_cast<long double>(a.x) * static_cast<long double>(b.y)
             - static_cast<long double>(b.x) * static_cast<long double>(a.y);
    }
    return std::abs(static_cast<double>(acc * 0.5L));
}

// Compute the parameter t in [0,1] where segment P1→P2 intersects segment P3→P4.
// Returns true and sets t, s if they intersect (including endpoints).
static bool seg_intersect_params(Vec2 P1, Vec2 P2, Vec2 P3, Vec2 P4,
                                  double& t, double& s) {
    double dx12 = P2.x - P1.x, dy12 = P2.y - P1.y;
    double dx34 = P4.x - P3.x, dy34 = P4.y - P3.y;
    double denom = dx12 * dy34 - dy12 * dx34;
    if (std::abs(denom) < 1e-15) return false;
    double dx13 = P3.x - P1.x, dy13 = P3.y - P1.y;
    t = (dx13 * dy34 - dy13 * dx34) / denom;
    s = (dx13 * dy12 - dy13 * dx12) / denom;
    const double eps = 1e-9;
    return (t >= -eps && t <= 1.0 + eps && s >= -eps && s <= 1.0 + eps);
}

// Total (unsigned) area enclosed between paths A→B→C→D and A→E→D.
//
// Area preservation guarantees L = R (left and right displacement areas are equal).
// The total displacement = L + R = 2L.
//
// We find the single intersection point I where the new path A→E→D crosses the
// old inner edge B→C (or A→E crosses B→C), split the region there, and sum the
// absolute areas of the two sub-regions.
//
// Verification against the assignment example:
//   A=(0.6,-0.2) B=(1.0,0.2) C=(1.2,0.2) D=(1.4,-0.2) E=(1.1,0.3)
//   I=(1.16, 0.2): E→D crosses B→C at t≈0.8, s≈0.8
//   Region 1 area(A,B,I,E) = 0.008, Region 2 area(I,C,D,E) = 0.008
//   Total = 0.016  ✓  (matches expected output)
double areal_displacement(Vec2 A, Vec2 B, Vec2 C, Vec2 D, Vec2 E) {
    double t, s;

    const double epsCross = 1e-10;

    if (seg_intersect_params(E, D, B, C, t, s)) {
        if (!(t <= epsCross || t >= 1.0 - epsCross || s <= epsCross || s >= 1.0 - epsCross)) {
            Vec2 I = {E.x + t * (D.x - E.x), E.y + t * (D.y - E.y)};
            double a1 = tri_area(A, B, I) + tri_area(A, I, E);
            double a2 = tri_area(I, C, D) + tri_area(I, D, E);
            return std::abs(a1) + std::abs(a2);
        }
    }

    if (seg_intersect_params(A, E, B, C, t, s)) {
        if (!(t <= epsCross || t >= 1.0 - epsCross || s <= epsCross || s >= 1.0 - epsCross)) {
            Vec2 I = {A.x + t * (E.x - A.x), A.y + t * (E.y - A.y)};
            double a1 = std::abs(tri_area(A, B, I));
            double a2 = std::abs(tri_area(I, C, D) + tri_area(I, D, E));
            return a1 + a2;
        }
    }

    if (seg_intersect_params(E, D, A, B, t, s)) {
        if (!(t <= epsCross || t >= 1.0 - epsCross || s <= epsCross || s >= 1.0 - epsCross)) {
            Vec2 I = {E.x + t * (D.x - E.x), E.y + t * (D.y - E.y)};
            double a1 = std::abs(tri_area(A, I, E));
            double a2 = poly_area({I, B, C, D, E});
            return a1 + a2;
        }
    }

    if (seg_intersect_params(A, E, C, D, t, s)) {
        if (!(t <= epsCross || t >= 1.0 - epsCross || s <= epsCross || s >= 1.0 - epsCross)) {
            Vec2 J = {A.x + t * (E.x - A.x), A.y + t * (E.y - A.y)};
            double a1 = std::abs(tri_area(J, D, E));
            double a2 = poly_area({A, B, C, J, E});
            return a1 + a2;
        }
    }

    double area = (A.x * B.y - B.x * A.y)
                + (B.x * C.y - C.x * B.y)
                + (C.x * D.y - D.x * C.y)
                + (D.x * E.y - E.x * D.y)
                + (E.x * A.y - A.x * E.y);
    double shoelace_disp = std::abs(area) * 0.5;
    double alt1 = std::abs(tri_area(A, B, C) + tri_area(A, C, E))
                + std::abs(tri_area(C, D, E));
    double alt2 = std::abs(tri_area(A, B, E))
                + std::abs(tri_area(B, C, D) + tri_area(B, D, E));
    double alt3 = std::abs(tri_area(A, B, E)) + std::abs(tri_area(E, C, D));
    return std::max(std::max(shoelace_disp, alt1), std::max(alt2, alt3));
}

// ---- Intersection checks ---------------------------------------------------

// CCW sign for three points.
static double ccw(Vec2 A, Vec2 B, Vec2 C) {
    return (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
}

bool segments_intersect(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4) {
    double d1 = ccw(p3, p4, p1);
    double d2 = ccw(p3, p4, p2);
    double d3 = ccw(p1, p2, p3);
    double d4 = ccw(p1, p2, p4);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
    return false;
}

static bool point_eq(Vec2 a, Vec2 b) {
    const double eps = 1e-12;
    return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps;
}

static int orient_sign(Vec2 a, Vec2 b, Vec2 c) {
    long double abx = static_cast<long double>(b.x) - static_cast<long double>(a.x);
    long double aby = static_cast<long double>(b.y) - static_cast<long double>(a.y);
    long double acx = static_cast<long double>(c.x) - static_cast<long double>(a.x);
    long double acy = static_cast<long double>(c.y) - static_cast<long double>(a.y);
    long double v   = abx * acy - aby * acx;
    long double tol = 1e-12L * (fabsl(abx) + fabsl(aby) + fabsl(acx) + fabsl(acy) + 1.0L);
    if (fabsl(v) <= tol) return 0;
    return (v > 0) ? 1 : -1;
}

static bool on_segment(Vec2 a, Vec2 b, Vec2 p) {
    const double eps = 1e-12;
    if (orient_sign(a, b, p) != 0) return false;
    return (p.x >= std::min(a.x, b.x) - eps && p.x <= std::max(a.x, b.x) + eps &&
            p.y >= std::min(a.y, b.y) - eps && p.y <= std::max(a.y, b.y) + eps);
}

static bool collinear_overlap_nontrivial(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    if (orient_sign(a, b, c) != 0 || orient_sign(a, b, d) != 0) return false;
    const double eps = 1e-12;
    const double abx = std::abs(b.x - a.x);
    const double aby = std::abs(b.y - a.y);
    auto proj = [&](Vec2 p) { return (abx >= aby) ? p.x : p.y; };
    double a0 = proj(a), a1 = proj(b);
    double c0 = proj(c), c1 = proj(d);
    if (a0 > a1) std::swap(a0, a1);
    if (c0 > c1) std::swap(c0, c1);
    double overlap = std::min(a1, c1) - std::max(a0, c0);
    return overlap > eps;
}

bool segments_intersect_nontrivial(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, bool ignore_shared_endpoints) {
    const bool shared_endpoint =
        point_eq(p1, p3) || point_eq(p1, p4) || point_eq(p2, p3) || point_eq(p2, p4);

    if (collinear_overlap_nontrivial(p1, p2, p3, p4)) return true;

    int o1 = orient_sign(p1, p2, p3);
    int o2 = orient_sign(p1, p2, p4);
    int o3 = orient_sign(p3, p4, p1);
    int o4 = orient_sign(p3, p4, p2);

    if (o1 * o2 < 0 && o3 * o4 < 0) return true;

    bool intersects =
        (o1 == 0 && on_segment(p1, p2, p3)) ||
        (o2 == 0 && on_segment(p1, p2, p4)) ||
        (o3 == 0 && on_segment(p3, p4, p1)) ||
        (o4 == 0 && on_segment(p3, p4, p2));

    if (!intersects) return false;
    if (ignore_shared_endpoints && shared_endpoint) return false;
    return true;
}

static bool ring_contains_point(const Ring& ring, Vec2 p) {
    if (!ring.head || ring.size == 0) return false;
    const Vertex* v = ring.head;
    do {
        if (!v->removed && point_eq({v->x, v->y}, p)) return true;
        v = v->next;
    } while (v != ring.head);
    return false;
}

bool collapse_causes_intersection(const Ring& ring, Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vec2 E, const SpatialGrid* grid) {
    Vec2 vA = {A->x, A->y};
    Vec2 vD = {D->x, D->y};

    if (point_eq(vA, E) || point_eq(vD, E)) return true;
    if (grid) {
        std::vector<Segment> near;
        grid->query(E.x, E.y, E.x, E.y, near);
        for (const auto& seg : near) {
            if (seg.ring_id != ring.ring_id) continue;
            Vertex* u = seg.start;
            Vertex* w = u->next;
            Vec2 pu = {u->x, u->y};
            Vec2 pw = {w->x, w->y};
            if ((point_eq(pu, E) && u != A && u != D) || (point_eq(pw, E) && w != A && w != D)) return true;
        }
    } else {
        if (!point_eq(vA, E) && !point_eq(vD, E) && ring_contains_point(ring, E)) return true;
    }

    if (grid) {
        std::vector<Segment> candidates;
        double ax0 = std::min({vA.x, E.x, vD.x}), ay0 = std::min({vA.y, E.y, vD.y});
        double ax1 = std::max({vA.x, E.x, vD.x}), ay1 = std::max({vA.y, E.y, vD.y});
        grid->query(ax0, ay0, ax1, ay1, candidates);

        std::unordered_set<Vertex*> seen;
        for (const auto& seg : candidates) {
            if (seg.ring_id != ring.ring_id) continue;
            Vertex* u = seg.start;
            if (!seen.insert(u).second) continue;
            Vertex* w = u->next;
            if (u == B || u == C || w == B || w == C) continue;
            if (u == A || u == D || w == A || w == D) continue;
            Vec2 pu = {u->x, u->y};
            Vec2 pw = {w->x, w->y};
            if (segments_intersect_nontrivial(vA, E, pu, pw, true)) return true;
            if (segments_intersect_nontrivial(E, vD, pu, pw, true)) return true;
        }
        return false;
    }

    const Vertex* u = ring.head;
    do {
        const Vertex* w = u->next;
        if (u == B || u == C || w == B || w == C) {
            u = w;
            continue;
        }
        Vec2 pu = {u->x, u->y};
        Vec2 pw = {w->x, w->y};
        if (segments_intersect_nontrivial(vA, E, pu, pw, true)) return true;
        if (segments_intersect_nontrivial(E, vD, pu, pw, true)) return true;
        u = w;
    } while (u != ring.head);
    return false;
}

bool collapse_causes_cross_ring_intersection(const Polygon& poly, int ring_id, Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vec2 E, const SpatialGrid* grid) {
    const Ring& ring = *poly.rings[ring_id];
    if (collapse_causes_intersection(ring, A, B, C, D, E, grid)) return true;

    Vec2 vA = {A->x, A->y};
    Vec2 vD = {D->x, D->y};

    if (grid) {
        // Use spatial index for cross-ring checks
        for (size_t j = 0; j < poly.rings.size(); ++j) {
            if (static_cast<int>(j) == ring_id) continue;
            const Ring& other = *poly.rings[j];
            if (!other.head || other.size < 2) continue;
            if (ring_contains_point(other, E)) return true;
        }

        std::vector<Segment> candidates;
        double ax0 = std::min({vA.x, E.x, vD.x}), ay0 = std::min({vA.y, E.y, vD.y});
        double ax1 = std::max({vA.x, E.x, vD.x}), ay1 = std::max({vA.y, E.y, vD.y});
        grid->query(ax0, ay0, ax1, ay1, candidates);

        std::unordered_set<Vertex*> seen;
        for (const auto& seg : candidates) {
            if (seg.ring_id == ring_id) continue;
            Vertex* u = seg.start;
            if (!seen.insert(u).second) continue;
            Vertex* w = u->next;
            Vec2 pu = {u->x, u->y};
            Vec2 pw = {w->x, w->y};
            if (segments_intersect_nontrivial(vA, E, pu, pw, false)) return true;
            if (segments_intersect_nontrivial(E, vD, pu, pw, false)) return true;
        }
        return false;
    }

    // Fallback: O(n) scan across all other rings
    for (size_t j = 0; j < poly.rings.size(); ++j) {
        if (static_cast<int>(j) == ring_id) continue;
        const Ring& other = *poly.rings[j];
        if (!other.head || other.size < 2) continue;
        if (ring_contains_point(other, E)) return true;
        const Vertex* u = other.head;
        do {
            const Vertex* w = u->next;
            Vec2 pu = {u->x, u->y};
            Vec2 pw = {w->x, w->y};
            if (segments_intersect_nontrivial(vA, E, pu, pw, false)) return true;
            if (segments_intersect_nontrivial(E, vD, pu, pw, false)) return true;
            u = w;
        } while (u != other.head);
    }
    return false;
}

static bool ring_has_any_edge_intersections(const Ring& ring) {
    if (!ring.head || ring.size < 3) return false;
    std::vector<Vec2> pts;
    pts.reserve(ring.size);
    const Vertex* v = ring.head;
    do {
        pts.push_back({v->x, v->y});
        v = v->next;
    } while (v != ring.head);

    const size_t n = pts.size();
    for (size_t i = 0; i < n; ++i) {
        Vec2 a = pts[i];
        Vec2 b = pts[(i + 1) % n];
        if (point_eq(a, b)) return true;
        for (size_t j = i + 1; j < n; ++j) {
            if (j == i) continue;
            if ((i + 1) % n == j) continue;
            if ((j + 1) % n == i) continue;
            Vec2 c = pts[j];
            Vec2 d = pts[(j + 1) % n];
            if (segments_intersect_nontrivial(a, b, c, d, false)) return true;
        }
    }
    return false;
}

bool polygon_has_any_edge_intersections(const Polygon& poly) {
    for (const auto& ringPtr : poly.rings) {
        if (ring_has_any_edge_intersections(*ringPtr)) return true;
    }
    for (size_t i = 0; i < poly.rings.size(); ++i) {
        const Ring& r1 = *poly.rings[i];
        if (!r1.head || r1.size < 2) continue;
        const Vertex* a = r1.head;
        do {
            const Vertex* b = a->next;
            Vec2 p1 = {a->x, a->y};
            Vec2 p2 = {b->x, b->y};
            for (size_t j = i + 1; j < poly.rings.size(); ++j) {
                const Ring& r2 = *poly.rings[j];
                if (!r2.head || r2.size < 2) continue;
                const Vertex* c = r2.head;
                do {
                    const Vertex* d = c->next;
                    Vec2 q1 = {c->x, c->y};
                    Vec2 q2 = {d->x, d->y};
                    if (segments_intersect_nontrivial(p1, p2, q1, q2, false)) return true;
                    c = d;
                } while (c != r2.head);
            }
            a = b;
        } while (a != r1.head);
    }
    return false;
}

void assert_polygon_topology_valid(const Polygon& poly) {
    assert(!polygon_has_any_edge_intersections(poly));
}
