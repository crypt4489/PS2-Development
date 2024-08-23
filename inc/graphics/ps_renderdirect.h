#ifndef PS_RENDERDIRECT_H
#define PS_RENDERDIRECT_H
#include "ps_global.h"

void RenderRay(Ray *ray, Color color, float t);
void RenderLine(Line *line, Color color);
void RenderGameObject(GameObject *obj);
void RenderPlaneLine(Plane *plane, Color color, int size);
void RenderSphereLine(BoundingSphere *sphere, Color color, int size);
void RenderAABBBoxLine(BoundingBox *boxx, Color color, MATRIX world);
void RenderVertices(VECTOR *verts, u32 numVerts, Color color);
void UploadBuffers(u32 start, u32 end, u32 maxCount, MeshVectors *buffer, VertexType type);
int DrawHeaderSize(GameObject *obj, int *baseHeader);
int MaxUploadSize(VertexType type, u32 headerEnd, u32 regCount, bool clipping);
void DetermineVU1Programs(ObjectProperties *state, qword_t *programs);
LinkedList *LoadMaterial(LinkedList *list, bool enddma, bool immediate, u32 *start, u32 *end);
#endif