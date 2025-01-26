#include "pipelines/ps_pipelines.h"
#include "graphics/ps_drawing.h"
#include "graphics/ps_renderdirect.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <draw2d.h>
#include <draw3d.h>

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
#include "graphics/ps_drawing.h"
#include "system/ps_spr.h"

#include <malloc.h>

#if 0 

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

    CreateSizesFromRenderFlags(obj->renderState.properties, &pipeCode, &drawCode, &renderPasses);

    CreatePipelineSizes(pipeCode | VU1Stage2, &cbsNums, &headerSize); //

    cbsNums+=2;

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    //  DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

    // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    uploadLoop = ((obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount) / drawSize) + 1;

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

    CreateSizesFromRenderFlags(obj->renderState.properties, &pipeCode, &drawCode, &renderPasses);

    if (msize == 0)
    {
        ERRORLOG("no materials. must be at least 1");
        return;
    }

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //
    cbsNums += 4;
    totalHeader = headerSize;
    if (obj->vertexBuffer.meshAnimationData && ((drawCode & DRAW_SKINNED) != 0))
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

    uploadLoop = ((obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount) / drawSize) + 1;

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

    CreateSizesFromRenderFlags(obj->renderState.properties, &pipeCode, &drawCode, &renderPasses);

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

#endif

static void EndAndCopy(VU1Pipeline *pipeline, qword_t *begin, GameObject *obj);

static void BasePipelineCommand(VU1Pipeline *pipeline, GameObject *obj, VertexType type)
{
    u32 matCount = obj->vertexBuffer.matCount;
    MeshVectors *buffer = obj->vertexBuffer.meshData[MESHTRIANGLES];
    u32 count = buffer->vertexCount;
    u32 start = 0, end = count-1;
    LinkedList *matIter = obj->vertexBuffer.materials;
    ObjectProperties *props = &obj->renderState.properties;
    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    pipeline->passes[0]->target = &obj->vertexBuffer;

    DetermineVU1Programs(props, &pipeline->passes[0]->programs);

    int base = 0;
    
    int headerSize = DrawHeaderSize(obj, &base);

    BeginCommand();
    qword_t *begin = GetGlobalDrawPointer();
    if (matCount) { 
        matIter = LoadMaterial(matIter, false, &start, &end);
    }
    ShaderHeaderLocation(headerSize);
    for(int i = 0; i<4; i++) ShaderProgram(pipeline->passes[0]->programs.sw[i], i);
    PipelineCallback *SetupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters , GetGlobalDrawPointer()-begin, NULL, DEFAULT_GS_REG_PCB);        
    pipeline = AddPipelineCallbackNode(pipeline, SetupGSRegs);
    DepthTest(obj->renderState.properties.Z_ENABLE, obj->renderState.properties.Z_TYPE);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    AllocateShaderSpace(base-8, 8);
    PipelineCallback *SetupVU1Header; 
    SetupVU1Header = CreateVU1WriteCBNode(SetupPerObjDrawVU1Header, GetVIFHeaderUpload()-begin, GetTopHeaderSize(), GetSplitHeaderUpload()-begin, DEFAULT_VU1_HEADER_PCB);
    pipeline = AddPipelineCallbackNode(pipeline, SetupVU1Header);

    BindMatrix(g_DrawCamera->viewProj, 0, false);
    BindMatrix(obj->world, 4, false);

    

    if (props->ANIMATION_TEXUTRE || props->ENVIRONMENTMAP)
    {
        PipelineCallback *setupStage2Mat = CreatePipelineCBNode(SetupStage2MATVU1, GetGlobalDrawPointer()-begin, NULL, DEFAULT_STAGE2_MATRIX_PCB);
        BindMatrix(NULL, VU1_LOCATION_UV_TRANSFORM, false);
        pipeline = AddPipelineCallbackNode(pipeline, setupStage2Mat);
    }

    if (props->LIGHTING_ENABLE)
    {
        PipelineCallback *setupLightsHeader = CreatePipelineCBNode(SetupPerObjLightBuffer, GetVIFHeaderUpload()-begin, NULL, LIGHT_BUFFER_PCB);
    }

    if (V_SKINNED & type)
    {
        PushInteger(base, 11, 0);
        BindVectors(obj->vertexBuffer.meshAnimationData->finalBones, obj->vertexBuffer.meshAnimationData->jointsCount * 3, 0, base);
    }

    int max = MaxUploadSize(type, headerSize, obj->renderState.gsstate.gs_reg_count, obj->renderState.properties.CLIPPING);

    
    UploadBuffers(start, end, max, buffer, type);
    for (int i = 1; i<matCount; i++)
    {
       matIter = LoadMaterial(matIter, false, &start, &end);
       UploadBuffers(start, end, max, buffer, type);
    }  
}

void CreateAlphaMapPipeline(GameObject *obj, const char *name)
{
    ObjectProperties *props = &obj->renderState.properties;
    
    VertexType type = GetVertexType(props);
    
    VU1Pipeline *pipeline = CreateVU1Pipeline(name, DetermineCallbackCount(props), 4);
    u32 matCount = obj->vertexBuffer.matCount;
    MeshVectors *buffer = obj->vertexBuffer.meshData[MESHTRIANGLES];
    u32 count = buffer->vertexCount;
    u32 start = 0, end = count-1;
    LinkedList *matIter = obj->vertexBuffer.materials;
    qword_t *vu1_addr = &pipeline->passes[0]->programs;
    vu1_addr->sw[0] = vu1_addr->sw[1] = vu1_addr->sw[2] = vu1_addr->sw[3] = MAX_VU1_CODE_ADDRESS;

    pipeline->passes[0]->target = &obj->vertexBuffer;

    DetermineVU1Programs(props, &pipeline->passes[0]->programs);

    int base = 0;
    
    int headerSize = DrawHeaderSize(obj, &base);

    BeginCommand();
    qword_t *begin = GetGlobalDrawPointer();
    if (matCount) { 
        matIter = LoadMaterial(matIter, false, &start, &end);
    }
    ShaderHeaderLocation(headerSize);
    for(int i = 0; i<4; i++) ShaderProgram(pipeline->passes[0]->programs.sw[i], i);
    PipelineCallback *SetupGSRegs = CreatePipelineCBNode(SetupAlphaMapPass1, GetGlobalDrawPointer()-begin, NULL, ALPHAMAP_SETUP_GS_PCB);        
    pipeline = AddPipelineCallbackNode(pipeline, SetupGSRegs);
    Color color;
    CREATE_RGBAQ_STRUCT(color, 0, 0, 0, 0, 1.0f);
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0x00);
    PrimitiveColor(color);
    FrameBufferMaskWord(0x00ffffff);
    DepthBufferMask(true);
    AllocateShaderSpace(base-8, 8);
    PipelineCallback *SetupVU1Header; 
    SetupVU1Header = CreateVU1WriteCBNode(SetupPerObjDrawVU1HeaderAlphaMap, GetVIFHeaderUpload()-begin, GetGapCount(), GetSplitHeaderUpload()-begin, DEFAULT_VU1_HEADER_PCB);
    pipeline = AddPipelineCallbackNode(pipeline, SetupVU1Header);

    BindMatrix(g_DrawCamera->viewProj, 0, false);
    BindMatrix(obj->world, 4, false);

    int max = MaxUploadSize(V_POS | V_TEXTURE, headerSize, 3, obj->renderState.properties.CLIPPING);

    UploadBuffers(start, end, max, buffer, type);

    PipelineCallback* setupAlphaBlend = CreatePipelineCBNode(SetupAlphaMapPass2, GetGlobalDrawPointer()-begin, NULL, ALPHAMAP_RENDERPASS_TWO_PCB);        
    pipeline = AddPipelineCallbackNode(pipeline, setupAlphaBlend);
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0x00);
    PrimitiveColor(obj->renderState.color);

    UploadBuffers(start, end, max, buffer, V_POS | V_TEXTURE);

    PipelineCallback* finalpass = CreatePipelineCBNode(SetupAlphaMapPass3, GetGlobalDrawPointer()-begin, NULL, ALPHAMAP_RENDERPASS_THREE_PCB);        
    pipeline = AddPipelineCallbackNode(pipeline, finalpass);

    DepthTest(true, 2);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0x00);
    DestinationAlphaTest(true, DTEST_METHOD_PASS_ONE);
    FrameBufferMaskWord(0x00ffffff);
    DepthBufferMask(false);
    AllocateShaderSpace(1, 12);
    max = MaxUploadSize(type, headerSize, obj->renderState.gsstate.gs_reg_count, obj->renderState.properties.CLIPPING);
    for (int i = 0; i<matCount; i++)
    {
        matIter = LoadMaterial(matIter, false, &start, &end);
        UploadBuffers(start, end, max, buffer, type);
    } 

    PipelineCallback* finish = CreatePipelineCBNode(SetupAlphaMapFinish, GetGlobalDrawPointer()-begin, NULL, ALPHAMAP_RENDERPASS_FINAL_PCB);        
    pipeline = AddPipelineCallbackNode(pipeline, finish);

    FrameBufferMaskWord(0x0);

    EndAndCopy(pipeline, GetDrawBegin(), obj);
}

void CreateSpecularPipeline(GameObject *obj, const char *name)
{
    ObjectProperties *props = &obj->renderState.properties;
    
    VertexType type = GetVertexType(props);
    
    VU1Pipeline *pipeline = CreateVU1Pipeline(name, DetermineCallbackCount(props), 1);

    BasePipelineCommand(pipeline, obj, type);
    blend_t blendee;

    blendee.color1 = BLEND_COLOR_SOURCE;
    blendee.color2 = BLEND_COLOR_ZERO;
    blendee.color3 = BLEND_COLOR_DEST;
    blendee.alpha = BLEND_ALPHA_FIXED;
    blendee.fixed_alpha = 0x80;

    DepthTest(true, 2);
    BlendingEquation(&blendee);
    AllocateShaderSpace(1, 10);
    u32 matCount = obj->vertexBuffer.matCount;
    MeshVectors *buffer = obj->vertexBuffer.meshData[MESHTRIANGLES];
    u32 count = buffer->vertexCount;
    u32 start = 0, end = count-1;
  //  LinkedList *matIter = obj->vertexBuffer.materials;
    int max = MaxUploadSize(type, 36, obj->renderState.gsstate.gs_reg_count, obj->renderState.properties.CLIPPING);
    VertexType typeSpecular = V_NORMAL | V_POS;
    UploadBuffers(start, end, max, buffer, type);  
    EndAndCopy(pipeline, GetDrawBegin(), obj);
}


void CreateGraphicsPipeline(GameObject *obj, const char *name)
{
    ObjectProperties *props = &obj->renderState.properties;
    
    VertexType type = GetVertexType(props);
    
    VU1Pipeline *pipeline = CreateVU1Pipeline(name, DetermineCallbackCount(props), 1);

    BasePipelineCommand(pipeline, obj, type);
    
    EndAndCopy(pipeline, GetDrawBegin(), obj);
} 

#include "util/ps_misc.h"
static void EndAndCopy(VU1Pipeline *pipeline, qword_t *begin, GameObject *obj)
{
    int size = ReturnCommand();
    pipeline->qwSize = size;
    pipeline->q = (qword_t *)memalign(64, sizeof(qword_t) * size);
    memset(pipeline->q, 0, size * 16);
    Ultimatememcpy(begin, size, pipeline->q);
    //memcpy(pipeline->q, GetDrawBegin(), size * 16);
   /** for(int i = 0; i<size; i++)
    {
       // DEBUGLOG("%d %d %d %d %d", i+1, begin[i].sw[0], begin[i].sw[1], begin[i].sw[2], begin[i].sw[3]);
    } **/
    //DEBUGLOG("size is %d", size*16);
   /* u8* ptr1 = (u8*)pipeline->q;
    u8 *ptr2 = (u8*)begin;
   for(int i = 0; i<size*16; i++)
   {
        if (*ptr1 != *ptr2)
        {
            good = i;
            break;
        }
        ptr1++;
        ptr2++;
   } */
    ClearTape(size);
    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

int DetermineCallbackCount(ObjectProperties *props)
{
    int cb = 2;
    cb += (props->ENVIRONMENTMAP || props->ANIMATION_TEXUTRE);
    cb += props->LIGHTING_ENABLE;
    cb += (props->SPECULAR);
    cb += props->MORPH_TARGET;
    return cb;
}