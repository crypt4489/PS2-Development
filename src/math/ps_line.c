#include "math/ps_line.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"
#include "math/ps_fast_maths.h"
#include "physics/ps_vbo.h"
#include "math/ps_matrix.h"

#include <float.h>

#define COLLISION 0
#define NOCOLLISION 1


int LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point)
{
    VECTOR halfwidths, center;
    FindCenterAndHalfAABB(box, center, halfwidths);

    VECTOR m, d;

    VectorAddXYZ(line->p1, line->p2, m);
    VectorScaleXYZ(m, m, 0.5f);

    VectorSubtractXYZ(line->p2, m, d);
    VectorSubtractXYZ(m, center, m);

    float adx = Abs(d[0]);
    if (Abs(m[0]) > halfwidths[0] + adx) return NOCOLLISION;
    float ady = Abs(d[1]);
    if (Abs(m[1]) > halfwidths[1] + ady) return NOCOLLISION;
    float adz = Abs(d[2]);
    if (Abs(m[2]) > halfwidths[2] + adz) return NOCOLLISION;

    adx += 0.001; ady += 0.001; adz += 0.001;

    if (Abs(m[1] * d[2] - m[2] * d[1]) > halfwidths[1] * adz + halfwidths[2] * ady) return NOCOLLISION;
    if (Abs(m[2] * d[0] - m[0] * d[2]) > halfwidths[0] * adz + halfwidths[2] * adx) return NOCOLLISION;
    if (Abs(m[0] * d[1] - m[1] * d[0]) > halfwidths[0] * ady + halfwidths[1] * adx) return NOCOLLISION;



    return COLLISION;
}

int LineSegmentIntersectSphere(Line *line, BoundingSphere *sphere, VECTOR point)
{
   float d = DistanceFromLineSegment(line, sphere->center, point);
   if (Abs(d) < sphere->radius) return COLLISION;
   return NOCOLLISION;
}

int LineSegmentIntersectPlane(Line *line, VECTOR plane, VECTOR point)
{
    VECTOR dist;

    VectorSubtractXYZ(line->p2, line->p1, dist);
    float p1n = DotProduct(plane, line->p1);
    float distn = DotProduct(plane, dist);
    float d = -plane[3];

    if (distn == 0.0f)
    {
        DEBUGLOG("Division by zero avoided");
        distn+=.001;
    }

    float t = (d - p1n) / distn;

    if (t >= 0.0f && t <= 1.0f)
    {
        VectorScaleXYZ(dist, dist, t);
        VectorAddXYZ(line->p1, dist, point);
        return COLLISION;
    }

    return NOCOLLISION;
}

float DistanceFromLineSegment(Line *line, VECTOR point, VECTOR close)
{
    VECTOR dir, m;
    VectorSubtractXYZ(line->p2, line->p1, dir);
    VectorSubtractXYZ(point, line->p1, m);

    float dots = DotProduct(m, dir) / DotProduct(dir, dir);

    if (dots <= 0.0f)
    {
        VectorCopy(close, line->p1);
    } else if (dots >= 1.0f) {
        VectorCopy(close, line->p2);
    } else {    

        VectorScaleXYZ(m, dir, dots);

        VectorAddXYZ(m, line->p1, close);
    }

    return DistFromPoints(close, point);

}

int LineSegmentIntersectsTriangle(Line *line, VECTOR a, VECTOR b, VECTOR c, VECTOR coordinates)
{
    VECTOR ab, ac, qp;
    VectorSubtractXYZ(b, a, ab);
    VectorSubtractXYZ(c, a, ac);
    VectorSubtractXYZ(line->p1, line->p2, qp);

    VECTOR n;
    CrossProduct(ab, ac, n);
    float d = DotProduct(qp, n);
    if (d <= 0.0f) return NOCOLLISION;

    VECTOR ap;

    VectorSubtractXYZ(line->p1, a, ap);
    float t = DotProduct(ap, n);
    if (t > d || t < 0.0f) return NOCOLLISION;

    VECTOR e;
    CrossProduct(qp, ap, e);

    float v = DotProduct(ac, e);
    if (v < 0.0f || v > d) return NOCOLLISION;
    float w = -DotProduct(ab, e);
    if (w < 0.0f || (v+w)>d) return NOCOLLISION;

    float o = 1.0f / d;
    t *= o;
    w *= o;
    v *= o;
    float u = 1.0f - v - w;
    CreateVector(u, v, w, t, coordinates);
    return COLLISION;
}

int LineSegmentIntersectForAllTriangles(Line *line, VECTOR *verts, u32 count, MATRIX m, void(*ft)(VECTOR*, int))
{
        

    
    for (int i = 0; i<count; i+=3)
    {
        VECTOR a, b, c, d;
       
        MatrixVectorMultiply(a, m, verts[i]);
        MatrixVectorMultiply(b, m, verts[i+1]);
        MatrixVectorMultiply(c, m, verts[i+2]);
        int ret =  LineSegmentIntersectsTriangle(line, c, b, a, d);
        if (ret)
        {
            ret =  LineSegmentIntersectsTriangle(line, a, b, c, d);
        }


         if (!ret)
        {
            ft(verts+i, i);
        }

    }
    return 0;
}


int LineIntersectLine(Line *l1, Line *l2, VECTOR point)
{
    VECTOR l12l11, l22l21, l21l11, ab, cb, ca;
    VectorSubtractXYZ(l1->p2, l1->p1, l12l11);
    VectorSubtractXYZ(l2->p2, l2->p1, l22l21);
    VectorSubtractXYZ(l2->p1, l1->p1, l21l11);
    CrossProduct(l12l11, l22l21, ab);
    CrossProduct(l21l11, l22l21, cb);
    CrossProduct(l21l11, l12l11, ca);


    float num = DotProduct(l21l11, ab);

    if (num != 0.0f)
    {
        return NOCOLLISION;
    }

    float denom = DotProduct(ab, ab);

    if (denom == 0.0f)
    {
       
        if ((Min(l1->p1[0], l1->p2[0]) <= l2->p1[0] && Max(l1->p1[0], l1->p2[0]) >= l2->p2[0])
        && (Min(l1->p1[1], l1->p2[1]) <= l2->p1[1] && Max(l1->p1[1], l1->p2[1]) >= l2->p2[1])
        && (Min(l1->p1[2], l1->p2[2]) <= l2->p1[2] && Max(l1->p1[2], l1->p2[2]) >= l2->p2[2]))
        {
            VectorCopy(point, l2->p1);
            return COLLISION;
        }
        return NOCOLLISION;
    }

    float s = DotProduct(cb, ab)/denom;
    float t = DotProduct(ca, ab)/denom;

    if ((t >= 0.0f && t <= 1.0f) && (s >= 0.0f && s <= 1.0f))
    {
    
        VectorScaleXYZ(point, l12l11, s);
        VectorAddXYZ(point, l1->p1, point);
        return COLLISION;
    } 

    return NOCOLLISION;
}