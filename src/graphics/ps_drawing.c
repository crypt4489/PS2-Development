#include "graphics/ps_drawing.h"
#include "dma/ps_dma.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_vector.h"
#include "system/ps_vumanager.h"
#include "pipelines/ps_vu1pipeline.h"
#include "log/ps_log.h"
#include <string.h>
static qword_t *sg_DrawBufferPtr = NULL;
static qword_t *sg_OpenDMATag = NULL;
static qword_t* sg_OpenDirectTag = NULL;
static qword_t *sg_OpenTestReg = NULL;
static qword_t *sg_VIFProgramUpload = NULL;
static qword_t *sg_VIFHeaderUpload = NULL;
static qword_t *sg_DCODEOpen = NULL;
static qword_t *sg_OpenGIFTag = NULL;
static int sg_PrimitiveType;
static int sg_reservedVU1Space;
static int sg_DrawCount;
static int sg_ShaderInUse;
static int sg_TTEUse;
static int sg_VertexType;

static void DMAOpenCheck();

static void CloseDMATag();

static void GSSetTagOpen();

static void CloseGSSetTag();

static void ResetState()
{
    
}

void ShaderProgram(int shader)
{
    sg_ShaderInUse = shader;
}

void BeginCommand()
{
    sg_TTEUse = 0;
    sg_DrawBufferPtr = InitializeDMAObject();
    sg_DCODEOpen = sg_DrawBufferPtr++;
}

void BeginCommandSet(qword_t *drawBuffer)
{
    sg_TTEUse = 0;
    sg_DrawBufferPtr = drawBuffer;
    sg_DCODEOpen = sg_DrawBufferPtr++;
}

static inline u32 GetDMACode(qword_t *q)
{
    u64 ret = (q->dw[0] & (7<<28)) >> 28;
    return ret;
}

static inline void SetDMACode(qword_t *q, u32 code)
{
    q->dw[0] &= ~(7<<28);
    q->dw[0] |= (7<<28);
}

#include "math/ps_misc.h"
qword_t* EndCommand()
{
    if (sg_VIFHeaderUpload)
        return NULL;

    if (sg_OpenDMATag)
    {
       u32 code = GetDMACode(sg_OpenDMATag);
       if (code != DMA_END)
       { 
            SetDMACode(sg_OpenDMATag, DMA_END);
            if (sg_OpenDirectTag)
            {
                CreateDirectTag(sg_OpenDirectTag, 0, 1);
            }
       }
    }

    CloseDMATag();
    CreateDCODEDmaTransferTag(sg_DCODEOpen, DMA_CHANNEL_VIF1, sg_TTEUse, 1, sg_DrawBufferPtr-sg_DCODEOpen-1);
    
    CreateDCODETag(sg_DrawBufferPtr, DMA_DCODE_END);
    SubmitDMABuffersAsPipeline(sg_DrawBufferPtr, NULL);
    
    qword_t *ret = sg_DrawBufferPtr;
    sg_DrawBufferPtr = NULL;
    return ret;
}

static void DMAOpenCheck()
{
    if (!sg_OpenDMATag)
    {
        sg_OpenDMATag = sg_DrawBufferPtr++;
        CreateDMATag(sg_OpenDMATag, DMA_CNT, 0, 0, 0, 0);
    }

    if (!sg_OpenDirectTag)
    {
        sg_OpenDirectTag = sg_DrawBufferPtr++;
        CreateDirectTag(sg_OpenDirectTag, 0, 0);
    }
}

static void CloseDMATag()
{
    if (!sg_OpenDMATag)
        return;

    AddSizeToDMATag(sg_OpenDMATag, sg_DrawBufferPtr-sg_OpenDMATag-1);

    if (sg_OpenDirectTag)
        AddSizeToDirectTag(sg_OpenDirectTag, sg_DrawBufferPtr-sg_OpenDirectTag-1);

    sg_OpenDirectTag = sg_OpenDMATag = sg_OpenTestReg = NULL;
    

    CloseGSSetTag();
}

static void CloseGSSetTag()
{
    if (sg_OpenGIFTag)
    {
        AddSizeToGSSetTag(sg_OpenGIFTag, sg_DrawBufferPtr-sg_OpenGIFTag-1);
        sg_OpenGIFTag = NULL;
    }
}

static void GSSetTagOpen()
{
    if (!sg_OpenGIFTag)
    {
        sg_OpenGIFTag = sg_DrawBufferPtr++;
        CreateGSSetTag(sg_OpenGIFTag, 0, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);
    }
}

void DepthTest(int enable, int method)
{
    DMAOpenCheck();
    GSSetTagOpen();
    
    if (!sg_OpenTestReg)
    {
        sg_OpenTestReg = sg_DrawBufferPtr++;
        SetupZTestGS(sg_OpenTestReg, method, enable,
                                       0, 0, 0, 0, 0, g_Manager.gs_context);
        return;                               
    } 
   
    sg_OpenTestReg->dw[0] |= GS_SET_TEST(DRAW_ENABLE, 0, 0, 0, 0, 0, enable, method);
    

}

void DestinationAlphaTest(int enable, int method)
{

    DMAOpenCheck();
    GSSetTagOpen();

    if (!sg_OpenTestReg)
    {
        sg_OpenTestReg = sg_DrawBufferPtr++;
        SetupZTestGS(sg_OpenTestReg, 0, 0,
                                       0, 0, 0, enable, method, g_Manager.gs_context);
        return;                               
    } 
    
    sg_OpenTestReg->dw[0] |= GS_SET_TEST(DRAW_ENABLE, 0, 0, 0, enable, method, 0, 0);
    
}

void SourceAlphaTest(int framebuffer, int method, int reference)
{
    DMAOpenCheck();
    GSSetTagOpen();

    if (!sg_OpenTestReg)
    {
        sg_OpenTestReg = sg_DrawBufferPtr++;
        SetupZTestGS(sg_OpenTestReg, 0, 0,
                                       reference, method, framebuffer, 0, 0, g_Manager.gs_context);
        return;
    } 
    
    sg_OpenTestReg->dw[0] |= GS_SET_TEST(DRAW_ENABLE, method, reference, framebuffer, 0, 0, 0, 0);
    
}

void PrimitiveType(int primitive)
{
    sg_PrimitiveType = primitive;
}

void VertexType(int vertextype)
{

}

void FrameBufferMaskWord(u32 mask)
{
    DMAOpenCheck();
    GSSetTagOpen();
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void FrameBufferMask(int red, int green, int blue, int alpha)
{
    DMAOpenCheck();
    GSSetTagOpen();
    int mask = ((alpha << 24) | (blue << 16) | (green << 8) | red); 
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void DepthBufferMask(int enable)
{
    DMAOpenCheck();
    GSSetTagOpen();
    sg_DrawBufferPtr = SetZBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->z, enable, g_Manager.gs_context);
}

void PushScaleVector()
{
    if (!sg_VIFHeaderUpload)
        return;
    VIFSetupScaleVector(sg_VIFHeaderUpload + 1 + VU1_LOCATION_SCALE_VECTOR);
}

void PushQWord(void *q, int offset)
{
    memcpy(sg_VIFHeaderUpload + 1 + offset, q, sizeof(qword_t));
}

void PushMatrix(float *mat, int offset, int size)
{
    memcpy(sg_VIFHeaderUpload + 1 + offset, mat, size);
}

void PushInteger(int num, int memoffset, int vecoffset)
{
    qword_t *temp = sg_VIFHeaderUpload + 1 + memoffset;
    temp->sw[vecoffset] = num;
}

void PushFloat(float num, int memoffset, int vecoffset)
{
    qword_t *temp = sg_VIFHeaderUpload + 1 + memoffset;
    ((float *)temp->sw)[vecoffset] = num;
}

void PushColor(int r, int g, int b, int a, int memoffset)
{
    qword_t *temp = sg_VIFHeaderUpload + 1 + memoffset;
    temp->sw[0] = r;
    temp->sw[1] = g;
    temp->sw[2] = b;
    temp->sw[3] = a;
}

void PushPairU64(u64 a, u64 b, u32 memoffset)
{
    qword_t *temp = sg_VIFHeaderUpload + 1 + memoffset;

    temp->dw[0] = a;
    temp->dw[1] = b;
}

void DrawVectorFloat(float x, float y, float z, float w)
{
    qword_t *temp = sg_DrawBufferPtr++;
    ((float *)temp->sw)[0] = x;
    ((float *)temp->sw)[1] = y;
    ((float *)temp->sw)[2] = z;
    ((float *)temp->sw)[3] = w;
}

void WritePairU64(u64 a, u64 b)
{
    qword_t *temp = sg_DrawBufferPtr++;
    temp->dw[0] = a;
    temp->dw[1] = b;
}

void DrawCount(int num, int vertexMemberCount)
{
    if (sg_OpenTestReg)
    {
        sg_OpenTestReg = NULL;
    }  

    sg_DrawCount = num;

    if (sg_VIFHeaderUpload)
    {
        sg_VIFHeaderUpload = NULL;
        sg_VIFProgramUpload = sg_DrawBufferPtr;
        sg_DrawBufferPtr = ReadUnpackData(sg_DrawBufferPtr, 0, (vertexMemberCount*num)+1, 1, VIF_CMD_UNPACK(0, 3, 0));
        sg_DrawBufferPtr->sw[3] = num;
        sg_DrawBufferPtr++;
    } else {
        CloseDMATag();
        DMAOpenCheck();
        
        PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
        sg_DrawBufferPtr++;

        PACK_GIFTAG(sg_DrawBufferPtr, GS_SET_PRIM(sg_PrimitiveType, PRIM_SHADE_FLAT, DRAW_ENABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), GS_REG_PRIM);
        sg_DrawBufferPtr++;
    
        u32 regCount = 2;

        u64 regFlag = DRAW_RGBAQ_REGLIST;

        PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_TAG(num, 1, 0, 0, GIF_FLG_REGLIST, regCount), regFlag);
        sg_DrawBufferPtr++;
    }
}

void DrawVector(VECTOR v)
{
    sg_DrawBufferPtr = VectorToQWord(sg_DrawBufferPtr, v);
}

void DrawColor(Color c)
{
    sg_DrawBufferPtr->sw[0] = c.r;
    sg_DrawBufferPtr->sw[1] = c.g;
    sg_DrawBufferPtr->sw[2] = c.b;
    sg_DrawBufferPtr->sw[3] = c.a;
    sg_DrawBufferPtr++;
}

void DrawVertices()
{
    if (sg_VIFProgramUpload)
    {
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, 0, VIF_CODE(GetProgramAddressVU1Manager(g_Manager.vu1Manager, sg_ShaderInUse), 0, VIF_CMD_MSCAL, 0), 0);
        sg_OpenDMATag = sg_DrawBufferPtr;
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);
        sg_VIFProgramUpload = NULL;
    } else {
        CloseDMATag();
    }
}

void ShaderHeaderLocation(int location)
{   
    sg_DrawBufferPtr = InitDoubleBufferingQWord(sg_DrawBufferPtr, location, GetDoubleBufferOffset(location));
}

void AllocateShaderSpace(int size, int offset)
{
    CloseDMATag();
    sg_TTEUse = 1;
    sg_reservedVU1Space = size;
    if (!sg_VIFHeaderUpload)
    {
        sg_VIFHeaderUpload = sg_DrawBufferPtr++;
        ReadUnpackData(sg_VIFHeaderUpload, offset, size, 0, VIF_CMD_UNPACK(0, 3, 0));
        sg_DrawBufferPtr += size;
    } 
}

