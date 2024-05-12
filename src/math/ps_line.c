#include "math/ps_line.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"
#include "math/ps_fast_maths.h"
#include "physics/ps_vbo.h"

#include <float.h>

#define COLLISION 1
#define NOCOLLISION 0


/*
int LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point)
{
    float tmin = 0.0f;
    float tmax = FLT_MAX;
     VECTOR dir;
    VectorSubtractXYZ(line->p2, line->p1, dir);
    Normalize(dir, dir);
    VECTOR *p = &line->p1;
    for (int i = 0; i<3; i++)
    {
        if (Abs(dir[i]) < .001)
        {
            if (*p[i] < box->bottom[i] || *p[i] > box->top[i]) return NOCOLLISION;
        } 
        else 
        {
            float ood = 1.0f / dir[i];
            float t1 = (box->bottom[i] - *p[i]) * ood;
            float t2 = (box->top[i] - *p[i]) * ood;

            if (t2 > t1)
            {
                t2 = t2 + t1;
                t1 = t2 - t1;
                t2 = t2 - t1;
            }


            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);

            if (tmin > tmax) return NOCOLLISION;

        }
    }

    VectorScaleXYZ(dir, dir , tmin);
    VectorAddXYZ(line->p1, dir, point);
    return COLLISION;
} */

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

int LineSegmentInterectSphere(Line *line, BoundingSphere *sphere, VECTOR point)
{
    VECTOR dir, m;
    VectorSubtractXYZ(line->p2, line->p1, dir);
    Normalize(dir, dir);
    VectorSubtractXYZ(line->p1, sphere->center, m);

    float b = DotProduct(m, dir);
    float c = DotProduct(m, m) - sphere->radius * sphere->radius;

    if (c > 0.0f && b > 0.0f) return NOCOLLISION;

    float bbc = b*b - c;

    if (bbc < 0.0f) return NOCOLLISION;

    float t = -b - Sqrt(bbc);

    if (t < 0.0f) t = 0.0f;

    VectorScaleXYZ(dir, dir, t);

    VectorAddXYZ(dir, line->p1, point);

    return COLLISION;
}

int LineSeqmentIntersectPlane(Line *line, VECTOR plane, VECTOR point)
{
    VECTOR dist;

    VectorSubtractXYZ(line->p2, line->p1, dist);
    float p1n = DotProduct(plane, line->p1);
    float distn = DotProduct(plane, dist);
    float d = plane[3];

    if (distn == 0.0f)
    {
        DEBUGLOG("Division by zero upcoming");
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