#ifndef PS_MODEL_IO_H
#define PS_MODEL_IO_H
#include "ps_global.h"
void ReadModelFile(const char *filename, MeshBuffers *buffers);

void CreateMeshBuffersFromFile(void *object, void *, u8 *buffer, u32 bufferLen);

void DestroyAnimationMesh(AnimationMesh *meshAnimationData);
#endif
