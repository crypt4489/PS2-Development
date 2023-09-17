#include "gameobject/ps_gameobject.h"

#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "dma/ps_dma.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "physics/ps_obb.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "pipelines/ps_vu1pipeline.h"
#include "animation/ps_morphtarget.h"
#include "log/ps_log.h"
#include "util/ps_linkedlist.h"

void SetupGameObjectPrimRegs(GameObject *obj, color_t color, u32 renderState)
{
  OBJ_RENDER_STATE *render = &obj->renderState.state.render_state;
  obj->renderState.color = color;
  render->state = renderState;
  if (render->TEXTURE_MAPPING)
  {
    obj->renderState.state.gs_reg_mask = DRAW_STQ2_REGLIST;
    obj->renderState.state.gs_reg_count = 3;
    obj->renderState.prim.mapping = DRAW_ENABLE;
    obj->renderState.prim.mapping_type = PRIM_MAP_ST;
  }
  else
  {
    obj->renderState.state.gs_reg_mask = DRAW_RGBAQ_REGLIST;
    obj->renderState.state.gs_reg_count = 2;
    obj->renderState.prim.mapping = DRAW_DISABLE;
    obj->renderState.prim.mapping_type = PRIM_MAP_UV;
  }

  if (render->ALPHA_ENABLE)
  {
    obj->renderState.prim.blending = DRAW_ENABLE;
  }
  else
  {
    obj->renderState.prim.blending = DRAW_DISABLE;
  }
  obj->renderState.prim.type = PRIM_TRIANGLE;
  obj->renderState.prim.shading = PRIM_SHADE_GOURAUD;
  obj->renderState.prim.antialiasing = obj->renderState.prim.fogging = DRAW_DISABLE;
  obj->renderState.prim.colorfix = PRIM_UNFIXED;
}

MeshBuffers *CreateMaterial(MeshBuffers *buff, u32 start, u32 end, u32 id)
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
  // free(obj->pipeline_dma);
  VU1Pipeline *pipes = obj->pipelines;
  VU1Pipeline *freepipe;
  if (obj != NULL)
  {

    while (pipes != NULL)
    {
      freepipe = pipes;
      pipes = pipes->next;
      DeletePipeline(freepipe);
    }

    if (obj->obb != NULL)
    {
      DestroyOBB(obj->obb);
    }

    if (obj->vertexBuffer.indices != NULL)
    {
      free(obj->vertexBuffer.indices);
    }

    if (obj->vertexBuffer.vertices != NULL)
    {
      free(obj->vertexBuffer.vertices);
    }

    if (obj->vertexBuffer.normals != NULL)
    {
      free(obj->vertexBuffer.normals);
    }

    if (obj->vertexBuffer.texCoords != NULL)
    {
      free(obj->vertexBuffer.texCoords);
    }

    if (obj->vertexBuffer.bones != NULL)
    {
      free(obj->vertexBuffer.bones);
    }

    if (obj->vertexBuffer.weights != NULL)
    {
      free(obj->vertexBuffer.weights);
    }

    if (obj->vertexBuffer.colors != NULL)
    {
      free(obj->vertexBuffer.colors);
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
  go->vertexBuffer.vertexCount = 0;
  go->vertexBuffer.indices = NULL;
  go->vertexBuffer.vertices = NULL;
  go->vertexBuffer.normals = NULL;
  go->vertexBuffer.texCoords = NULL;
  go->vertexBuffer.bones = NULL;
  go->vertexBuffer.weights = NULL;
  go->vertexBuffer.colors = NULL;
  go->vertexBuffer.matCount = 0;
  go->vertexBuffer.materials = NULL;
  go->vertexBuffer.meshAnimationData = NULL;
  go->obb = NULL;
  go->interpolator = NULL;
  go->objAnimator = NULL;
  return go;
}

qword_t *CreateMeshDMAUpload(qword_t *q, GameObject *obj, u32 drawSize, u16 drawCode, u32 matCount, qword_t *vu1_addr)
{

  u32 msize = matCount;

  if (msize == 0)
  {
    qword_t *dma_vif1 = q;
    q++;

    if ((drawCode & DRAW_MORPH) != 0)
    {
      q = CreateVU1TargetUpload(q, obj, 0, obj->vertexBuffer.vertexCount - 1, drawSize, drawCode, vu1_addr);
    }
    else
    {
      q = CreateVU1VertexUpload(q, &obj->vertexBuffer, 0, obj->vertexBuffer.vertexCount - 1, drawSize, drawCode, vu1_addr);
    }

    u32 meshPipe = q - dma_vif1 - 1;

    CreateDCODEMeshUpload(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
  }
  else
  {
    LinkedList *node = obj->vertexBuffer.materials;
    for (u32 index = 0; index < msize; index++, node = node->next)
    {

      Material *mat = (Material *)node->data;

      // q = CreateLoadByIdDCODETag(q, id);

      q = CreateMaterialDCODETag(q, (u32)mat);

      qword_t *dma_vif1 = q;
      q++;

      if ((drawCode & DRAW_MORPH) != 0)
      {
        q = CreateVU1TargetUpload(q, obj, mat->start, mat->end, drawSize, drawCode, vu1_addr);
      }
      else
      {
        q = CreateVU1VertexUpload(q, &obj->vertexBuffer, mat->start, mat->end, drawSize, drawCode, vu1_addr);
      }

      u32 meshPipe = q - dma_vif1 - 1;

      CreateDCODEMeshUpload(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    }
  }

  return q;
}

qword_t *CreateVU1TargetUpload(qword_t *q, GameObject *obj, u32 start, u32 end, u32 drawSize, u8 code, qword_t *vu1_addrs)
{
  int first = 1;

  u32 idxCount = (end - start) + 1;

  int numOfVerts = idxCount;

  Interpolator *node = GetCurrentInterpolatorNode(obj->interpolator);

  MeshBuffers *targetBeg = GetMorphMeshBuffer(obj->interpolator, node->begin); // obj->interpolator->morph_targets[node->begin];

  MeshBuffers *targetEnd = GetMorphMeshBuffer(obj->interpolator, node->end);

  u32 top = 0;

  while (numOfVerts > 0)
  {
    int thisIndicesCount = 0;

    int indexOffset = idxCount - numOfVerts;

    if (numOfVerts > drawSize)
    {
      thisIndicesCount = drawSize;
      numOfVerts -= thisIndicesCount;
    }
    else
    {
      thisIndicesCount = numOfVerts;
      numOfVerts -= thisIndicesCount;
    }

    q = ReadUnpackData(q, top, 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    q->sw[2] = vu1_addrs->sw[2];
    q->sw[1] = vu1_addrs->sw[1];
    q->sw[0] = vu1_addrs->sw[0];
    q->sw[3] = thisIndicesCount;
    q++;

    top += 1;

    q = PackBuffersVU1(q, targetBeg, thisIndicesCount, &top, indexOffset + start, code);

    q = PackBuffersVU1(q, targetEnd, thisIndicesCount, &top, indexOffset + start, code);

    top = 0;

    if (first)
    {
      q = CreateDMATag(q, DMA_CNT, 0, 0, VIF_CODE(vu1_addrs->sw[3], 0, VIF_CMD_MSCAL, 0), 0);
      first = 0;
    }
    else
    {
      q = CreateDMATag(q, DMA_CNT, 0, 0, VIF_CODE(vu1_addrs->sw[3], 0, VIF_CMD_MSCAL, 0), 0);
    }
  }

  q = CreateDMATag(q, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

  return q;
}

qword_t *CreateVU1VertexUpload(qword_t *q, MeshBuffers *buffer, u32 start, u32 end, u32 drawSize, u8 code, qword_t *vu1_addrs)
{
  int first = 1;

  u32 idxCount = (end - start) + 1;

  int numOfVerts = idxCount;

  u32 top = 0;

  while (numOfVerts > 0)
  {
    int thisIndicesCount = 0;

    int indexOffset = idxCount - numOfVerts;

    if (numOfVerts > drawSize)
    {
      thisIndicesCount = drawSize;
      numOfVerts -= thisIndicesCount;
    }
    else
    {
      thisIndicesCount = numOfVerts;
      numOfVerts -= thisIndicesCount;
    }

    q = ReadUnpackData(q, top, 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    q->sw[2] = vu1_addrs->sw[2];
    q->sw[1] = vu1_addrs->sw[1];
    q->sw[0] = vu1_addrs->sw[0];
    q->sw[3] = thisIndicesCount;
    q++;
    top += 1;

    q = PackBuffersVU1(q, buffer, thisIndicesCount, &top, indexOffset + start, code);

    top = 0;
    if (first)
    {
      q = CreateDMATag(q, DMA_CNT, 0, 0, VIF_CODE(vu1_addrs->sw[3], 0, VIF_CMD_MSCAL, 0), 0);
      first = 0;
    }
    else
    {
      q = CreateDMATag(q, DMA_CNT, 0, 0, VIF_CODE(vu1_addrs->sw[3], 0, VIF_CMD_MSCAL, 0), 0);
    }
  }

  q = CreateDMATag(q, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

  return q;
}

qword_t *PackBuffersVU1(qword_t *q, MeshBuffers *buffer, u32 count, u32 *top, u32 offset, u8 code)
{
  q = add_unpack_data(q, *top, &(buffer->vertices[offset]), count, 1, VIF_CMD_UNPACK(0, 3, 0));

  *top += count;

  if (code & DRAW_TEXTURE)
  {
    q = add_unpack_data(q, *top, &(buffer->texCoords[offset]), count, 1, VIF_CMD_UNPACK(0, 3, 0));

    *top += count;
  }

  if (code & DRAW_NORMAL)
  {

    q = add_unpack_data(q, *top, &(buffer->normals[offset]), count, 1, VIF_CMD_UNPACK(0, 3, 0));

    *top += count;
  }

  if (code & DRAW_SKINNED)
  {
    // DEBUGLOG("HERREREERERERER!!!");
    q = add_unpack_data(q, *top, &(buffer->bones[offset]), count, 1, VIF_CMD_UNPACK(0, 3, 0));

    *top += count;

    q = add_unpack_data(q, *top, &(buffer->weights[offset]), count, 1, VIF_CMD_UNPACK(0, 3, 0));

    *top += count;
  }

  return q;
}
