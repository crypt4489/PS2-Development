#include "graphics/ps_drawing.h"
#include "dma/ps_dma.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_vector.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "pipelines/ps_vu1pipeline.h"
#include "log/ps_log.h"
#include "textures/ps_texture.h"
#include <string.h>
static qword_t *sg_DrawBufferPtr = NULL;
static qword_t *sg_OpenDMATag = NULL;
static qword_t* sg_OpenDirectTag = NULL;
static qword_t *sg_OpenTestReg = NULL;
static qword_t *sg_VIFProgramUpload = NULL;
static qword_t *sg_VIFHeaderUpload = NULL;
static qword_t *sg_OpenGIFTag = NULL;
static qword_t *sg_VIFCodeUpload = NULL;
static qword_t *sg_OpenTextureGap = NULL;
static qword_t *sg_VU1ProgramEnd = NULL;
static qword_t *sg_GifDrawPtr;
static bool VU1Sync;
static int sg_reservedVU1Space;
static int sg_DrawCount;
static int sg_ShaderInUse;
static bool sg_TTEUse;
static int sg_VertexType;
static int sg_RegisterCount;
static int sg_RegisterType;
static u64 sg_PrimitiveType;
static int sg_VifCodeUploadCount;

static void OpenDMATag();

static void CloseDMATag();

static void GSSetTagOpen();

static void CloseGSSetTag();

void ResetState() {
    sg_DrawBufferPtr = NULL;
    sg_OpenDMATag = NULL;
    sg_OpenDirectTag = NULL;
    sg_OpenTestReg = NULL;
    sg_VIFProgramUpload = NULL;
    sg_VIFHeaderUpload = NULL;
    sg_OpenGIFTag = NULL;
    sg_VIFCodeUpload = NULL;
    sg_OpenTextureGap = NULL;
    sg_VU1ProgramEnd = NULL;
    sg_reservedVU1Space = 0;
    sg_DrawCount = 0;
    sg_ShaderInUse = 0;
    sg_TTEUse = false;
    sg_VertexType = 0;
    sg_RegisterCount = 0;
    sg_RegisterType = 0;
    sg_PrimitiveType = 0;
    sg_VifCodeUploadCount = 0;
    sg_GifDrawPtr = NULL;
    VU1Sync = false; 
}

void ShaderProgram(int shader)
{
    sg_ShaderInUse = shader;
}

void BeginCommand()
{
    while(PollVU1DoneProcessing(&g_Manager));
    sg_TTEUse = 0;
    sg_DrawBufferPtr = g_Manager.drawBuffers->currentvif;
}

void BeginCommandSet(qword_t *drawBuffer)
{
    sg_TTEUse = 0;
    sg_DrawBufferPtr = drawBuffer;
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
       } else if (VU1Sync) {
        
            //CloseDMATag();
            //sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_END, 0, 0, 0, 0);
       }
    }

    if (sg_VU1ProgramEnd)
    {
       
      
       if (VU1Sync)
       {
            sg_VU1ProgramEnd = CreateDMATag(sg_VU1ProgramEnd, DMA_END, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0, 0);
       } else {
            u32 code = GetDMACode(sg_VU1ProgramEnd);
            if (code != DMA_END)
            { 
                SetDMACode(sg_VU1ProgramEnd, DMA_END);
            }
            sg_VU1ProgramEnd->sw[2] = VIF_CODE(0, 0, VIF_CMD_FLUSHA, 1);
       }
        
    }


    CloseDMATag();

    if (sg_GifDrawPtr)
    {
        SubmitDrawBuffersToController(sg_GifDrawPtr, DMA_CHANNEL_GIF, 1, false);
    }

    if (sg_DrawBufferPtr)
    {
      // dump_packet(g_Manager.drawBuffers->currentvif, 250, 0);
        SubmitDrawBuffersToController(sg_DrawBufferPtr, DMA_CHANNEL_VIF1, 1, sg_TTEUse);
    }
    
    qword_t *ret = sg_DrawBufferPtr;
    ResetState();
    return ret;
}

void PrintOut()
{
    qword_t *dump = g_Manager.drawBuffers->currentvif;
    dump_packet(dump, 250, 0);
}

static void OpenDMATag()
{
    u32 arg1 = 0, arg2 = 0;
    if (!sg_OpenDMATag)
    {
        sg_VU1ProgramEnd = NULL;
        sg_OpenDMATag = sg_DrawBufferPtr;
        if (sg_OpenTextureGap)
        {
            VU1Sync = true;
            arg1 = 0;
            arg2 = VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0);
            
        }
       
        sg_DrawBufferPtr = CreateDMATag(sg_OpenDMATag, DMA_CNT, 0, arg1, arg2, 0);
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

void DepthTest(bool enable, int method)
{
    OpenDMATag();
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

void DestinationAlphaTest(bool enable, int method)
{

    OpenDMATag();
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
    OpenDMATag();
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

void PrimitiveType(u64 primitive)
{
    sg_PrimitiveType = primitive;
}
void PrimitiveTypeStruct(prim_t prim)
{
    sg_PrimitiveType = GS_SET_PRIM(prim.type, 
    prim.shading, prim.mapping, prim.fogging, 
    prim.blending, prim.antialiasing, prim.mapping_type, g_Manager.gs_context, prim.colorfix);
}

void VertexType(int vertextype)
{

}

void FrameBufferMaskWord(u32 mask)
{
    OpenDMATag();
    GSSetTagOpen();
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void FrameBufferMask(int red, int green, int blue, int alpha)
{
    OpenDMATag();
    GSSetTagOpen();
    int mask = ((alpha << 24) | (blue << 16) | (green << 8) | red); 
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void DepthBufferMask(bool enable)
{
    OpenDMATag();
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

void BlendingEquation(blend_t *blend)
{
    PACK_GIFTAG(sg_DrawBufferPtr, GS_SET_ALPHA(blend->color1, blend->color2, blend->alpha, blend->color3, blend->fixed_alpha), GS_REG_ALPHA + g_Manager.gs_context);
	sg_DrawBufferPtr++;
}

void PrimitiveColor(Color c)
{
    PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_RGBAQ(c.r, c.g, c.b, c.a, (int)c.q), GIF_REG_RGBAQ);
    sg_DrawBufferPtr++;
}

void SetRegSizeAndType(u64 size, u64 type)
{
    sg_RegisterCount = size;
    sg_RegisterType = type;
}

void DrawCount(int num, int vertexMemberCount, bool toVU)
{
    sg_DrawCount = num;
    

    if (toVU)
    {
        
        if (!sg_OpenDMATag && sg_OpenTextureGap) {
            sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, 0, VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), 0);
            VU1Sync = true;
        } else 
            CloseDMATag();
    
        sg_VIFHeaderUpload = NULL;
        sg_VIFCodeUpload = NULL;
        sg_OpenTextureGap = NULL;
        sg_VIFProgramUpload = sg_DrawBufferPtr;
        sg_DrawBufferPtr = ReadUnpackData(sg_DrawBufferPtr, 0, (vertexMemberCount*num)+1, 1, VIF_CMD_UNPACK(0, 3, 0));
        sg_DrawBufferPtr->sw[3] = num;
        sg_DrawBufferPtr++;
        sg_TTEUse = true;
    } else {
        sg_VIFCodeUpload = NULL;
        sg_OpenTextureGap = NULL;
        sg_OpenDMATag->sw[2] = VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0);
        PACK_GIFTAG(sg_DrawBufferPtr, sg_PrimitiveType, GS_REG_PRIM);
        sg_DrawBufferPtr++;
        CloseGSSetTag();
        PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_TAG(num, 1, 0, 0, GIF_FLG_REGLIST, sg_RegisterCount), sg_RegisterType);
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

void StartVertexShader()
{
    if (sg_VIFProgramUpload)
    {
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), VIF_CODE(GetProgramAddressVU1Manager(g_Manager.vu1Manager, sg_ShaderInUse), 0, VIF_CMD_MSCAL, 0), 0);        
        sg_VU1ProgramEnd = sg_DrawBufferPtr;
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 0), 0, 0);    
        sg_VIFProgramUpload = NULL;
    }
}

void ShaderHeaderLocation(int location)
{   
    if (sg_VIFCodeUpload)
    {
        sg_VIFCodeUpload->sw[2] = VIF_CODE(location, 0, VIF_CMD_BASE, 0);
        sg_VIFCodeUpload->sw[3] = VIF_CODE(GetDoubleBufferOffset(location), 0, VIF_CMD_OFFSET, 0);
        sg_VIFCodeUpload++;
        sg_VifCodeUploadCount++;
        return;
    }
    sg_DrawBufferPtr = InitDoubleBufferingQWord(sg_DrawBufferPtr, location, GetDoubleBufferOffset(location));
}

void AllocateShaderSpace(int size, int offset)
{
    
    sg_TTEUse = true;
    sg_reservedVU1Space = size;

    if (!sg_VIFHeaderUpload && sg_OpenTextureGap)
    {   
        int diff = 24-(size+1);
        if (diff <= sg_VifCodeUploadCount)
        {
            ERRORLOG("too many vif code and too much geometry unpack");
        }
        CreateDMATag(sg_OpenTextureGap, DMA_CNT, diff, 0, VIF_CODE(0x0000, 0, VIF_CMD_MSKPATH3, 0), 0);
        sg_VIFHeaderUpload = sg_OpenTextureGap + diff + 1;
        ReadUnpackData(sg_VIFHeaderUpload, offset, size, 0, VIF_CMD_UNPACK(0, 3, 0));
    }
    else if (!sg_VIFHeaderUpload)
    {
        CloseDMATag();
        sg_VIFHeaderUpload = sg_DrawBufferPtr++;
        ReadUnpackData(sg_VIFHeaderUpload, offset, size, 0, VIF_CMD_UNPACK(0, 3, 0));
        sg_DrawBufferPtr += size;
    } 
}

void BindTexture(Texture *tex, bool end)
{
    if (!sg_GifDrawPtr)
        sg_GifDrawPtr = g_Manager.drawBuffers->currentgif;
    
    sg_GifDrawPtr = CreateTextureUploadChain(tex, sg_GifDrawPtr, false, end);

    CloseDMATag();

    sg_TTEUse = true;

    if (sg_VU1ProgramEnd) {
        sg_DrawBufferPtr = sg_VU1ProgramEnd;
        sg_VU1ProgramEnd = NULL;
    }

    sg_OpenTextureGap = sg_DrawBufferPtr;
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 24, 0, VIF_CODE(0x0000, 0, VIF_CMD_MSKPATH3, 0), 0);
    sg_VIFCodeUpload = sg_DrawBufferPtr;
    memset(sg_DrawBufferPtr, 0, sizeof(qword_t)*24);
    sg_DrawBufferPtr += 24;
}
