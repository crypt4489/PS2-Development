#ifndef PS_PRIMTEST_H
#define PS_PRIMTEST_H
#include "ps_global.h"
bool SphereIntersectTriangle(BoundingSphere *sphere, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect);
void ClosestPointToTriangle(VECTOR point, VECTOR a, VECTOR b, VECTOR c, VECTOR intersect);
bool AABBIntersectTriangle(VECTOR a, VECTOR b, VECTOR c, BoundingBox *box);
#endif