#ifndef PS_RENDERDIRECT_H
#define PS_RENDERDIRECT_H
#include "ps_global.h"

void RenderRay(Ray *ray, Color color, float t);
void RenderLine(Line *line, Color color);
void RenderGameObject(GameObject *obj, Color *colors);
void RenderPlaneLine(Plane *plane, Color color, int size);
void RenderSphereLine(BoundingSphere *sphere, Color color, int size);
void RenderAABBBoxLine(BoundingBox *boxx, Color color, MATRIX world);
#endif