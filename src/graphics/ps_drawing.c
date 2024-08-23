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
static qword_t *sg_VIFDirectDraw = NULL;
static qword_t *sg_SplitHeaderUpload = NULL;
static qword_t *sg_OpenGIFTag = NULL;
static qword_t *sg_VIFCodeUpload = NULL;
static qword_t *sg_OpenTextureGap = NULL;
static qword_t *sg_VU1ProgramEnd = NULL;
static qword_t *sg_GifDrawPtr = NULL;
static int sg_ShaderInUse[4];
static bool sg_TTEUse;
static int sg_RegisterCount;
static int sg_RegisterType;
static u64 sg_PrimitiveType;
static int sg_VifCodeUploadCount;
static int sg_GapCount;
static int sg_VU1LoadOffset;

static inline void AddVIFCode(u32 a1, u32 a2);

static inline void OpenDMATag();

static inline void CloseDMATag(bool end);

static inline void OpenGSSetTag();

static inline void CloseGSSetTag();

static inline void FlushProgram(bool end);

static inline qword_t* ChangeGapSize();

static inline qword_t* BindLocation();

static inline qword_t *DetermineVIFHeader(int *offset);

static inline void InitProgramUpload(int vertexNum, int unpacksize);

qword_t *GetVIFHeaderUpload()
{
    return sg_VIFHeaderUpload;
}

qword_t *GetSplitHeaderUpload()
{
    return sg_SplitHeaderUpload;
}

u32 GetGapCount()
{
    return sg_GapCount;
}

void DispatchDrawBuffers()
{
    if (sg_GifDrawPtr)
    {
        SubmitDrawBuffersToController(sg_GifDrawPtr, DMA_CHANNEL_GIF, 1, false);
    }
   // PrintOut();
    if (sg_DrawBufferPtr)
    {
        SubmitDrawBuffersToController(sg_DrawBufferPtr, DMA_CHANNEL_VIF1, 1, sg_TTEUse);
    }
}

void ResetState() {
    sg_DrawBufferPtr = NULL;
    sg_GifDrawPtr = NULL;
    sg_OpenDMATag = NULL;
    sg_OpenDirectTag = NULL;
    sg_OpenTestReg = NULL;
    sg_VIFProgramUpload = NULL;
    sg_VIFHeaderUpload = NULL;
    sg_OpenGIFTag = NULL;
    sg_VIFCodeUpload = NULL;
    sg_OpenTextureGap = NULL;
    sg_VU1ProgramEnd = NULL;
    sg_VIFDirectDraw = NULL;
    for(int i = 0; i<4; i++) sg_ShaderInUse[i] = 0;
    sg_TTEUse = false;
    sg_RegisterCount = 0;
    sg_RegisterType = 0;
    sg_PrimitiveType = 0;
    sg_VifCodeUploadCount = 0;
    sg_GapCount = 0;
    sg_VU1LoadOffset = 0;
}

void ShaderProgram(int shader, int slot)
{
    sg_ShaderInUse[slot] = shader;
}

void BeginCommand()
{
    sg_DrawBufferPtr = g_Manager.drawBuffers->currentvif;
}

void BeginCommandSet(qword_t *drawBuffer)
{
    sg_DrawBufferPtr = drawBuffer;
}

static inline u32 GetDMACode(qword_t *q)
{
    return (q->sw[0] & (7<<28)) >> 28;
}

static inline void SetDMACode(qword_t *q, u32 code)
{
    q->sw[0] = (q->sw[0] & ~(7<<28)) | (code<<28);
}

static inline int GetOffsetIntoHeader(int offset)
{
    if (offset < sg_VU1LoadOffset)
    {
        ERRORLOG("Pushing with too high offset %d %d", offset, sg_VU1LoadOffset);
        return -1;
    }
    return offset - sg_VU1LoadOffset;
}

int CloseCommand()
{
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_RET, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0, 0);
    qword_t *ret = sg_DrawBufferPtr;
    ResetState();
    return ret - g_Manager.drawBuffers->currentvif;
}

qword_t* SubmitCommand()
{

    FlushProgram(true);

    CloseDMATag(true);

    DispatchDrawBuffers();
    
    qword_t *ret = sg_DrawBufferPtr;
    ResetState();
    return ret;
}

void PrintOut()
{
    #include "math/ps_misc.h"
    qword_t *dump = g_Manager.drawBuffers->currentvif;
    dump_packet(dump, 250, 0);
}

static inline void OpenDMATag()
{
    if (sg_OpenDMATag)
        return;

    FlushProgram(false);

    u32 arg1 = 0, arg2 = 0;
    
    sg_OpenDMATag = sg_DrawBufferPtr;

    if (sg_OpenTextureGap)
    {
        arg1 = VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0);
        arg2 = VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0);
    }
       
    sg_DrawBufferPtr = CreateDMATag(sg_OpenDMATag, DMA_CNT, 0, arg1, arg2, 0);
    sg_OpenDirectTag = sg_DrawBufferPtr;
    sg_DrawBufferPtr = CreateDirectTag(sg_OpenDirectTag, 0, 0);
}

static inline void CloseDMATag(bool end)
{
    if (!sg_OpenDMATag)
        return;

    if (end) {
        SetDMACode(sg_OpenDMATag, DMA_END);
        CreateDirectTag(sg_OpenDirectTag, 0, 1);
    }

    AddSizeToDMATag(sg_OpenDMATag, sg_DrawBufferPtr-sg_OpenDMATag-1);

    AddSizeToDirectTag(sg_OpenDirectTag, sg_DrawBufferPtr-sg_OpenDirectTag-1);

    sg_OpenDirectTag = sg_OpenDMATag = sg_OpenTestReg = NULL;
    
    CloseGSSetTag();
}

static inline void CloseGSSetTag()
{
    if (!sg_OpenGIFTag)
        return;
    
    AddSizeToGSSetTag(sg_OpenGIFTag, sg_DrawBufferPtr-sg_OpenGIFTag-1);
    sg_OpenGIFTag = NULL;
}

static inline void OpenGSSetTag()
{
    if (sg_OpenGIFTag)
        return;
    
    sg_OpenGIFTag = sg_DrawBufferPtr++;
    CreateGSSetTag(sg_OpenGIFTag, 0, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);
}

void DepthTest(bool enable, int method)
{
    OpenDMATag();
    OpenGSSetTag();
    
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
    OpenGSSetTag();

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
    OpenGSSetTag();

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

void FrameBufferMaskWord(u32 mask)
{
    OpenDMATag();
    OpenGSSetTag();
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void FrameBufferMask(int red, int green, int blue, int alpha)
{
    OpenDMATag();
    OpenGSSetTag();
    int mask = ((alpha << 24) | (blue << 16) | (green << 8) | red); 
    sg_DrawBufferPtr = SetFrameBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->render, mask, g_Manager.gs_context);
}

void DepthBufferMask(bool enable)
{
    OpenDMATag();
    OpenGSSetTag();
    sg_DrawBufferPtr = SetZBufferMask(sg_DrawBufferPtr, g_Manager.targetBack->z, enable, g_Manager.gs_context);
}

void BindMatrix(MATRIX mat, int offset, bool top)
{
    qword_t *assign = BindLocation();
    UnpackAddress(assign, offset, mat, 4, top, VIF_CMD_UNPACK(0, 3, 0));
}

void BindVector(VECTOR vec, int offset, bool top)
{
    qword_t *assign = BindLocation();
    UnpackAddress(assign, offset, vec, 1, top, VIF_CMD_UNPACK(0, 3, 0));
}

void BindVectors(VECTOR *vectors, u32 count, bool top, u32 offset)
{
    qword_t *assign = BindLocation();
    UnpackAddress(assign, offset, vectors, count, top, VIF_CMD_UNPACK(0, 3, 0));
}

void BindVectorInts(VectorInt *vectors, u32 count, bool top, u32 offset)
{
    qword_t *assign = BindLocation();
    UnpackAddress(assign, offset, vectors, count, top, VIF_CMD_UNPACK(0, 3, 0));
}

void PushScaleVector()
{
    if (!sg_VIFHeaderUpload) return;
    int diff = GetOffsetIntoHeader(VU1_LOCATION_SCALE_VECTOR);
    VIFSetupScaleVector(sg_VIFHeaderUpload + 1 + diff);
}

void PushQWord(void *q, int offset)
{
    offset = GetOffsetIntoHeader(offset);
    qword_t *temp = DetermineVIFHeader(&offset);
    temp += 1 + offset;
    memcpy(temp, q, sizeof(qword_t));
}

void PushMatrix(float *mat, int offset, int size)
{
    offset = GetOffsetIntoHeader(offset);
    qword_t *temp = DetermineVIFHeader(&offset);
    temp += 1 + offset;
    memcpy(temp, mat, size);
}

void PushInteger(int num, int memoffset, int vecoffset)
{
    memoffset = GetOffsetIntoHeader(memoffset);
    qword_t *temp = DetermineVIFHeader(&memoffset);
    temp += 1 + memoffset;
    temp->sw[vecoffset] = num;
}

void PushFloat(float num, int memoffset, int vecoffset)
{
    memoffset = GetOffsetIntoHeader(memoffset);
    qword_t *temp = DetermineVIFHeader(&memoffset);
    temp += 1 + memoffset;
    ((float *)temp->sw)[vecoffset] = num;
}

void PushColor(int r, int g, int b, int a, int memoffset)
{
    memoffset = GetOffsetIntoHeader(memoffset);
    qword_t *temp = DetermineVIFHeader(&memoffset);
    temp += 1 + memoffset;
    temp->sw[0] = r;
    temp->sw[1] = g;
    temp->sw[2] = b;
    temp->sw[3] = a;
}

void PushPairU64(u64 a, u64 b, u32 memoffset)
{
    memoffset = GetOffsetIntoHeader(memoffset);
    qword_t *temp = sg_VIFHeaderUpload + 1 + memoffset;
    temp->dw[0] = a;
    temp->dw[1] = b;
}

void BlendingEquation(blend_t *blend)
{
    OpenDMATag();
    OpenGSSetTag();
    PACK_GIFTAG(sg_DrawBufferPtr, GS_SET_ALPHA(blend->color1, blend->color2, blend->alpha, blend->color3, blend->fixed_alpha), GS_REG_ALPHA + g_Manager.gs_context);
	sg_DrawBufferPtr++;
}

void PrimitiveColor(Color c)
{
    OpenDMATag();
    OpenGSSetTag();
    PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_RGBAQ(c.r, c.g, c.b, c.a, (int)c.q), GIF_REG_RGBAQ);
    sg_DrawBufferPtr++;
}

void SetRegSizeAndType(u64 size, u64 type)
{
    sg_RegisterCount = size;
    sg_RegisterType = type;
}

void DrawCountDirect(int num)
{
    OpenDMATag();
    OpenGSSetTag();    
    PACK_GIFTAG(sg_DrawBufferPtr, sg_PrimitiveType, GS_REG_PRIM);
    sg_DrawBufferPtr++;
    CloseGSSetTag();
    PACK_GIFTAG(sg_DrawBufferPtr, GIF_SET_TAG(num, 1, 0, 0, GIF_FLG_REGLIST, sg_RegisterCount), sg_RegisterType);
    sg_VIFDirectDraw = sg_DrawBufferPtr;
    sg_DrawBufferPtr++;
}

void DrawCountWrite(int num, int vertexMemberCount)
{
    u32 size = (vertexMemberCount * num);
    InitProgramUpload(num, size);
}

void DrawUpload(int num)
{
    InitProgramUpload(num, 0);    
}

void DrawVector(VECTOR v)
{
    if (!sg_VIFProgramUpload)
        return;
    sg_DrawBufferPtr = VectorToQWord(sg_DrawBufferPtr, v);
}

void DrawColor(Color c)
{
    if (!sg_VIFProgramUpload)
        return;
    sg_DrawBufferPtr->sw[0] = c.r;
    sg_DrawBufferPtr->sw[1] = c.g;
    sg_DrawBufferPtr->sw[2] = c.b;
    sg_DrawBufferPtr->sw[3] = c.a;
    sg_DrawBufferPtr++;
}

void DrawVectorFloat(float x, float y, float z, float w)
{
    if (!sg_VIFProgramUpload)
        return;
    qword_t *temp = sg_DrawBufferPtr++;
    ((float *)temp->sw)[0] = x;
    ((float *)temp->sw)[1] = y;
    ((float *)temp->sw)[2] = z;
    ((float *)temp->sw)[3] = w;
}

void DrawPairU64(u64 a, u64 b)
{
    if (!sg_VIFDirectDraw)
        return;
    sg_DrawBufferPtr->dw[0] = a;
    sg_DrawBufferPtr->dw[1] = b;
    sg_DrawBufferPtr++;
}

void StartVertexShader()
{
    u32 arg1 = 0;
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, arg1, VIF_CODE(GetProgramAddressVU1Manager(g_Manager.vu1Manager, sg_ShaderInUse[0]), 0, VIF_CMD_MSCAL, 0), 0);        
    sg_VU1ProgramEnd = sg_DrawBufferPtr;    
    sg_VIFProgramUpload = NULL;
}

void ShaderHeaderLocation(int location)
{   
    if (sg_VIFCodeUpload)
    {
        AddVIFCode(VIF_CODE(location, 0, VIF_CMD_BASE, 0), VIF_CODE(GetDoubleBufferOffset(location), 0, VIF_CMD_OFFSET, 0));
        return;
    }
    sg_DrawBufferPtr = InitDoubleBufferingQWord(sg_DrawBufferPtr, location, GetDoubleBufferOffset(location));
}

void AllocateShaderSpace(int size, int offset)
{
    if (size > 256)
        return;

    sg_TTEUse = true;
    sg_VU1LoadOffset = offset;
    FlushProgram(false);
    u32 outSize = size;
    qword_t **out = &sg_VIFHeaderUpload;
    if (sg_OpenTextureGap)
    {   
        u32 gapSize = outSize;
        if (gapSize >= sg_GapCount)
        {
            gapSize = sg_GapCount-1;
        }
        sg_VIFHeaderUpload = ChangeGapSize(gapSize);
        gapSize = (24-sg_VifCodeUploadCount)-sg_GapCount; 
        ReadUnpackData(sg_VIFHeaderUpload, offset, gapSize, 0, VIF_CMD_UNPACK(0, 3, 0));
        outSize -= gapSize;
        if (!outSize) return;
        if (!sg_OpenDMATag) {
            sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT,
                0, VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), 0);
        }
        out = &sg_SplitHeaderUpload;
    }
    
    CloseDMATag(false);
    *out = sg_DrawBufferPtr++;
    ReadUnpackData(*out, offset+(size-outSize), outSize, 0, VIF_CMD_UNPACK(0, 3, 0));
    memset(sg_DrawBufferPtr, 0, 16*outSize);
    sg_DrawBufferPtr += outSize; 
}

void BindTexture(Texture *tex, bool end, bool immediate)
{
    if (immediate) UploadTextureDrawing(tex, end);

    CloseDMATag(false);

    sg_TTEUse = true;

    u32 arg1 = 0;
    if (sg_VU1ProgramEnd)
    {
        arg1 = VIF_CODE(0, 0, VIF_CMD_FLUSH, 0);
        sg_VU1ProgramEnd = NULL;
    }

    sg_OpenTextureGap = sg_DrawBufferPtr;
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 24, arg1, VIF_CODE(0x0000, 0, VIF_CMD_MSKPATH3, 0), 0);
    sg_VIFCodeUpload = sg_DrawBufferPtr;
    memset(sg_DrawBufferPtr, 0, sizeof(qword_t)*24);
    sg_DrawBufferPtr += 24;
    sg_GapCount = 24;
}

void UploadTextureDrawing(Texture *tex, bool end)
{
    if (!sg_GifDrawPtr) { 
        sg_GifDrawPtr = g_Manager.drawBuffers->currentgif;
    }
    
    sg_GifDrawPtr = CreateTextureUploadChain(tex, sg_GifDrawPtr, false, end);
}

qword_t *GetGlobalDrawPointer()
{
    return sg_DrawBufferPtr;
}

qword_t *GetDrawBegin()
{
    return g_Manager.drawBuffers->currentvif;
}

void InitializeDMATag(qword_t *mem, bool giftag)
{
    int offset = 1;
    sg_OpenDMATag = mem;
    sg_OpenDirectTag = mem+offset++;
    if (giftag) sg_OpenGIFTag = mem+offset++;
    sg_DrawBufferPtr = mem+offset;
}

void InitializeVIFHeaderUpload(qword_t *top, qword_t *bottom, u32 count)
{
    sg_VIFHeaderUpload = top;
    sg_SplitHeaderUpload = bottom;
    sg_GapCount = count;
}

void SendBuffer(qword_t *q)
{
    sg_TTEUse = true;
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CALL, 0, 0, 0, 0, (u32)q);
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0);
}

static inline void AddVIFCode(u32 a1, u32 a2)
{
    sg_VIFCodeUpload->sw[2] = a1;
    sg_VIFCodeUpload->sw[3] = a2;
    sg_VIFCodeUpload++;
    sg_VifCodeUploadCount++;
}

static inline void FlushProgram(bool end)
{
    if (!sg_VU1ProgramEnd)
        return;
    u32 code = (6 * end) + 1;
    sg_VU1ProgramEnd = CreateDMATag(sg_VU1ProgramEnd, code, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, end), 0, 0);  
    sg_DrawBufferPtr++;
    sg_VU1ProgramEnd = NULL;
}

static inline qword_t* ChangeGapSize(u32 size)
{
    sg_GapCount -= (size+1);
    if (sg_GapCount <= sg_VifCodeUploadCount || sg_GapCount < 0)
    {
       sg_GapCount = sg_VifCodeUploadCount;
    }
    AddSizeToDMATag(sg_OpenTextureGap, sg_GapCount);
    return sg_OpenTextureGap + sg_GapCount + 1;
}

static inline qword_t *DetermineVIFHeader(int *offset)
{
    int splitSizeTop = (23 - sg_GapCount);
    if (*offset >= splitSizeTop) { 
        *offset = *offset - splitSizeTop;
        return sg_SplitHeaderUpload;
    }
    return sg_VIFHeaderUpload;
}

static inline qword_t* BindLocation()
{
    qword_t *assign;
    if (sg_OpenTextureGap)
    {  
        assign = ChangeGapSize(0);
        return assign;
    } 
    CloseDMATag(false);
    assign = sg_DrawBufferPtr++;
    return assign;
}

static inline void InitProgramUpload(int vertexNum, int unpacksize)
{
    if (!sg_OpenDMATag && !sg_VIFHeaderUpload && sg_OpenTextureGap)
    {
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0,   VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), 0);
    }   
    CloseDMATag(false);
    sg_OpenTextureGap = NULL;
    sg_TTEUse = true;
    sg_VIFHeaderUpload = NULL;
    sg_VIFCodeUpload = NULL;
    sg_VIFProgramUpload = sg_DrawBufferPtr;

    sg_DrawBufferPtr = ReadUnpackData(sg_DrawBufferPtr, 0, unpacksize+1, 1, VIF_CMD_UNPACK(0, 3, 0));
    for (int i = 0; i < 2; i++) sg_DrawBufferPtr->sw[i] = sg_ShaderInUse[i+1];
    sg_DrawBufferPtr->sw[3] = vertexNum;
    sg_DrawBufferPtr++;
}