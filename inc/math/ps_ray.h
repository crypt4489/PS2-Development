#ifndef PS_RAY_H
#define PS_RAY_H
#include "ps_global.h"
#include "physics/ps_vbo.h"
int RayIntersectBox(Ray *ray, BoundingBox *box, VECTOR p, float *t);
int RayIntersectSphere(Ray *ray, BoundingSphere *sphere, VECTOR point);
int RayIntersectPlane(Ray *ray, Plane *plane, VECTOR point);
int RayIntersectRay(Ray *ray, Ray *ray2);
int RayIntersectsTriangle(Ray *ray, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect);
#endif