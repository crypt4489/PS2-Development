#include "ps_gameobject.h"

#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "ps_dma.h"
#include "ps_vumanager.h"
#include "ps_manager.h"
#include "ps_misc.h"
#include "ps_obb.h"
#include "ps_gs.h"
#include "ps_vif.h"
#include "ps_vu1pipeline.h"
#include "ps_morphtarget.h"



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
  if (obj)
  {

    while (pipes != NULL)
    {
      freepipe = pipes;
      pipes = pipes->next;
      DeletePipeline(freepipe);
    }

    if (obj->obb)
    {
      DestroyOBB(obj->obb);
    }

    if (obj->vertexBuffer.vertices)
      free(obj->vertexBuffer.vertices);
    if (obj->vertexBuffer.texCoords)
      free(obj->vertexBuffer.texCoords);
    if (obj->vertexBuffer.normals)
      free(obj->vertexBuffer.normals);
    if (obj->vertexBuffer.indices)
      free(obj->vertexBuffer.indices);

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
  go->vertexBuffer.matCount = 0;
  go->vertexBuffer.materials = NULL;
  go->interpolator = NULL;
  return go;
}

void SetupGameObjectPrimRegs(GameObject *obj, prim_t p, color_t color, u32 regMask, u32 regCount, u32 renderState)
{
  obj->renderState.prim = p;
  obj->renderState.color = color;
  obj->renderState.state.gs_reg_mask = regMask;
  obj->renderState.state.gs_reg_count = regCount;
  obj->renderState.state.render_state.state = renderState;
}

qword_t * CreateMeshDMAUpload(qword_t *q, GameObject *obj, u32 drawSize, u16 drawCode, u32 matCount, qword_t *vu1_addr)
{
   
    u32 msize = matCount;

    if (msize == 0)
    {
        qword_t *dma_vif1 = q;
        q++;

        if ((drawCode & DRAW_MORPH) != 0)
        {
          q = CreateVU1TargetUpload(q, obj, 0, obj->vertexBuffer.vertexCount-1, drawSize, drawCode, vu1_addr);
        }
        else
        {
          q = CreateVU1VertexUpload(q, &obj->vertexBuffer, 0, obj->vertexBuffer.vertexCount-1, drawSize, drawCode, vu1_addr);
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

  MeshBuffers *targetBeg = GetMorphMeshBuffer(obj->interpolator, node->begin);//obj->interpolator->morph_targets[node->begin];

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

    q = read_unpack_data(q, top, 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    q->sw[2] = vu1_addrs->sw[2];
    q->sw[1] = vu1_addrs->sw[1];
    q->sw[0] = vu1_addrs->sw[0];
    q->sw[3] = thisIndicesCount;
    q++;

    top+=1;

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

    q = read_unpack_data(q, top, 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    q->sw[2] = vu1_addrs->sw[2];
    q->sw[1] = vu1_addrs->sw[1];
    q->sw[0] = vu1_addrs->sw[0];
    q->sw[3] = thisIndicesCount;
    q++;
    top +=1;

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

  return q;
}