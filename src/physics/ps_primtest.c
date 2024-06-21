#include "physics/ps_primtest.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"

#define NOCOLLISION 1
#define COLLISION 0


void ClosestPointToTriangle(VECTOR point, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect)
{
    VECTOR ab, ac;
    float d1, d2, d3, d4, d5, d6;
     asm __volatile__(
        "lqc2 $vf1, 0x00(%2)\n"
        "lqc2 $vf2, 0x00(%3)\n"
        "lqc2 $vf4, 0x00(%4)\n"
        "lqc2 $vf5, 0x00(%5)\n"
        "vsub.xyz $vf6, $vf2, $vf1\n"
        "vsub.xyz $vf7, $vf4, $vf1\n"
        "vsub.xyz $vf3, $vf5, $vf1\n"
        "vmul.xyz $vf8, $vf6, $vf3\n"
        "vmul.xyz $vf9, $vf7, $vf3\n"
        "vaddy.x $vf9x, $vf9x, $vf9y\n"
        "vaddz.x $vf9x, $vf9x, $vf9z\n"
        "vaddy.x $vf8x, $vf8x, $vf8y\n"
        "vaddz.x $vf8x, $vf8x, $vf8z\n"
        "qmfc2 %1, $vf9\n"
        "qmfc2 %0, $vf8\n"
        : "=r"(d1), "=r"(d2)
        : "r"(a), "r"(b), "r"(c), "r"(point)
        : "memory");
        
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            VectorCopy(intersect, a);
            return;
        }

        asm __volatile__(
        "vsub.xyz $vf10, $vf5, $vf2\n"
        "vmul.xyz $vf8, $vf10, $vf6\n"
        "vmul.xyz $vf9, $vf10, $vf7\n"
        "vaddy.x $vf9x, $vf9x, $vf9y\n"
        "vaddz.x $vf9x, $vf9x, $vf9z\n"
        "vaddy.x $vf8x, $vf8x, $vf8y\n"
        "vaddz.x $vf8x, $vf8x, $vf8z\n"
        "qmfc2 %1, $vf9\n"
        "qmfc2 %0, $vf8\n"
        "sqc2 $vf6, 0x00(%2)\n"
        : "=r"(d3), "=r"(d4)
        : "r"(ab)
        : "memory");
       
         if (d3 >= 0.0f && d4 <= 0.0f)
        {
            VectorCopy(intersect, b);
            return;
        }

        float vc = d1*d4 - d3*d2;
      
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float v = d1 / (d1 -d3);
            VectorScaleXYZ(intersect, ab, v);
            VectorAddXYZ(intersect, a, intersect);
            return;
        }

        asm __volatile__(
        "vsub.xyz $vf11, $vf5, $vf4\n"
        "vmul.xyz $vf8, $vf11, $vf6\n"
        "vmul.xyz $vf9, $vf11, $vf7\n"
        "vaddy.x $vf9x, $vf9x, $vf9y\n"
        "vaddz.x $vf9x, $vf9x, $vf9z\n"
        "vaddy.x $vf8x, $vf8x, $vf8y\n"
        "vaddz.x $vf8x, $vf8x, $vf8z\n"
        "qmfc2 %1, $vf9\n"
        "qmfc2 %0, $vf8\n"
        "sqc2 $vf7, 0x00(%2)\n"
        : "=r"(d5), "=r"(d6)
        : "r"(ac)
        : "memory");

        float vb = d5*d2 - d1*d6;
        
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float w = d2 / (d2 - d6);
            VectorScaleXYZ(intersect, ac, w);
            VectorAddXYZ(intersect, a, intersect);
            return;
        }

        float va = d6*d3 - d5*d4;
        
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) <= 0.0f)
        {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            VectorSubtractXYZ(c, b, intersect);
            VectorScaleXYZ(intersect, intersect, w);
            VectorAddXYZ(intersect, b, intersect);
            return;
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        VectorScaleXYZ(ab, ab, v);
        VectorScaleXYZ(ac, ac, w);
        VectorAddXYZ(ab, ac, ac);
        VectorAddXYZ(ac, a, intersect);
}



int SphereIntersectTriangle(BoundingSphere *sphere, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect)
{
    ClosestPointToTriangle(sphere->center, a , b, c, intersect);
    if (DistFromPoints(intersect, sphere->center) <= sphere->radius) return COLLISION;
    return NOCOLLISION;
}