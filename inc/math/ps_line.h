#ifndef PS_LINE_H
#define PS_LINE_H
#include "ps_global.h"
int LineSeqmentIntersectPlane(Line *line, VECTOR plane, VECTOR point);
int LineSegmentInterectSphere(Line *line, BoundingSphere *sphere, VECTOR point);
int LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point);
#endif
