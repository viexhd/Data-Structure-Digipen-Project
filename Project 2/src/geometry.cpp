#include "geometry.hpp"
#include <cassert>
#include <cmath>
#include <algorithm>

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

    // --- Signed cross products (proportional to perpendicular distance) ---
    double crossB = cross_AD_AP(A, D, B);   // > 0 => B left of AD
    double crossC = cross_AD_AP(A, D, C);   // > 0 => C left of AD

    bool   same_side = (crossB * crossC >= 0.0);
    bool   use_AB;   // true => intersect E* with line AB; false => with line CD

    if (same_side) {
        // Compare distances (|cross| is proportional to distance from AD)
        use_AB = (std::abs(crossB) >= std::abs(crossC));
    } else {
        // Opposite sides: compare side of B with side of E* relative to AD.
        // Pick any point on E* to determine its side.
        Vec2 Estar_pt;
        if (std::abs(b) >= std::abs(a)) {
            // b != 0: set x=0, y = -c/b
            Estar_pt = {0.0, -c / b};
        } else {
            // a != 0: set y=0, x = -c/a
            Estar_pt = {-c / a, 0.0};
        }
        double crossEstar = cross_AD_AP(A, D, Estar_pt);
        use_AB = ((crossB >= 0.0) == (crossEstar >= 0.0));
    }

    if (use_AB) {
        return intersect_Estar_with_line(a, b, c, A, B);
    } else {
        return intersect_Estar_with_line(a, b, c, C, D);
    }
}

// ---- areal_displacement ----------------------------------------------------

// Signed area of triangle P1, P2, P3 (positive if CCW).
static double tri_area(Vec2 P1, Vec2 P2, Vec2 P3) {
    return 0.5 * ((P2.x - P1.x) * (P3.y - P1.y)
                - (P3.x - P1.x) * (P2.y - P1.y));
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

    // Case 1: new edge E→D crosses old inner edge B→C
    if (seg_intersect_params(E, D, B, C, t, s)) {
        Vec2 I = {E.x + t * (D.x - E.x), E.y + t * (D.y - E.y)};
        double a1 = tri_area(A, B, I) + tri_area(A, I, E);
        double a2 = tri_area(I, C, D) + tri_area(I, D, E);
        return std::abs(a1) + std::abs(a2);
    }

    // Case 2: new edge A→E crosses old inner edge B→C
    if (seg_intersect_params(A, E, B, C, t, s)) {
        Vec2 I = {A.x + t * (E.x - A.x), A.y + t * (E.y - A.y)};
        double a1 = std::abs(tri_area(A, B, I));
        double a2 = std::abs(tri_area(I, C, D) + tri_area(I, D, E));
        return a1 + a2;
    }

    // No crossing with B→C: polygon A,B,C,D,E is simple — use shoelace directly.
    double area = (A.x * B.y - B.x * A.y)
                + (B.x * C.y - C.x * B.y)
                + (C.x * D.y - D.x * C.y)
                + (D.x * E.y - E.x * D.y)
                + (E.x * A.y - A.x * E.y);
    return std::abs(area) * 0.5;
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

bool collapse_causes_intersection(const Ring& ring, Vertex* A, Vertex* D, Vec2 E) {
    Vec2 vA = {A->x, A->y};
    Vec2 vD = {D->x, D->y};

    const Vertex* u = ring.head;
    do {
        const Vertex* w = u->next;
        if (u == A || u == D || w == A || w == D) {
            u = w;
            continue;
        }
        Vec2 pu = {u->x, u->y};
        Vec2 pw = {w->x, w->y};
        if (segments_intersect(vA, E, pu, pw)) return true;
        if (segments_intersect(E, vD, pu, pw)) return true;
        u = w;
    } while (u != ring.head);
    return false;
}
