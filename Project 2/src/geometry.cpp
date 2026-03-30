#include "geometry.hpp"
#include <cassert>
#include <limits>

// ---- Area ------------------------------------------------------------------

double signed_area(const Ring& ring) {
    if (!ring.head || ring.size < 3) return 0.0;
    double area = 0.0;
    const Vertex* v = ring.head;
    do {
        // Shoelace: sum of (x_i * y_{i+1} - x_{i+1} * y_i)
        area += v->x * v->next->y - v->next->x * v->y;
        v = v->next;
    } while (v != ring.head);
    return area * 0.5;
}

// ---- APSC geometry ---------------------------------------------------------

// Kronenfeld et al. (2020) Section 3:
// When collapsing A→B→C→D to A→E→D, the signed-area contribution of those
// four edges to the shoelace sum changes.  E must satisfy:
//
//   Shoelace(A,E,D) = Shoelace(A,B,C,D)
//
// where Shoelace(A,B,C,D) = 0.5*(Ax*By - Bx*Ay + Bx*Cy - Cx*By + Cx*Dy - Dx*Cy)
// and   Shoelace(A,E,D)   = 0.5*(Ax*Ey - Ex*Ay + Ex*Dy - Dx*Ey)
//                         = 0.5*(Ey*(Ax-Dx) + Ex*(Dy-Ay))
//
// This gives one linear constraint on (Ex, Ey). E is free to slide along the
// iso-area line; among all valid E, we pick the one that minimises areal
// displacement (the area enclosed between A→B→C→D and A→E→D).
//
// The minimising E is the foot of the perpendicular from the centroid of the
// removed triangle(s) projected onto the constraint line.  See Kronenfeld
// eq. (7)-(9) for the full derivation.
//
// TODO: implement the closed-form from the paper once you have read Section 3.
Vec2 compute_E(Vec2 A, Vec2 B, Vec2 C, Vec2 D) {
    // Signed area that A→B→C→D contributes to shoelace (without the 0.5 factor)
    double S_orig = A.x * B.y - B.x * A.y
                  + B.x * C.y - C.x * B.y
                  + C.x * D.y - D.x * C.y;

    // Constraint line direction: normal to (D-A) gives the iso-area direction
    Vec2 AD  = {D.x - A.x, D.y - A.y};
    double ad_len2 = len2(AD);

    // Centroid of the quadrilateral A,B,C,D as a starting guess for E
    Vec2 centroid = {(A.x + B.x + C.x + D.x) / 4.0,
                     (A.y + B.y + C.y + D.y) / 4.0};

    // Project centroid onto the constraint line passing through the midpoint of AD
    // that satisfies the area equation.
    // The constraint is: Ey*(Ax-Dx) + Ex*(Dy-Ay) = S_orig
    // Parametrise E = midpoint(A,D) + t*(AD_perp)  and solve for the correct offset.
    // TODO: replace with the exact Kronenfeld formula from Section 3 of the paper.
    Vec2 mid = {(A.x + D.x) * 0.5, (A.y + D.y) * 0.5};

    if (ad_len2 < 1e-18) {
        // A and D coincide — degenerate; return centroid of B,C
        return {(B.x + C.x) * 0.5, (B.y + C.y) * 0.5};
    }

    // Area equation evaluated at mid:
    double S_mid = A.x * mid.y - mid.x * A.y
                 + mid.x * D.y - D.x * mid.y;
    double delta_S = S_orig - S_mid;

    // AD perpendicular (rotated 90°)
    Vec2 AD_perp = {-AD.y, AD.x};
    // How far along perp do we need to shift mid to satisfy the area constraint?
    // dS/dt = cross(A, AD_perp) - cross(D, AD_perp) ... derive carefully
    // For now, use a simplified closed-form placeholder:
    double dS_dt = A.x * AD_perp.y - AD_perp.x * A.y
                 + AD_perp.x * D.y - D.x * AD_perp.y;

    double t = (std::abs(dS_dt) > 1e-18) ? delta_S / dS_dt : 0.0;
    (void)centroid; // suppress warning until TODO is resolved
    return {mid.x + t * AD_perp.x, mid.y + t * AD_perp.y};
}

double areal_displacement(Vec2 A, Vec2 B, Vec2 C, Vec2 D, Vec2 E) {
    // Areal displacement = |area enclosed between A→B→C→D and A→E→D|
    // = |signed_area(A,B,C,D) - signed_area(A,E,D)|  ... but these are equal by construction.
    // Instead, compute the actual enclosed area using the shoelace on the combined polygon.
    // Split into two triangles: A-B-E and C-D-E (depending on which side E falls).
    // A cleaner decomposition: the enclosed region is polygon A,B,C,E  +/-  A,C,D,E
    // TODO: verify this against the paper definition.
    double area_ABCE = std::abs(
          A.x*(B.y - E.y) + B.x*(C.y - A.y) + C.x*(E.y - B.y) + E.x*(A.y - C.y)
    ) * 0.5;
    double area_ACDE = std::abs(
          A.x*(C.y - E.y) + C.x*(D.y - A.y) + D.x*(E.y - C.y) + E.x*(A.y - D.y)
    ) * 0.5;
    // The symmetric difference of the two paths shares vertex A and D;
    // total displacement is the area swept between them.
    return area_ABCE + area_ACDE;
}

// ---- Intersection checks ---------------------------------------------------

// Standard CCW test
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

    // Collinear overlap cases (treat as non-intersecting for robustness)
    return false;
}

bool collapse_causes_intersection(const Ring& ring, Vertex* A, Vertex* D, Vec2 E) {
    // The two new edges introduced are A→E and E→D.
    Vec2 vA = {A->x, A->y};
    Vec2 vD = {D->x, D->y};

    const Vertex* u = ring.head;
    do {
        const Vertex* w = u->next;
        // Skip edges that share an endpoint with the new edges
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
