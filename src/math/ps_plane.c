#include "math/ps_plane.h"
#include "math/ps_vector.h"
#include "math/ps_fast_maths.h"
#include "math/ps_line.h"

void ComputePlane(VECTOR vec, VECTOR normal, VECTOR plane)
{
    float d;
    d = DotProduct(vec, normal) * -1.0f;
    CreateVector(normal[0], normal[1], normal[2], d, plane);
}

void PointInPlane(VECTOR plane, VECTOR p, VECTOR pointInPlane, VECTOR planePoint)
{
    VECTOR v, n;
    VectorCopy(n, plane);
    VectorSubtractXYZ(pointInPlane, p, v);
    float d = Abs(DotProduct(v, n));
    VectorScaleXYZ(n, n, d);
    VectorSubtractXYZ(p, n, planePoint);
}

void SetupPlane(VECTOR planeNormal, VECTOR planePoint, Plane *plane)
{
    VectorCopy(plane->pointInPlane, planePoint);
    ComputePlane(planePoint, planeNormal, plane->planeEquation);
}

float DistanceFromPlane(VECTOR planeEquation, VECTOR point)
{
    return DotProduct(planeEquation, point) + planeEquation[3];
}

void NormalizePlane(VECTOR in, VECTOR out)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vmaddax.w $ACC, $vf0, $vf2\n"
        "vmadday.w $ACC, $vf0, $vf2\n"
        "vmaddz.w $vf2, $vf0, $vf2\n"
        "vrsqrt $Q, $vf0w, $vf2w\n"
        "vsub.w $vf0, $vf0, $vf0\n"
        "vwaitq \n"
        "vmulq.xyzw $vf1, $vf1, $Q \n"
        "sqc2 $vf1, 0x00(%0) \n"
        :
        : "r"(out), "r"(in)
        : "memory");
}

static int IsSign(float t)
{
    if (t == 0)
    {
        return -1;
    }
    Bin2Float x;
    x.float_x = t;
    return ((x.int_x & 0x80000000) >> 31);
}

bool PlaneIntersectsTriangle(Plane *plane, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect)
{
    float d1 = DistanceFromPlane(plane->planeEquation, a);
    float d2 = DistanceFromPlane(plane->planeEquation, b);
    float d3 = DistanceFromPlane(plane->planeEquation, c);

    int i1 = IsSign(d1);
    int i2 = IsSign(d2);
    int i3 = IsSign(d3);

    if ((!i1 && !i2 && !i3) || (i1 > 0 && i2 > 0 && i3 > 0)) 
    {
        return true;
    }

    if (i1 < 0 && i2 < 0 && i3 < 0)
    {
        VectorCopy(intersect, a);
        return false;
    }
    Line line;
    if (i1 != i2)
    {
        VectorCopy(line.p1, a);
        VectorCopy(line.p2, b);
        
    } 
    else if (i2 != i3)
    {
        VectorCopy(line.p1, b);
        VectorCopy(line.p2, c);
    } 
    else if (i3 != i1)
    {
        VectorCopy(line.p1, c);
        VectorCopy(line.p2, a);
    }

    LineSegmentIntersectPlane(&line, plane->planeEquation, intersect);

    return false;

}