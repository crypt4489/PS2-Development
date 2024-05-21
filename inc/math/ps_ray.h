#ifndef PS_RAY_H
#define PS_RAY_H
#include "ps_global.h"
int RayPlaneIntersect(Ray *ray, Plane *plane, VECTOR point);
int RayIntersectSphere(Ray *ray, BoundingSphere *sphere, VECTOR point);
int RayIntersectPlane(Ray *ray, Plane *plane, VECTOR point);
#endif