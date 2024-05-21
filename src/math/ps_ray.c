#include "math/ps_ray.h"
#include "math/ps_line.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"
#include "math/ps_fast_maths.h"
#include "physics/ps_vbo.h"
#include "math/ps_matrix.h"

#include <float.h>
#define COLLISION 0
#define NOCOLLISION 1

int RayIntersectSphere(Ray *ray, BoundingSphere *sphere, VECTOR point)
{
     VECTOR dir, m;
  
    Normalize(ray->direction, dir);
    VectorSubtractXYZ(ray->origin, sphere->center, m);

    float b = DotProduct(m, dir);
    float c = DotProduct(m, m) - sphere->radius * sphere->radius;

    if (c > 0.0f && b > 0.0f) return NOCOLLISION;

    float bbc = b*b - c;

    if (bbc < 0.0f) return NOCOLLISION;

    float t = -b - Sqrt(bbc);

    if (t < 0.0f) t = 0.0f;

    VectorScaleXYZ(dir, dir, t);

    VectorAddXYZ(dir, ray->origin, point);

    return COLLISION;
}

int RayIntersectBox(Ray *ray, BoundingBox *box, VECTOR p, float *t)
{
    *t = 0.0f;
    float max = FLT_MAX;
    for (int i = 0; i<3; i++)
    {
        if (Abs(ray->direction[i]) < 0.001)
        {
            if (ray->origin[i] < box->bottom[i] || ray->origin[i] > box->top[i]) return NOCOLLISION;
        }
    }

    VECTOR odd, t1, t2;
    CreateVector(1.f/ray->direction[0], 1.f/ray->direction[1], 1.f/ray->direction[2], 1.0f, odd);
    VectorSubtractXYZ(box->bottom, ray->origin, t1);
    VectorMultiply(t1, odd, t1);

    VectorSubtractXYZ(box->top, ray->origin, t2);
    VectorMultiply(t2, odd, t2);

    for (int i = 0; i<3; i++)
    {
        float tsub1 = t1[i], tsub2 = t2[i];
        if (tsub1 > tsub2)
        {
            tsub1=t2[i];
            tsub2=t1[i];
        }

        *t = Max(*t, tsub1);
        max = Min(max, tsub2);
        if (*t > max)  return NOCOLLISION;
    }
    VectorScaleXYZ(p, ray->direction, *t);
    VectorAddXYZ(p, ray->origin, p);
    return COLLISION;

}
int RayIntersectPlane(Ray *ray, Plane *plane, VECTOR point)
{
    float orignormal = DotProduct(ray->origin, plane->planeEquation) - plane->planeEquation[3];
    float normaldir = DotProduct(ray->direction, plane->planeEquation);
    if (normaldir == 0.0f)
    {
        return NOCOLLISION;
    }
    float t = -orignormal/normaldir;

    if (t < 0.0f) return NOCOLLISION;
    VectorScaleXYZ(point, ray->direction, t);
    VectorAddXYZ(point, ray->origin, point);
    return COLLISION;
}