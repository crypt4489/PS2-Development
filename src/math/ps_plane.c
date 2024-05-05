#include "math/ps_plane.h"
#include "math/ps_vector.h"
#include "math/ps_fast_maths.h"

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
    ScaleVectorXYZ(n, n, d);
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