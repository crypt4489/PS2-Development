#ifndef PS_RAY_H
#define PS_RAY_H
#include "ps_global.h"
#include "physics/ps_vbo.h"
bool RayIntersectBox(Ray *ray, BoundingBox *box, VECTOR p, float *t);
bool RayIntersectSphere(Ray *ray, BoundingSphere *sphere, VECTOR point);
bool RayIntersectPlane(Ray *ray, Plane *plane, VECTOR point);
bool RayIntersectRay(Ray *ray, Ray *ray2);
bool RayIntersectsTriangle(Ray *ray, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect);
#endif