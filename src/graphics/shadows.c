#include "graphics/shadows.h"

#include <stdlib.h>
#include <malloc.h>
#include <draw2d.h>
#include <draw3d.h>

#include "pipelines/ps_pipelineinternal.h"
#include "gameobject/ps_gameobject.h"
#include "pipelines/ps_vu1pipeline.h"
#include "dma/ps_dma.h"
#include "gs/ps_gs.h"
#include "system/ps_vumanager.h"
#include "textures/ps_texture.h"
#include "system/ps_vif.h"
#include "pipelines/ps_pipelinecbs.h"

void DrawQuad(int height, int width, int xOffset, int yOffset, u8 blend, Texture *shadowTex)
{
    UploadTextureViaManagerToVRAM(shadowTex);
    qword_t *ret = InitializeDMAObject();

    // u64 reglist = ((u64)DRAW_UV_REGLIST) << 8 | DRAW_UV_REGLIST;

    qword_t *dcode_tag_vif1 = ret;
    ret++;

    blend_t blender;

    u8 red, green, blue, alpha;

    red = green = blue = 0xFF;

    alpha = 0x80;

    if (blend)
    {
        ret = CreateDMATag(ret, DMA_CNT, 4, 0, 0, 0);

        ret = CreateDirectTag(ret, 3, 0);

        ret = CreateGSSetTag(ret, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);
        CREATE_ALPHA_REGS(blender, BLEND_COLOR_DEST, BLEND_COLOR_ZERO, BLEND_COLOR_ZERO, BLEND_ALPHA_DEST, 0xFF);

        ret = SetupAlphaGS(ret, &blender, g_Manager.gs_context);
    }
    else
    {
        ret = CreateDMATag(ret, DMA_CNT, 3, 0, 0, 0);

        ret = CreateDirectTag(ret, 2, 0);

        ret = CreateGSSetTag(ret, 1, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);
    }

    ret = SetupZTestGS(ret, 1, 1, 0xFF, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    qword_t *dmatag = ret;
    ret++;
    qword_t *direct = ret;

    ret++;
    PACK_GIFTAG(ret, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    ret++;

    PACK_GIFTAG(ret, GS_SET_PRIM(PRIM_TRIANGLE_STRIP, PRIM_SHADE_FLAT, DRAW_ENABLE, DRAW_DISABLE, blend != 0, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), GS_REG_PRIM);
    ret++;

    u32 regCount = 3;

    u64 regFlag = regCount == 3 ? DRAW_RGBAQ_UV_REGLIST : ((u64)DRAW_UV_REGLIST) << 8 | DRAW_UV_REGLIST;

    PACK_GIFTAG(ret, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_REGLIST, regCount), regFlag);
    ret++;

    int u0 = ((0 + xOffset) << 4);
    int v0 = ((0 + yOffset) << 4);

    int u1 = ((shadowTex->width + xOffset) << 4);
    int v1 = ((shadowTex->height + yOffset) << 4);

    PACK_GIFTAG(ret, GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u0, v0));
    ret++;

    PACK_GIFTAG(ret, GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, -), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));
    ret++;
    PACK_GIFTAG(ret, GIF_SET_UV(u0, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    ret++;
    PACK_GIFTAG(ret, GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u1, v0));

    ret++;
    PACK_GIFTAG(ret, GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, -), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));
    ret++;
    PACK_GIFTAG(ret, GIF_SET_UV(u1, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, +), 0xFFFFFF));

    ret++;

    CreateDMATag(dmatag, DMA_END, ret - dmatag - 1, 0, 0, 0);

    CreateDirectTag(direct, ret - direct - 1, 1);

    u32 sizeOfPipeline = ret - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 0, 1, sizeOfPipeline);

    CreateDCODETag(ret, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(ret, NULL);
}

void CreateShadowMapVU1Pipeline(GameObject *obj, u32 programNumber, u32 qwSize)
{
    u32 sizeOfPipeline;

    qword_t *pipeline_dma = (qword_t *)malloc(sizeof(qword_t) * qwSize);

    VU1Pipeline *pipeline = CreateVU1Pipeline("GENERIC_SHADOW_MAP", 3, 1);

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

    PipelineCallback *setupGSRegs = CreatePipelineCBNode(SetupPerObjDrawShadowRegisters, q, NULL, 0x90);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupGSRegs, dcode_callback_tags, q);

    q += 2; //(obj-> tex == NULL ? 4 : 14);

    q = ReadUnpackData(q, 0, 16, 0, VIF_CMD_UNPACK(0, 3, 0));

    PipelineCallback *setupMVPHeader = CreatePipelineCBNode(SetupPerObjMVPMatrix, q, NULL, DEFAULT_OBJ_WVP_PCB);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupMVPHeader, dcode_callback_tags, q);

    PipelineCallback *setupVU1Header = CreatePipelineCBNode(SetupPerObjDrawShadowVU1Header, q, NULL, 0x91);

    dcode_callback_tags = AddPipelineCallbackNodeQword(pipeline, setupVU1Header, dcode_callback_tags, q);

    q += 16;

    qword_t vu1_addr;
    vu1_addr.sw[0] = vu1_addr.sw[1] = vu1_addr.sw[2] = 0;

    vu1_addr.sw[3] = GetProgramAddressVU1Manager(g_Manager.vu1Manager, programNumber);

    q = CreateVU1VertexUpload(q, &obj->vertexBuffer, 0, obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, 81, DRAW_VERTICES, &vu1_addr);

    sizeOfPipeline = q - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 1, 1, sizeOfPipeline);

    CreateDCODETag(q, DMA_DCODE_END);

    pipeline->q = pipeline_dma;
    // AddPipelineCallbackNode(pipeline, setupGSRegs);
    // AddPipelineCallbackNode(pipeline, setupVU1Header);

    AddVU1Pipeline(obj, pipeline);
    // SetActivePipeline(obj, pipeline);
}
void SetupPerObjDrawShadowRegisters(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
    qword_t *q = pipeline_loc;
    Color black;
    black.r = 0;
    black.g = 0;
    black.b = 0;
    black.a = 255;
    black.q = 1.0f;
    // q = vif_set_z_test(q, obj->renderState.state.render_state.Z_TYPE, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context); // 4
    // q = vif_setup_rgbaq(q, black);

    q = SetupZTestGS(q, obj->renderState.state.render_state.Z_TYPE, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);
    q = SetupRGBAQGS(q, black);
}
void SetupPerObjDrawShadowVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
    qword_t *wvp_screen = pipeline_loc;

    wvp_screen += VU1_LOCATION_SCALE_VECTOR;

    wvp_screen = VIFSetupScaleVector(wvp_screen);

    wvp_screen->sw[0] = (int)0;
    wvp_screen->sw[1] = (int)0;
    wvp_screen->sw[2] = (int)0;
    wvp_screen->sw[3] = (int)255;
    wvp_screen++;

    wvp_screen->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, 0, 0, 0, 0, obj->renderState.prim.mapping_type, g_Manager.gs_context, obj->renderState.prim.colorfix), 0, 2);
    wvp_screen->dw[1] = DRAW_RGBAQ_REGLIST;
}
