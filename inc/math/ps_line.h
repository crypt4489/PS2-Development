#ifndef PS_LINE_H
#define PS_LINE_H
#include "ps_global.h"
#include "physics/ps_vbo.h"
bool LineSegmentIntersectPlane(Line *line, VECTOR plane, VECTOR point);
bool LineSegmentIntersectSphere(Line *line, BoundingSphere *sphere, VECTOR point);
bool LineSegmentIntersectBox(Line *line, BoundingBox *box, VECTOR point);
float DistanceFromLineSegment(Line *line, VECTOR point, VECTOR close);
bool LineSegmentIntersectsTriangle(Line *line, VECTOR a, VECTOR b, VECTOR c, VECTOR coordinates);
bool LineSegmentIntersectForAllTriangles(Line *line, VECTOR *verts, u32 count, MATRIX m, void(*ft)(VECTOR*, int));
bool LineIntersectLine(Line *l1, Line *l2, VECTOR point);
#endif
