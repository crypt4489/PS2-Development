#ifndef PS_VBO_H
#define PS_VBO_H

#include "ps_global.h"
#define NOCOLLISION 1
#define COLLISION 0

void ReadVBOFromVU1(GameObject *obj);
void InitVBO(GameObject *obj, int type);
void DestroyVBO(ObjectBounds *bound);

int AABBCollision(VECTOR top1, VECTOR bottom1, VECTOR top2, VECTOR bottom2);

int CheckCollision(GameObject *obj1, GameObject *obj2, ...);

int SphereCollision(BoundingSphere *s1, BoundingSphere *s2);
//find center and half pos from trans, scale, and axes of a RotatedAABB
void FindCenterAndHalfRotatedAABB(BoundingBox *box, VECTOR pos, VECTOR scale, VECTOR xAxis, VECTOR yAxis, VECTOR zAxis, VECTOR outCenter, VECTOR outHalf);
void FindCenterAndHalfAABB(BoundingBox *box, VECTOR outCenter, VECTOR outHalf);

int CheckSeparatingPlane(VECTOR pos, VECTOR plane, VECTOR half1, VECTOR half2, VECTOR xAxis1, VECTOR yAxis1, VECTOR zAxis1, VECTOR xAxis2, VECTOR yAxis2, VECTOR zAxis2);

int PerformSAT(VECTOR pos, VECTOR half1, VECTOR half2, VECTOR xAxis1, VECTOR yAxis1, VECTOR zAxis1, VECTOR xAxis2, VECTOR yAxis2, VECTOR zAxis2);

void FindAABBMaxAndMinVerticesVU0(GameObject *obj);

void ComputeBoundingSphere(GameObject *obj);

void ComputeBoundingSphereIterative(GameObject *obj);

float SqrDistFromAABB(VECTOR p, BoundingBox *box);

void ClosestPointToAABB(VECTOR p, BoundingBox *box, VECTOR out);

float SqDistToOBB(VECTOR p, VECTOR center, VECTOR halfwidths);

void ClosestPointToOBB(VECTOR p, VECTOR center, VECTOR halfWidths, VECTOR q);

void MoveBox(BoundingBox *box, VECTOR move);

int SpherePlaneCollision(BoundingSphere *s, Plane *p);

int PlaneCollision(Plane *p1, Plane *p2, VECTOR axisOfIntersect, VECTOR point);

int PlaneOBBCollision(Plane *p, BoundingOrientBox *box);
int PlaneRotatedAABBCollision(Plane *p, BoundingBox *box, VECTOR right, VECTOR up, VECTOR forward);
int PlaneAABBCollision(Plane *plane, BoundingBox *box);

#endif
