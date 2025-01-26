#include "gameobject/ps_material.h"

#include "util/ps_linkedlist.h"

#include <stdlib.h>

MeshBuffers *CreateMaterial(MeshBuffers *buff, u32 start, u32 end, u64 id)
{
  Material *mat = (Material *)malloc(sizeof(Material));
  mat->start = start;
  mat->end = end;
  mat->materialId = id;
  buff->materials = AddMaterial(buff->materials, mat);
  buff->matCount++;
  return buff;
}

LinkedList *AddMaterial(LinkedList *list, Material *mat)
{
  LinkedList *node = CreateLinkedListItem(mat);
  list = AddToLinkedList(list, node);
  return list;
}