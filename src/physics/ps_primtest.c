#include "physics/ps_primtest.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"
#include "physics/ps_vbo.h"
#include "math/ps_fast_maths.h"
#include "math/ps_plane.h"
#include "math/ps_misc.h"


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
    if (DotProduct(intersect, sphere->center) <= sphere->radius * sphere->radius) return COLLISION;
    return NOCOLLISION;
}

int AABBIntersectTriangle(VECTOR a, VECTOR b, VECTOR c, BoundingBox *box)
{
    VECTOR half, cent;
    FindCenterAndHalfAABB(box, cent, half);

    VECTOR a0, b0, c0, f0, f1, f2;
    
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x00(%2)\n"
        "lqc2 $vf4, 0x00(%3)\n"
        "vsub.xyz $vf5, $vf1, $vf4\n"
        "vsub.xyz $vf6, $vf2, $vf4\n"
        "vsub.xyz $vf7, $vf3, $vf4\n"
        "sqc2 $vf5, 0x00(%4)\n"
        "sqc2 $vf6, 0x00(%5)\n"
        "sqc2 $vf7, 0x00(%6)\n"
        "vsub.xyz $vf1, $vf6, $vf5\n"
        "vsub.xyz $vf2, $vf7, $vf6\n"
        "vsub.xyz $vf3, $vf5, $vf7\n"
        "sqc2 $vf1, 0x00(%7)\n"
        "sqc2 $vf2, 0x00(%8)\n"
        "sqc2 $vf3, 0x00(%9)\n"
        : 
        : "r"(a), "r"(b), "r"(c), "r"(cent), "r"(a0), "r"(b0), "r"(c0), "r"(f0), "r"(f1), "r"(f2)
        : "memory");
        
        //axis 1

        float p0 = a0[2] * b0[1] - a0[1] * b0[2];
        float p2 = c0[2] * (f0[1]) - c0[2] * (f0[2]);
        float r = half[1] * Abs(f0[2]) + half[2] * Abs(f0[1]);
        if (Max(-Max(p0, p2), Min(p0, p2)) > r) return NOCOLLISION;

        //axis 2
        
        p0 = -a0[1] * (f1[2]) + a0[2] * (f1[1]); 
        float p1 = -b0[1] * (f1[2]) + b0[2] * (f1[1]); 
       // p2 = -c0[1] * (c0[2] - b0[2]) + c0[2] * (c0[1] - b0[1]); 
        r = half[1] * Abs(f1[2]) + half[2] * Abs(f1[1]);
        if (Max(-Max(p0, p1), Min(p0, p1)) > r) return NOCOLLISION;

         //axis 3
        
        //p0 = -a0[1] * (a0[2] - c0[2]) + a0[2] * (a0[1] - c0[1]);
        p1 = -b0[1] * (f2[2]) + b0[2] * (f2[1]); 
        p2 = -c0[1] * (f2[2]) + c0[2] * (f2[1]);
        r = half[1] * Abs(f2[2]) + half[2] * Abs(f2[1]);
        if (Max(-Max(p1, p2), Min(p1, p2)) > r) return NOCOLLISION;

         //axis 4
        
        //p0 = a0[0] * (b0[2] - a0[2]) - a0[2] * (b0[0] - a0[0]);
        p1 = b0[0] * (f0[2]) - b0[2] * (f0[0]); 
        p2 = c0[0] * (f0[2]) - c0[2] * (f0[0]);
        r = half[0] * Abs(f0[2]) + half[2] * Abs(f0[0]);
        if (Max(-Max(p1, p2), Min(p1, p2)) > r) return NOCOLLISION;

        //axis 5
        
        p0 = a0[0] * (f1[2]) - a0[2] * (f1[0]);
        //p1 = b0[0] * (c0[2] - b0[2]) - b0[2] * (b0[0] - a0[0]); 
        p2 = c0[0] * (f1[2]) - c0[2] * (f1[0]);
        r = half[0] * Abs(f1[2]) + half[2] * Abs(f1[0]);
        if (Max(-Max(p0, p2), Min(p0, p2)) > r) return NOCOLLISION;

         //axis 6
        
        //p0 = a0[0] * (a0[2] - c0[2]) - a0[2] * (a0[0] - c0[0]);
        p1 = b0[0] * (f2[2]) - b0[2] * (f2[0]); 
        p2 = c0[0] * (f2[2]) - c0[2] * (f2[0]);
        r = half[0] * Abs(f2[2]) + half[2] * Abs(f2[0]);
        if (Max(-Max(p1, p2), Min(p1, p2)) > r) return NOCOLLISION;

         //axis 7
        
       // p0 = -a0[0] * (b0[1] - a0[1]) + a0[1] * (b0[0] - a0[0]);
        p1 = -b0[0] * (f0[1]) + b0[1] * (f0[0]); 
        p2 = -c0[0] * (f0[1]) + c0[1] * (f0[0]);
        r = half[0] * Abs(f0[1]) + half[1] * Abs(f0[0]);
        if (Max(-Max(p1, p2), Min(p1, p2)) > r) return NOCOLLISION;

           //axis 8
        
        p0 = -a0[0] * (f1[1]) + a0[1] * (f1[0]);
        p1 = -b0[0] * (f1[1]) + b0[1] * (f1[0]); 
       // p2 = -c0[0] * (c0[1] - b0[1]) + c0[1] * (c0[0] - b0[0]);
        r = half[0] * Abs(f1[1]) + half[1] * Abs(f1[0]);
        if (Max(-Max(p1, p0), Min(p1, p0)) > r) return NOCOLLISION;

           //axis 9
        
        p0 = -a0[0] * (f2[1]) + a0[1] * (f2[0]);
        p1 = -b0[0] * (f2[1]) + b0[1] * (f2[0]); 
        //p2 = -c0[0] * (a0[1] - c0[1]) + c0[1] * (a0[0] - c0[0]);
        r = half[0] * Abs(f2[1]) + half[1] * Abs(f2[0]);
        if (Max(-Max(p1, p0), Min(p1, p0)) > r) return NOCOLLISION;


        VECTOR max, min;
        asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x00(%2)\n"
        "vmax.xyz $vf4, $vf1, $vf2\n"
        "vmax.xyz $vf4, $vf4, $vf3\n"
        "vmini.xyz $vf5, $vf1, $vf2\n"
        "vmini.xyz $vf5, $vf5, $vf3\n"
        "sqc2 $vf5, 0x00(%3)\n"
        "sqc2 $vf4, 0x00(%4)\n"
        : 
        : "r"(a0), "r"(b0), "r"(c0), "r"(min), "r"(max)
        : "memory");

        
        if (max[0] < -half[0] || min[0] > half[0]) return NOCOLLISION; 
        if (max[1] < -half[1] || min[1] > half[1]) return NOCOLLISION; 
        if (max[2] < -half[2] || min[2] > half[2]) return NOCOLLISION; 
        
        Plane plane;
        VECTOR planeNormal;
        CrossProduct(f0, f1, planeNormal);
        Normalize(planeNormal, planeNormal);
        ComputePlane(a, planeNormal, plane.planeEquation);
    
        return PlaneAABBCollision(&plane, box);
    
}