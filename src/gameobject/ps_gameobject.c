#include "gameobject/ps_gameobject.h"

#include <string.h>
#include <stdlib.h>
#include <draw2d.h>
#include <draw3d.h>

#include "dma/ps_dma.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "physics/ps_vbo.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "pipelines/ps_vu1pipeline.h"
#include "animation/ps_morphtarget.h"
#include "log/ps_log.h"
#include "util/ps_linkedlist.h"
#include "io/ps_file_io.h"
#include "io/ps_model_io.h"

void SetupGameObjectPrimRegs(GameObject *obj, Color color, u32 renderState)
{
  ObjectProperties *render = &obj->renderState.properties;
  obj->renderState.color = color;
  render->props = renderState;
  if (render->TEXTURE_MAPPING)
  {
    obj->renderState.gsstate.gs_reg_mask = DRAW_STQ2_REGLIST;
    obj->renderState.gsstate.gs_reg_count = 3;
    obj->renderState.gsstate.prim.mapping = DRAW_ENABLE;
    obj->renderState.gsstate.prim.mapping_type = PRIM_MAP_ST;
  }
  else
  {
    obj->renderState.gsstate.gs_reg_mask = DRAW_RGBAQ_REGLIST;
    obj->renderState.gsstate.gs_reg_count = 2;
    obj->renderState.gsstate.prim.mapping = DRAW_DISABLE;
    obj->renderState.gsstate.prim.mapping_type = PRIM_MAP_UV;
  }

  if (render->ALPHA_ENABLE)
  {
    obj->renderState.gsstate.prim.blending = DRAW_ENABLE;
  }
  else
  {
    obj->renderState.gsstate.prim.blending = DRAW_DISABLE;
  }
  obj->renderState.gsstate.prim.type = PRIM_TRIANGLE;
  obj->renderState.gsstate.prim.shading = PRIM_SHADE_GOURAUD;
  obj->renderState.gsstate.prim.antialiasing = obj->renderState.gsstate.prim.fogging = DRAW_DISABLE;
  obj->renderState.gsstate.prim.colorfix = PRIM_UNFIXED;
}

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

void CleanGameObject(GameObject *obj)
{
  
  if (obj)
  {
    VU1Pipeline *pipes = obj->pipelines;
    VU1Pipeline *freepipe;
    while (pipes)
    {
      freepipe = pipes;
      pipes = pipes->next;
      DeletePipeline(freepipe);
    }

    if (obj->vertexBuffer.materials)
    {
      LinkedList *list = obj->vertexBuffer.materials;
      while(list)
      {
        free(list->data);
        list = CleanLinkedListNode(list);
      }
    }

    if (obj->vboContainer)
    {
      DestroyVBO(obj->vboContainer);
    }

    if (obj->vertexBuffer.indices)
    {
      free(obj->vertexBuffer.indices);
    }
    for (int i = 0; i < 2; i++)
    {
      if (!obj->vertexBuffer.meshData[i])
      {
        continue;
      }
      if (obj->vertexBuffer.meshData[i]->vertices)
      {
        free(obj->vertexBuffer.meshData[i]->vertices);
      }

      if (obj->vertexBuffer.meshData[i]->normals)
      {
        free(obj->vertexBuffer.meshData[i]->normals);
      }

      if (obj->vertexBuffer.meshData[i]->texCoords)
      {
        free(obj->vertexBuffer.meshData[i]->texCoords);
      }

      if (obj->vertexBuffer.meshData[i]->bones)
      {
        free(obj->vertexBuffer.meshData[i]->bones);
      }

      if (obj->vertexBuffer.meshData[i]->weights)
      {
        free(obj->vertexBuffer.meshData[i]->weights);
      }

      if (obj->vertexBuffer.meshData[i]->colors)
      {
        free(obj->vertexBuffer.meshData[i]->colors);
      }

      DestroyAnimationMesh(obj->vertexBuffer.meshAnimationData);
    }

    if (obj->objAnimator)
    {
      free(obj->objAnimator);
    }

    if (obj->interpolator)
    {
      DestroyMorphTarget(obj->interpolator);
    }

    free(obj);
  }
}

GameObject *InitializeGameObject()
{
  GameObject *go = (GameObject *)malloc(sizeof(GameObject));
  go->activePipeline = NULL;
  go->pipelines = NULL;
  go->objData = NULL;
  go->update_object = NULL;
  go->vertexBuffer.indices = NULL;
  go->vertexBuffer.meshData[0] = (MeshVectors*)malloc(sizeof(MeshVectors));
  go->vertexBuffer.meshData[MESHTRIANGLES] = (MeshVectors*)malloc(sizeof(MeshVectors));
  for (int i = 0; i < 2; i++)
  {
    go->vertexBuffer.meshData[i]->vertexCount = 0;

    go->vertexBuffer.meshData[i]->vertices = NULL;
    go->vertexBuffer.meshData[i]->normals = NULL;
    go->vertexBuffer.meshData[i]->texCoords = NULL;
    go->vertexBuffer.meshData[i]->bones = NULL;
    go->vertexBuffer.meshData[i]->weights = NULL;
    go->vertexBuffer.meshData[i]->colors = NULL;
  }
  go->vertexBuffer.matCount = 0;
  go->vertexBuffer.materials = NULL;
  go->vertexBuffer.meshAnimationData = NULL;
  go->vboContainer = NULL;
  go->interpolator = NULL;
  go->objAnimator = NULL;
  return go;
}

VertexType GetVertexType(ObjectProperties *state)
{
  VertexType ret = V_POS | (state->COLOR_ENABLE * V_COLOR);
  ret |= (state->TEXTURE_MAPPING * V_TEXTURE);
  ret |= (state->LIGHTING_ENABLE * V_NORMAL);
  ret |= (state->SKELETAL_ANIMATION * V_SKINNED);
  return ret;
}