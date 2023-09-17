#include "pipelines/ps_pipelines.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "pipelines/ps_pipelineinternal.h"
#include "gameobject/ps_gameobject.h"
#include "dma/ps_dma.h"
#include "pipelines/ps_vu1pipeline.h"
#include "system/ps_vumanager.h"
#include "gs/ps_gs.h"
#include "pipelines/ps_pipelinecbs.h"
#include "system/ps_vif.h"
#include "log/ps_log.h"
#include "util/ps_linkedlist.h"

static inline u32 UploadSize(u32 loop, u32 count)
{
    return (loop * (count + 3));
}

static inline u32 RenderPassesForAnim(u32 renderPasses, u32 pipeCode)
{
    return (renderPasses * ((VU1Stage1 & pipeCode) != 0 ? 1 : 0));
}

static inline u32 MaterialSizeDMACount(u32 msize)
{
    return (3 * msize);
}

static void CreateSizesFromRenderFlags(OBJ_RENDER_STATE renderState, u32 *pCode, u16 *dCode, u32 *renderPasses)
{
    if (renderState.CLIPPING & 1)
    {
        *pCode = *pCode | VU1StageClip;
    }

    if (renderState.COLOR_ENABLE & 1)
    {
        *dCode = *dCode | DRAW_COLOR;
    }

    if (renderState.TEXTURE_MAPPING & 1)
    {
        *dCode = *dCode | DRAW_TEXTURE;
    }

    if (renderState.LIGHTING_ENABLE & 1)
    {
        *dCode = *dCode | DRAW_NORMAL;
        *pCode = *pCode | VU1Stage3;
    }

    if (renderState.MORPH_TARGET & 1)
    {
        *dCode = *dCode | DRAW_MORPH;
        *pCode = *pCode | VU1Stage1;
    }

    if (renderState.ENVIRONMENTMAP & 1)
    {
        *dCode = *dCode | DRAW_TEXTURE;
        // *pCode = *pCode | VU1Stage2;
        *dCode = *dCode | DRAW_NORMAL;
        *dCode = *dCode | DRAW_ENVMAP;
        *renderPasses = 2;
    }

    if (renderState.ANIMATION_TEXUTRE & 1)
    {
        *dCode = *dCode | DRAW_TEXTURE;
        *pCode = *pCode | VU1Stage2;
        *renderPasses = 1;
    }

    if (renderState.SPECULAR & 1)
    {
        *dCode = *dCode | DRAW_NORMAL;
        //  *pCode = *pCode | VU1Stage3;
        *renderPasses = 2;
    }

    if (renderState.SKELETAL_ANIMATION & 1)
    {
        *dCode = *dCode | DRAW_SKINNED;
        *pCode = *pCode | VU1Stage1;
    }

    if (renderState.ALPHA_MAPPING & 1)
    {
        *dCode = *dCode | DRAW_ALPHAMAP;
        *renderPasses = 3;
    }
}

static u32 CreateDrawSizeandUploadCount(u32 dcode, u32 pcode, u32 *drawSize)
{
    u32 uploadCount = 1;
    u32 lsize = 81;
    if (dcode & DRAW_TEXTURE)
    {
        uploadCount++;
    }

    if (dcode & DRAW_NORMAL || dcode & DRAW_COLOR)
    {
        lsize = 54;
        uploadCount++;
    }

    if (pcode & VU1StageClip)
    {
        lsize = 36;
    }

    if (pcode & VU1Stage1)
    {
        lsize = 27;

        if (dcode & DRAW_MORPH)
        {
            uploadCount = uploadCount * 2;
        }
        else if (dcode & DRAW_SKINNED)
        {
            uploadCount += 2;
        }
    }

    *drawSize = lsize;
    return uploadCount;
}

static u32 CreateUpload(LinkedList *materials, u32 drawSize, u32 msize)
{
    LinkedList *node = materials;

    u32 uploadLoop = 0;

    for (u32 index = 0; index < msize; index++, node = node->next)
    {
        Material *mat = (Material *)node->data;

        // DEBUGLOG("%d %d %d", mat->start, mat->end, mat->materialId);
        uploadLoop += (((mat->end - mat->start) + 1) / drawSize) + 1;
    }

    return uploadLoop;
}

void SetupStage2MATRIX(VU1Pipeline *pipeline, MATRIX m)
{
    u32 size = pipeline->callBackSize;

    for (int i = 0; i<size; i++)
    {
        PipelineCallback *search = pipeline->cbs[i];
        if (search->id == DEFAULT_STAGE2_MATRIX_PCB)
        {
            search->args = m;
            return;
        }
    }

    ERRORLOG("Reached end of SetupStage2MATRIX unsucessfully");
}

void SetupTextureCB(VU1Pipeline *pipeline, Texture *tex)
{
    u32 size = pipeline->callBackSize;

    for (int i = 0; i<size; i++)
    {
        PipelineCallback *search = pipeline->cbs[i];
        if (search->id == SETUP_MAP_TEXTURE_PCB)
        {
            search->args = tex;
            return;
        }
    }

    ERRORLOG("Reached end of SetupMapTextureCB unsucessfully");
}

void create_pipeline_obj_wireframe_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize)
{
    u32 sizeOfPipeline;

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * qwSize);

    VU1Pipeline *pipeline = CreateVU1Pipeline("WIREFRAME_PIPELINE", 3, 1);

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += 3;

    qword_t *dcode_tag_vif1 = q;

    q++;

    q = InitDoubleBufferingQWord(q, 16, 496);

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 4, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL, DEFAULT_GS_REG_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14);

    q = ReadUnpackData(q, 0, 16, 0, VIF_CMD_UNPACK(0, 3, 0));

    qword_t *targ = q;

    PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawWireframeVU1Header, targ, NULL, 0x00);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, dcode_callback_tags, targ);

    PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, targ, NULL, 0x89);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, dcode_callback_tags, targ);

    q += 16;

    qword_t vu1_addr;
    vu1_addr.sw[0] = vu1_addr.sw[1] = vu1_addr.sw[2] = 0;

    q = CreateVU1VertexUpload(q, &obj->vertexBuffer, 0, obj->vertexBuffer.vertexCount - 1, 54, DRAW_VERTICES, &vu1_addr);

    sizeOfPipeline = q - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;
    // AddPipelineCallbackNode(pipeline, setupGSRegs);
    // AddPipelineCallbackNode(pipeline, setupVU1Header);

    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

void create_pipeline_tess_grid_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize, TessGrid *grid)
{
    u32 sizeOfPipeline;

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * qwSize);

    VU1Pipeline *pipeline = CreateVU1Pipeline("TESS_GRID_PIPELINE", 4, 1);

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += 4;

    qword_t *dcode_tag_vif1 = q;

    q++;

    q = InitDoubleBufferingQWord(q, 16, 496);

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 4, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL, DEFAULT_GS_REG_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14);

    q = ReadUnpackData(q, 0, 16, 0, VIF_CMD_UNPACK(0, 3, 0));

    PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawVU1Header, q, NULL, DEFAULT_VU1_HEADER_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, dcode_callback_tags, q);

    PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, q, NULL, DEFAULT_OBJ_WVP_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, dcode_callback_tags, q);

    PipelineCallback *setupTessGrid = CreatePipelineCBNode(SetupPerObjDrawTessVU1Header, q, (void *)grid, 0x90);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupTessGrid, dcode_callback_tags, q);

    q += 16;

    q = add_start_program_vu1(q, GetProgramAddressVU1Manager(g_Manager.vu1Manager, programNumber));

    sizeOfPipeline = q - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;
    // AddPipelineCallbackNode(pipeline, setupGSRegs);
    // AddPipelineCallbackNode(pipeline, setupVU1Header);
    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

void CreateGraphicsPipeline(GameObject *obj, const char *name)
{
    u32 totalHeader, sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize, pipeCode = VU1Stage4;
    u16 drawCode = DRAW_VERTICES;
    totalHeader = sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 1;

    CreateSizesFromRenderFlags(obj->renderState.state.render_state, &pipeCode, &drawCode, &renderPasses);

    if (msize == 0)
    {
        ERRORLOG("no materials. must be at least 1");
        return;
    }

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //
    totalHeader = headerSize;
    if (obj->vertexBuffer.meshAnimationData != NULL && ((drawCode & DRAW_SKINNED) != 0))
    {
        u32 boneCount = obj->vertexBuffer.meshAnimationData->jointsCount * 3;
        totalHeader += boneCount;
        cbsNums += 1;
    }

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

    // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + totalHeader + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    // DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    pipeline->passes[0]->target = &obj->vertexBuffer;

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += cbsNums;

    qword_t *dcode_tag_vif1 = q;

    q++;

    q = InitDoubleBufferingQWord(q, totalHeader, GetDoubleBufferOffset(headerSize)); // 2

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 4, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL, DEFAULT_GS_REG_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, totalHeader, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, totalHeader, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

    dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

    q += totalHeader;

    sizeOfDCode = q - dcode_tag_vif1 - 1;

    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode);

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

    // dump_packet(pipeline->q, 3000, 0);

    // while(1);
    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}



qword_t *CreateVU1Callbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerSize, u32 pCode, u32 dCode)
{
    PipelineCallback *setupVU1Header;

    if ((dCode & DRAW_ALPHAMAP) != 0)
    {
        setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawVU1HeaderAlphaMap, q, NULL, ALPHAMAP_VU1_HEADER_PCB);
    }
    else
    {
        setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawVU1Header, q, NULL, DEFAULT_VU1_HEADER_PCB);
    }

    tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, tag, q);

    PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, q, NULL, DEFAULT_OBJ_WVP_PCB);

    tag = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, tag, q);

    if ((pCode & VU1Stage3) != 0)
    {
        PipelineCallback *setupLightsHeader = CreatePipelineCBNode(SetupPerObjLightBuffer, q, NULL, LIGHT_BUFFER_PCB);

        tag = AddPipelineCallbackNodeQword(pipeline, setupLightsHeader, tag, q);
    }

    if ((pCode & VU1Stage2) != 0)
    {
        PipelineCallback *setupStage2Mat = CreatePipelineCBNode(SetupStage2MATVU1, q, NULL, DEFAULT_STAGE2_MATRIX_PCB);
        tag = AddPipelineCallbackNodeQword(pipeline, setupStage2Mat, tag, q);
    }

    if ((pCode & VU1Stage1) != 0)
    {
        if ((dCode & DRAW_MORPH) != 0)
        {
            tag = CreateMorphPipelineCallbacks(tag, q, pipeline);
        }
        else if ((dCode & DRAW_SKINNED) != 0)
        {
            tag = CreateSkinnedAnimationCallbacks(tag, q, pipeline, headerSize);
            tag = CreateBonesVectorsDMAUpload(tag, q + headerSize, pipeline);
        }
    }
    return tag;
}

void CreateVU1ProgramsList(qword_t *q, u32 pipeCode, u16 drawCode)
{
    u32 stage4, stage3, stage2, stage1;
    stage4 = stage3 = stage2 = stage1 = 0;

    q->sw[3] = stage4;

    if ((pipeCode & VU1StageClip) != 0)
    {
        q->sw[0] = stage4;
        q->sw[3] = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericClipper);
    }

    if ((pipeCode & VU1Stage3) != 0)
    {
        if ((drawCode & DRAW_SPECULAR) != 0)
        {
            stage3 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericSpecular);
        }
        else
        {
            stage3 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericLight3D);
        }

        if ((pipeCode & VU1StageClip) == 0)
        {
            q->sw[3] = stage3;
        }
        else
        {
            q->sw[0] = stage3;
        }
    }

    if ((pipeCode & VU1Stage2) != 0)
    {
        if ((drawCode & DRAW_ENVMAP) != 0)
        {
            stage2 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericEnvMap);
        }
        else if ((drawCode & DRAW_ANIM_TEX) != 0)
        {
            stage2 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericAnimTex);
        }

        q->sw[2] = stage3;
        if ((pipeCode & VU1StageClip) == 0)
        {
            q->sw[3] = stage2;
        }
        else
        {
            q->sw[0] = stage2;
        }
    }

    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
        {
            stage1 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericMorph);
        }
        else if ((drawCode & DRAW_SKINNED) != 0)
        {
            stage1 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericSkinned);
        }

        q->sw[1] = ((pipeCode & VU1Stage2) != 0 ? stage2 : (pipeCode & VU1Stage3) != 0 ? stage3
                                                       : (pipeCode & VU1Stage3) != 0   ? q->sw[3]
                                                                                       : stage4);
        q->sw[3] = stage1;
    }
}

void CreateEnvMapPipeline(GameObject *obj, const char *name)
{
    u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize, pipeCode = VU1Stage4;
    u16 drawCode = DRAW_VERTICES;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses;

    if (msize == 0)
    {
        ERRORLOG("no materials. must be at least 1");
        return;
    }

    CreateSizesFromRenderFlags(obj->renderState.state.render_state, &pipeCode, &drawCode, &renderPasses);

    CreatePipelineSizes(pipeCode | VU1Stage2, &cbsNums, &headerSize); //

    cbsNums+=2;

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    //  DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

    // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    uploadLoop = ((obj->vertexBuffer.vertexCount) / drawSize) + 1;

    sizeOfPipeline = sizeOfPipeline + UploadSize(uploadLoop, upload) + 1 + 3 + 10;

    // DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += (cbsNums);

    qword_t *dcode_tag_vif1 = q;

    q += 1;

    q = InitDoubleBufferingQWord(q, headerSize, GetDoubleBufferOffset(headerSize)); // 2

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 4, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL, DEFAULT_GS_REG_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

    dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode | VU1Stage2, drawCode);

    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;
    // DEBUGLOG("%d", sizeOfDCode);

    // dcode_tag_vif1->sw[0] = dcode_tag_vif1->sw[1] = dcode_tag_vif1->sw[2] = dcode_tag_vif1->sw[3] = 0;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode);

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    PipelineCallback *setupTexture = CreatePipelineCBNode(SetupMapTexture, q, NULL, SETUP_MAP_TEXTURE_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupTexture, dcode_callback_tags, q);

    q++;

    qword_t env_vu1;

    CreateVU1ProgramsList(&env_vu1, pipeCode | VU1Stage2, drawCode);

    qword_t *envmap_upload = q;

    q++;

    q = CreateDMATag(q, DMA_CNT, 4, 0, 0, 0);

    q = CreateDirectTag(q, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupEnvMapReg = CreatePipelineCBNode(SetupBlendingCXT, q, NULL, SETUP_BLENDING_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupEnvMapReg, dcode_callback_tags, q);

    q += 2;

    q = CreateDMATag(q, DMA_END, 1, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE((10 | (1 << 14) | (0 << 15)), 1, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    q++; // set prim tag

    CreateDCODEDmaTransferTag(envmap_upload, DMA_CHANNEL_VIF1, 1, 1, q - envmap_upload - 1);

    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, 0);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, DRAW_NORMAL | DRAW_VERTICES | DRAW_TEXTURE, 0, &env_vu1);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

void CreateAlphaMapPipeline(GameObject *obj, const char *name)
{
    u32 totalHeader, sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize, pipeCode = VU1Stage4;
    u16 drawCode = DRAW_VERTICES;
    totalHeader = sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 1;

    CreateSizesFromRenderFlags(obj->renderState.state.render_state, &pipeCode, &drawCode, &renderPasses);

    if (msize == 0)
    {
        ERRORLOG("no materials. must be at least 1");
        return;
    }

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //
    cbsNums += 4;
    totalHeader = headerSize;
    if (obj->vertexBuffer.meshAnimationData != NULL && ((drawCode & DRAW_SKINNED) != 0))
    {
        u32 boneCount = obj->vertexBuffer.meshAnimationData->jointsCount * 3;
        totalHeader += boneCount;
        cbsNums += 1;
    }

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

    // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + totalHeader + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    uploadLoop = ((obj->vertexBuffer.vertexCount) / drawSize) + 1;

    sizeOfPipeline = sizeOfPipeline + ((UploadSize(uploadLoop, upload) + 1 + 3 + 10) * 2);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += cbsNums;

    qword_t *dcode_tag_vif1 = q;

    q++;

    q = InitDoubleBufferingQWord(q, totalHeader, GetDoubleBufferOffset(headerSize)); // 2

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 6, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 5, 0);

    q = CreateGSSetTag(q, 4, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupAlphaMapPass1, q, NULL, ALPHAMAP_SETUP_GS_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 4; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, totalHeader, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, totalHeader, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

    dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

    q += totalHeader;

    sizeOfDCode = q - dcode_tag_vif1 - 1;

    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, VU1Stage1, DRAW_TEXTURE | DRAW_VERTICES);

    PipelineCallback *setupTexture = CreatePipelineCBNode(SetupMapTexture, q, NULL, SETUP_MAP_TEXTURE_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupTexture, dcode_callback_tags, q);

    q++;

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, DRAW_TEXTURE | DRAW_VERTICES, 0, vu1_addr);

    q = CreateDCODEDmaTransferTag(q, DMA_CHANNEL_VIF1, 1, 1, 6);

    q = CreateDMATag(q, DMA_END, 5, 0, 0, 0);

    q = CreateDirectTag(q, 4, 1);

    q = CreateGSSetTag(q, 3, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupAlphaBlend = CreatePipelineCBNode(SetupAlphaMapPass2, q, NULL, ALPHAMAP_RENDERPASS_TWO_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupAlphaBlend, dcode_callback_tags, q);

    q += 3;

    q = CreateMeshDMAUpload(q, obj, drawSize, DRAW_TEXTURE | DRAW_VERTICES, 0, vu1_addr);

    qword_t normal_vu1;

    CreateVU1ProgramsList(&normal_vu1, pipeCode, drawCode);

    qword_t *alphamap_upload = q;

    q++;

    q = CreateDMATag(q, DMA_CNT, 5, 0, 0, 0);

    q = CreateDirectTag(q, 4, 0);

    q = CreateGSSetTag(q, 3, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupAlphaMapDraw = CreatePipelineCBNode(SetupAlphaMapPass3, q, NULL, ALPHAMAP_RENDERPASS_THREE_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupAlphaMapDraw, dcode_callback_tags, q);

    q += 3;

    q = CreateDMATag(q, DMA_END, 1, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE((12 | (1 << 14) | (0 << 15)), 1, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    q++; // set prim tag

    CreateDCODEDmaTransferTag(alphamap_upload, DMA_CHANNEL_VIF1, 1, 1, q - alphamap_upload - 1);

    if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, 0);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, msize, &normal_vu1);

    q = CreateDCODEDmaTransferTag(q, DMA_CHANNEL_VIF1, 1, 1, 4);

    q = CreateDMATag(q, DMA_END, 3, 0, 0, 0);

    q = CreateDirectTag(q, 2, 1);

    q = CreateGSSetTag(q, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *finishAlphaMap = CreatePipelineCBNode(SetupAlphaMapFinish, q, NULL, ALPHAMAP_RENDERPASS_FINAL_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, finishAlphaMap, dcode_callback_tags, q);

    q += 1;

    CreateDCODETag(q, DMA_DCODE_END);
    // dump_packet(pipeline_dma, 2056, 0);
    pipeline->q = pipeline_dma;

    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

void CreateSpecularPipeline(GameObject *obj, const char *name)
{
    u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize, pipeCode = VU1Stage4;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;
    u16 drawCode = DRAW_VERTICES;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 1;

    if (msize == 0)
    {
        ERRORLOG("no materials. must be at least 1");
        return;
    }

    CreateSizesFromRenderFlags(obj->renderState.state.render_state, &pipeCode, &drawCode, &renderPasses);

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //

    cbsNums++;

    u32 cbInput = cbsNums;

    if ((pipeCode & VU1Stage1) != 0)
    {
        cbInput += renderPasses;
    }

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

    // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    sizeOfPipeline = (sizeOfPipeline * 2) + 1 + 3 + 10;

    // DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbInput, renderPasses);

    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    qword_t *q = pipeline_dma;

    qword_t *dcode_callback_tags = q;

    q += cbsNums;

    qword_t *dcode_tag_vif1 = q;

    q++;

    q = InitDoubleBufferingQWord(q, headerSize, GetDoubleBufferOffset(headerSize)); // 2

    qword_t *per_obj_tag = q;

    q = CreateDMATag(per_obj_tag, DMA_CNT, 4, 0, 0, 0);

    qword_t *direct_tag = q;

    q = CreateDirectTag(direct_tag, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL, DEFAULT_GS_REG_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

    dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;

    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode);

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    qword_t *spec_vu1 = &pipeline->passes[1]->programs;

    CreateVU1ProgramsList(spec_vu1, pipeCode, drawCode | DRAW_SPECULAR);

    qword_t *spec_upload = q;

    q++;

    q = CreateDMATag(q, DMA_CNT, 4, 0, 0, 0);

    q = CreateDirectTag(q, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *SetupAlphaAndPrimReg = CreatePipelineCBNode(SetupBlendingCXT, q, NULL, SETUP_BLENDING_PCB);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, SetupAlphaAndPrimReg, dcode_callback_tags, q);

    q += 2;

    q = CreateDMATag(q, DMA_END, 1, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE((10 | (1 << 14) | (0 << 15)), 1, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    q++; // set prim tag

    CreateDCODEDmaTransferTag(spec_upload, DMA_CHANNEL_VIF1, 1, 1, q - spec_upload - 1);

    if ((pipeCode & VU1Stage1) != 0)
    {
        if ((drawCode & DRAW_MORPH) != 0)
            q = CreateMorphInterpolatorDMAUpload(q, q + 1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, spec_vu1);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

    // dump_packet(pipeline->q);

    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}
