#ifndef PS_MATERIAL_H
#define PS_MATERIAL_H
#include "ps_global.h"
MeshBuffers *CreateMaterial(MeshBuffers *buff, u32 start, u32 end, u64 id);
LinkedList *AddMaterial(LinkedList *list, Material *mat);
#endif