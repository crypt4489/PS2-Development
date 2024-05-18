#ifndef PS_LINE_H
#define PS_LINE_H
#include "ps_global.h"
int LineSegmentIntersectPlane(Line *line, VECTOR plane, VECTOR point);
int LineSegmentIntersectSphere(Line *line, BoundingSphere *sphere, VECTOR point);
int LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point);
float DistanceFromLineSegment(Line *line, VECTOR point);
#endif
