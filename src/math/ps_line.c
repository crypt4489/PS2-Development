#include "math/ps_line.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"

#define COLLISION 1
#define NOCOLLISION 0

int LineSeqmentIntersectPlane(Line *line, Plane *plane, VECTOR point)
{
    VECTOR dist;

    VectorSubtractXYZ(line->p2, line->p1, dist);
    float p1n = DotProduct(plane->planeEquation, line->p1);
    float distn = DotProduct(plane->planeEquation, dist);
    float d = plane->planeEquation[3];

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