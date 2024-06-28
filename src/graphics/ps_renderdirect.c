#include "graphics/ps_renderdirect.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_fast_maths.h"
#include "math/ps_matrix.h"
#include "dma/ps_dma.h"
#include "pipelines/ps_vu1pipeline.h"
#include "gamemanager/ps_manager.h"
#include "math/ps_vector.h"
#include "pipelines/ps_pipelinecbs.h"

#include <string.h>
#include <stdlib.h>

extern VECTOR up;


void RenderRay(Ray *ray, Color color, float t)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[2];

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    VectorCopy(v[0], ray->origin);
    //VectorCopy(v[1], line->p2);
    VectorScaleXYZ(v[1], ray->direction, t);
    VectorAddXYZ(v[1], ray->origin, v[1]);
    v[1][3] = v[0][3] = 1.0f;

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;

    ret = ReadUnpackData(ret, 0, 3, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = 2;
    ret++;
    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[1]);

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}

void RenderLine(Line *line, Color color)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[2];

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    VectorCopy(v[0], line->p1);
    VectorCopy(v[1], line->p2);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;

    ret = ReadUnpackData(ret, 0, 3, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = 2;
    ret++;
    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[1]);

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}

void RenderVertices(VECTOR *verts, u32 numVerts, Color color)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;


    u32 count = numVerts;
    ret = ReadUnpackData(ret, 0,  count + 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = count;
    ret++;

    for (int i = 0; i<count; i++)
    {
        ret = VectorToQWord(ret, verts[i]);
    }

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}

void RenderGameObject(GameObject *obj, Color *colors)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;

    MatrixIdentity(vp);

    MATRIX m;

    CreateWorldMatrixLTM(obj->ltm, m);

    MatrixMultiply(vp, vp, m);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    SetupPerObjDrawVU1Header(NULL, obj, NULL, pipeline_temp);

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;


    u32 count = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount;
    ret = ReadUnpackData(ret, 0,  count*2 + 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = count ;
    ret++;

    VECTOR *verts = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices;
    for (int i = 0; i<count; i++)
    {
        ret = VectorToQWord(ret, verts[i]);
    }

    for (int i = 0; i<count; i++)
    {
        
        ret->sw[0] = (int)colors[i].r;
        ret->sw[1] = (int)colors[i].g;
        ret->sw[2] = (int)colors[i].b;
        ret->sw[3] = (int)colors[i].a;
        
        
        ret++; 
    }

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}

void RenderPlaneLine(Plane *plane, Color color, int size)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[4];
    VECTOR temp, temp2, temp3;
  

    CreateVector(1.0f *size, 0.0f, 1.0 *size, 1.0f, v[0]);
    CreateVector(1.0f *size, 0.0f, -1.0 *size, 1.0f, v[1]);
    CreateVector(-1.0f *size, 0.0f, 1.0 *size, 1.0f, v[2]);
    CreateVector(-1.0f *size, 0.0f, -1.0 *size, 1.0f, v[3]);
    
    CrossProduct(plane->planeEquation, up, temp);
    float angle = ASin(dist(temp));  

    MATRIX m;

    CreateRotationMatrix(temp, angle, m); 

    VectorCopy(&m[12], plane->pointInPlane);

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, m);
    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;

    ret = ReadUnpackData(ret, 0, (5 * 2) + 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = 10;
    ret++;

    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[1]);

    ret = VectorToQWord(ret, v[1]);
    ret = VectorToQWord(ret, v[3]);

    ret = VectorToQWord(ret, v[3]);
    ret = VectorToQWord(ret, v[2]);
    ret = VectorToQWord(ret, v[2]);
    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[3]);
    ret = VectorToQWord(ret, v[0]); 

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);

}

void RenderSphereLine(BoundingSphere *sphere, Color color, int size)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR *v = (VECTOR *)malloc(size * sizeof(VECTOR));
    VECTOR center;
    VectorCopy(center, sphere->center);
    float r = sphere->radius;

    float step = TWOPI / size;
    float ang = 0;

    for (int i = 0; i < size; i++)
    {
        CreateVector(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f, v[i]);
        ang += step;
    }

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;

    ret = ReadUnpackData(ret, 0, (size * 2) + 1, 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = size << 1;
    ret++;
    for (int i = 0; i < size - 1; i++)
    {
        ret = VectorToQWord(ret, v[i]);
        ret = VectorToQWord(ret, v[i + 1]);
    }

    ret = VectorToQWord(ret, v[size - 1]);
    ret = VectorToQWord(ret, v[0]);

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);

    free(v);
}

void RenderAABBBoxLine(BoundingBox *boxx, Color color, MATRIX world)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[8];

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, world);

    MatrixMultiply(vp, vp, g_DrawCamera->view);
    MatrixMultiply(vp, vp, g_DrawCamera->proj);

    VectorCopy(v[0], boxx->top);
    VectorCopy(v[7], boxx->bottom);

    
    CreateVector(boxx->top[0], boxx->top[1], boxx->bottom[2], 1.0f, v[1]);
    CreateVector(boxx->top[0], boxx->bottom[1], boxx->bottom[2], 1.0f, v[2]);
    CreateVector(boxx->top[0], boxx->bottom[1], boxx->top[2], 1.0f, v[3]);

    CreateVector(boxx->bottom[0], boxx->top[1], boxx->bottom[2], 1.0f, v[4]);
    CreateVector(boxx->bottom[0], boxx->top[1], boxx->top[2], 1.0f, v[5]);
    CreateVector(boxx->bottom[0], boxx->bottom[1], boxx->top[2], 1.0f, v[6]);

    qword_t *ret = InitializeDMAObject();

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    ret = InitDoubleBufferingQWord(ret, 16, GetDoubleBufferOffset(16));

    ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

    ret = CreateDirectTag(ret, 2, 0);

    ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    ret = SetupZTestGS(ret, 3, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    ret = CreateDMATag(ret, DMA_END, 16, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, 16, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    qword_t *pipeline_temp = ret;

    pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

    pipeline_temp = VIFSetupScaleVector(pipeline_temp);

    pipeline_temp->sw[0] = color.r;
    pipeline_temp->sw[1] = color.b;
    pipeline_temp->sw[2] = color.g;
    pipeline_temp->sw[3] = color.a;
    pipeline_temp++;

    pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2);
    pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
    pipeline_temp += 2;
    pipeline_temp->sw[3] = 0;

    pipeline_temp = ret;

    memcpy(pipeline_temp, vp, 4 * sizeof(qword_t));

    ret += 16;

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);
    qword_t *dma_vif1 = ret;
    ret++;

    ret = ReadUnpackData(ret, 0, 1 + (2 * 12), 1, VIF_CMD_UNPACK(0, 3, 0));

    ret->sw[3] = 24;
    ret++;
    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[1]);
    ret = VectorToQWord(ret, v[1]);
    ret = VectorToQWord(ret, v[2]);
    ret = VectorToQWord(ret, v[2]);
    ret = VectorToQWord(ret, v[3]);
    ret = VectorToQWord(ret, v[3]);
    ret = VectorToQWord(ret, v[0]);

    ret = VectorToQWord(ret, v[7]);
    ret = VectorToQWord(ret, v[4]);
    ret = VectorToQWord(ret, v[4]);
    ret = VectorToQWord(ret, v[5]);
    ret = VectorToQWord(ret, v[5]);
    ret = VectorToQWord(ret, v[6]);
    ret = VectorToQWord(ret, v[6]);
    ret = VectorToQWord(ret, v[7]);

    ret = VectorToQWord(ret, v[5]);
    ret = VectorToQWord(ret, v[0]);
    ret = VectorToQWord(ret, v[7]);
    ret = VectorToQWord(ret, v[2]);
    ret = VectorToQWord(ret, v[4]);
    ret = VectorToQWord(ret, v[1]);
    ret = VectorToQWord(ret, v[3]);
    ret = VectorToQWord(ret, v[6]);

    ret = CreateDMATag(ret, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_MSCAL, 0), 0);
    ret = CreateDMATag(ret, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);

    u32 meshPipe = ret - dma_vif1 - 1;

    CreateDCODEDmaTransferTag(dma_vif1, DMA_CHANNEL_VIF1, 1, 1, meshPipe);
    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}