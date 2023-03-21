#include "ps_pipelines.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>


#include "ps_gameobject.h"
#include "ps_dma.h"
#include "ps_vu1pipeline.h"
#include "ps_vumanager.h"
#include "ps_gs.h"
#include "ps_misc.h"
#include "ps_pipelinecbs.h"
#include "ps_vif.h"
#include "ps_log.h"


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

static u32 CreateUpload(LinkedList *materials, u32 drawSize, u32 msize)
{
    LinkedList *node = materials;

    u32 uploadLoop = 0;

    for (u32 index = 0; index<msize; index++, node = node->next)
    {
        Material *mat = (Material *)node->data;

       // DEBUGLOG("%d %d %d", mat->start, mat->end, mat->materialId);
        uploadLoop += (((mat->end - mat->start) + 1) / drawSize) + 1;
    }

    return uploadLoop;
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

  PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

  dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

  q += 2; //(obj-> tex == NULL ? 4 : 14);

  q = read_unpack_data(q, 0, 16, 0, VIF_CMD_UNPACK(0, 3, 0));

  qword_t *targ = q;

  PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawWireframeVU1Header, targ, NULL);

  dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, dcode_callback_tags, targ);

  PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, targ, NULL);

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

  PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

  dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

  q += 2; //(obj-> tex == NULL ? 4 : 14);

  q = read_unpack_data(q, 0, 16, 0, VIF_CMD_UNPACK(0, 3, 0));

  PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawVU1Header, q, NULL);

   dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, dcode_callback_tags, q);

  PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, q, NULL);

  dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, dcode_callback_tags, q);

  PipelineCallback *setupTessGrid = CreatePipelineCBNode(SetupPerObjDrawTessVU1Header, q, (void *)grid);

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

void CreateClipperGraphicsPipeline(GameObject *obj, const char* name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2)
{
    u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = 1;

    u32 renderPasses = 1;

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    drawSize = 27;

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

   // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);    

    DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = pipeline->programs[0];
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

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

   dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

    


    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;


    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

   // CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode, stg2, stg1);

    vu1_addr->sw[0] = GetProgramAddressVU1Manager(g_Manager.vu1Manager, 0);
    vu1_addr->sw[1] = 0;
    vu1_addr->sw[2] = 0;
    vu1_addr->sw[3] = GetProgramAddressVU1Manager(g_Manager.vu1Manager, stg1);
    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }
    

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, 1, vu1_addr);

    

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

   // dump_packet(pipeline->q);


    AddVU1Pipeline(obj, pipeline);
}

void CreateGraphicsPipeline(GameObject *obj, const char* name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2)
{
    u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 1;

    if (msize == 0)
    {
        ERRORLOG("bounce out no materials");
        return;
    }

    CreatePipelineSizes(pipeCode, &cbsNums, &headerSize); //

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode, &drawSize);

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

   // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);    

    DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = pipeline->programs[0];
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

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

   dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

    


    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;


    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode, stg2, stg1);

    

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
    if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }
    

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

   // dump_packet(pipeline->q);


    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}






qword_t *CreateVU1Callbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerSize, u32 pCode, u32 dCode, ...)
{
    va_list cbs_args;
    va_start(cbs_args, dCode);

    PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawVU1Header, q, NULL);

    tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, tag, q);

    PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, q, NULL);

    tag = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, tag, q);

    if ((pCode & VU1Stage3) != 0)
    {
        PipelineCallback *setupLightsHeader = CreatePipelineCBNode(SetupPerObjLightBuffer, q, NULL);

        tag = AddPipelineCallbackNodeQword(pipeline, setupLightsHeader, tag, q);
    }

    if ((pCode & VU1Stage2) != 0)
    {
        float* matrix = va_arg(cbs_args, float *);
        PipelineCallback *setupStage2Mat = CreatePipelineCBNode(SetupStage2MATVU1, q, matrix);
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
            
        }
    }



    va_end(cbs_args);
    return tag;
}

void CreateVU1ProgramsList(qword_t *q, u32 pipeCode, u16 drawCode, u16 stg2PrgNum, u16 stg1PrgNum)
{
    u32 stage4, stage3, stage2, stage1;
    stage4 = stage3 = stage2 = stage1 = 0;
  
    stage4 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericStage4);

    q->sw[3] = stage4;
    if ((pipeCode & VU1Stage3) != 0)
    {
        if ((drawCode & DRAW_SPECULAR) != 0 )
        {
            stage3 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericSpecular);
        } else {
            stage3 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, VU1GenericLight3D);
        }
        q->sw[3] = stage3;
        q->sw[2] = stage4;
    }

    if ((pipeCode & VU1Stage2) != 0)
    {
        stage2 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, stg2PrgNum);
        q->sw[3] = stage2;
        q->sw[1] = ((pipeCode & VU1Stage3) != 0 ? stage3 : stage4);
    }

    if ((pipeCode & VU1Stage1) != 0)
    {
        stage1 = GetProgramAddressVU1Manager(g_Manager.vu1Manager, stg1PrgNum);
        q->sw[3] = stage1;
        q->sw[0] = ((pipeCode & VU1Stage2) != 0 ? stage2 : (pipeCode & VU1Stage3) != 0 ? stage3
                                                                                       : stage4);
    }
}

void CreateEnvMapPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2, Texture *envMap, MATRIX envMatrix)
{
     u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 2;

    if (msize == 0)
    {
        ERRORLOG("bounce out no materials");
        return;
    }

    CreatePipelineSizes(pipeCode | VU1Stage2, &cbsNums, &headerSize); //

    cbsNums++;

    u32 upload = CreateDrawSizeandUploadCount(drawCode, pipeCode | VU1Stage2, &drawSize);

    // DEBUGLOG("cbs and vu1headers %d %d ", cbsNums, headerSize);

   // DEBUGLOG("uppie and drawie %d %d ", upload, drawSize);

    u32 uploadLoop = CreateUpload(obj->vertexBuffer.materials, drawSize, msize);

    sizeOfPipeline = 1 + MaterialSizeDMACount(msize) + UploadSize(uploadLoop, upload) + cbsNums + headerSize + 8 + RenderPassesForAnim(renderPasses, pipeCode);

    uploadLoop = ((obj->vertexBuffer.vertexCount) / drawSize) + 1;  

    sizeOfPipeline = sizeOfPipeline +  UploadSize(uploadLoop, upload) + 1 + 3 + 10;

    DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbsNums, renderPasses);

    qword_t *vu1_addr = pipeline->programs[0];
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

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

   dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode | VU1Stage2, drawCode, envMatrix);

    

    

    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;


    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode, 0, stg1);

    

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
     if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    q = CreateLoadByIdDCODETag(q, envMap->id);

    qword_t env_vu1;

    CreateVU1ProgramsList(&env_vu1, pipeCode | VU1Stage2, drawCode, stg2, stg1);

    qword_t *envmap_upload = q;

    q++;

    

     q = CreateDMATag(q, DMA_CNT, 4, 0, 0, 0);

    q = CreateDirectTag(q, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupEnvMapReg = CreatePipelineCBNode(SetupBlendingCXT, q, NULL);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupEnvMapReg, dcode_callback_tags, q);

    q+=2;

    q = CreateDMATag(q, DMA_END, 1, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE((10 | (1 << 14) | (0 << 15)), 1, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    q++;

    CreateDCODEDmaTransferTag(envmap_upload, DMA_CHANNEL_VIF1, 1, 1, q- envmap_upload - 1); 

    if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, 0);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, DRAW_NORMAL | DRAW_VERTICES | DRAW_TEXTURE, 0, &env_vu1);

  //  q = CreateVU1VertexUpload(q, &obj->vertexBuffer, 0, obj->vertexBuffer.vertexCount-1, drawSize, DRAW_NORMAL | DRAW_VERTICES | DRAW_TEXTURE, &env_vu1);

   

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

    //dump_packet(pipeline->q);


    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}

void CreateSpecularPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2)
{
     u32 sizeOfDCode, headerSize, cbsNums, sizeOfPipeline, drawSize;
    sizeOfDCode = headerSize = cbsNums = sizeOfPipeline = drawSize = 0;

    u32 msize = obj->vertexBuffer.matCount;

    u32 renderPasses = 2;

    if (msize == 0)
    {
        ERRORLOG("bounce out no materials");
        return;
    }

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

    

    DEBUGLOG("size of pipe : %d", sizeOfPipeline);

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * sizeOfPipeline);

    VU1Pipeline *pipeline = CreateVU1Pipeline(name, cbInput, renderPasses);

    qword_t *vu1_addr = pipeline->programs[0];
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

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawRegisters, q, NULL);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14); //7

    q = CreateDMATag(q, DMA_END, headerSize, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE(0, headerSize, VIF_CMD_UNPACK(0, 3, 0), 1), 0); // 8

   dcode_callback_tags = CreateVU1Callbacks(dcode_callback_tags, q, pipeline, headerSize, pipeCode, drawCode);

 

    q += headerSize;

    sizeOfDCode = q - dcode_tag_vif1 - 1;


    // DEBUGLOG("%d", sizeOfDCode);

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfDCode);

    CreateVU1ProgramsList(vu1_addr, pipeCode, drawCode, 0, stg1);

    

    // vu1upload =  headerSize + 8

    // matQword = matCount * 3 + (((vertexCount / drawSize) + 1) * (uploadCount + 2)

    // total qwords cbsNums + vu1upload + matQword
     if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }

    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, vu1_addr);

    qword_t *env_vu1 = pipeline->programs[1];

    CreateVU1ProgramsList(env_vu1, pipeCode, drawCode | DRAW_SPECULAR, stg2, stg1);
    //env_vu1.sw[3] = GetProgramAddressVU1Manager(g_Manager.vu1Manager, 7);
    

    qword_t *spec_upload = q;

    q++;


    

    q = CreateDMATag(q, DMA_CNT, 4, 0, 0, 0);

    q = CreateDirectTag(q, 3, 0);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PipelineCallback *setupEnvMapReg = CreatePipelineCBNode(SetupBlendingCXT, q, NULL);
    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupEnvMapReg, dcode_callback_tags, q);

    q += 2;

    q = CreateDMATag(q, DMA_END, 1, VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0), VIF_CODE((10 | (1 << 14) | (0 << 15)), 1, VIF_CMD_UNPACK(0, 3, 0), 1), 0);

    q++;

    CreateDCODEDmaTransferTag(spec_upload, DMA_CHANNEL_VIF1, 1, 1, q- spec_upload - 1);

    if ((pipeCode & VU1Stage1) != 0)
    {
        q = CreateMorphInterpolatorDMAUpload(q, q+1, pipeline, obj->interpolator->currInterpNode, obj->vertexBuffer.matCount);
    }
    
    q = CreateMeshDMAUpload(q, obj, drawSize, drawCode, obj->vertexBuffer.matCount, env_vu1);


    
    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;

   // dump_packet(pipeline->q);


    AddVU1Pipeline(obj, pipeline);
    SetActivePipeline(obj, pipeline);
}
