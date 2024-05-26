#ifndef PS_LINE_H
#define PS_LINE_H
#include "ps_global.h"
#include "physics/ps_vbo.h"
int LineSegmentIntersectPlane(Line *line, VECTOR plane, VECTOR point);
int LineSegmentIntersectSphere(Line *line, BoundingSphere *sphere, VECTOR point);
int LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point);
float DistanceFromLineSegment(Line *line, VECTOR point, VECTOR close);
int LineSegmentIntersectsTriangle(Line *line, VECTOR a, VECTOR b, VECTOR c, VECTOR coordinates);
int LineSegmentIntersectForAllTriangles(Line *line, VECTOR *verts, u32 count, MATRIX m, void(*ft)(VECTOR*, int));
int LineIntersectLine(Line *l1, Line *l2, VECTOR point);
#endif
