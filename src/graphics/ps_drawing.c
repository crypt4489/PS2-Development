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
static qword_t *sg_GifDrawPtr = NULL;
static qword_t *sg_OpenDMATag = NULL;
static qword_t *sg_OpenDirectTag = NULL;
static qword_t *sg_OpenTestReg = NULL;
static qword_t *sg_VIFProgramUpload = NULL;
static qword_t *sg_VIFHeaderUpload = NULL;
static qword_t *sg_VIFDirectDraw = NULL;
static qword_t *sg_SplitHeaderUpload = NULL;
static qword_t *sg_OpenGIFTag = NULL;
static qword_t *sg_VIFCodeUpload = NULL;
static qword_t *sg_OpenTextureGap = NULL;
static qword_t *sg_VU1ProgramEnd = NULL;
static int sg_ShaderInUse[4];
static int sg_RegisterCount;
static int sg_RegisterType;
static u64 sg_PrimitiveType;
static int sg_VifCodeUploadCount;
static int sg_GapCount;
static int sg_VU1LoadOffset;
static int sg_VU1SplitTopSize;
#define sgc_TextureSyncSize 24

static void AddVIFCode(u32 a1, u32 a2);

static void OpenDMATag();

static void CloseDMATag(bool end);

static void OpenGSSetTag();

static void CloseGSSetTag();

static void FlushProgram(bool end);

static qword_t* ChangeGapSize();

static qword_t* BindLocation();

static qword_t *DetermineVIFHeader(int *offset);

static void InitProgramUpload(int vertexNum, int unpacksize);

static int GetOffsetIntoHeader(int offset);

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

qword_t* GetTextureUploadPtr()
{
    return sg_GifDrawPtr;
}

void DispatchDrawBuffers()
{
   
   // PrintOut();

    DrawBuffers *buffer = g_Manager.drawBuffers;

    u32 gifsize = buffer->currentgif - buffer->readgif;

    u32 vifsize = buffer->currentvif - buffer->readvif;
   
    if (gifsize) SubmitDrawBuffersToController(g_Manager.drawBuffers->currentgif, DMA_CHANNEL_GIF, 1, false);

    if (vifsize) SubmitDrawBuffersToController(g_Manager.drawBuffers->currentvif, DMA_CHANNEL_VIF1, 1, true);
    
    ResetState();
}

void ResetDMAState()
{
    sg_OpenDMATag = NULL;
    sg_OpenDirectTag = NULL;
    sg_OpenTestReg = NULL;
    sg_OpenGIFTag = NULL;
    sg_DrawBufferPtr = NULL;
}

void ResetVIFDrawingState()
{
    sg_VIFProgramUpload = NULL;
    sg_VIFHeaderUpload = NULL;
    sg_VIFCodeUpload = NULL;
    sg_VU1ProgramEnd = NULL;
    sg_VIFDirectDraw = NULL;
    sg_VifCodeUploadCount = 0;
    sg_GapCount = 0;
    sg_VU1LoadOffset = 0;
    sg_VU1SplitTopSize = 0;
    sg_DrawBufferPtr = NULL;
    sg_SplitHeaderUpload = NULL;
}

void ResetState() 
{
    // Reset DMA state
    ResetDMAState();

    // Reset VIF drawing state
    ResetVIFDrawingState();

    // Additional resets not covered by the above functions
    sg_OpenTextureGap = NULL;
    for(int i = 0; i < 4; i++) sg_ShaderInUse[i] = 0;
    sg_RegisterCount = 0;
    sg_RegisterType = 0;
    sg_PrimitiveType = 0;
    sg_GifDrawPtr = NULL;
}
void ShaderProgram(int shader, int slot)
{
    sg_ShaderInUse[slot] = shader;
}

void BeginCommand()
{
    if (!sg_DrawBufferPtr) {
        sg_DrawBufferPtr = g_Manager.drawBuffers->currentvif;
    }
}

void BeginCommandSet(qword_t *drawBuffer)
{
    sg_DrawBufferPtr = drawBuffer;
}

int ReturnCommand()
{
    qword_t *retloc = sg_DrawBufferPtr;
    if (sg_VU1ProgramEnd)
    {
        retloc = sg_VU1ProgramEnd;
        FlushProgram(false);
        SetDMACode(retloc, DMA_RET);
    } 
    else if (sg_OpenDMATag)
    {
        retloc = sg_OpenDMATag;
        CloseDMATag(false);
        SetDMACode(retloc, DMA_RET);
    } else {
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_RET, 0, 0, 0, 0, 0);
    }

    u32 size = sg_DrawBufferPtr - g_Manager.drawBuffers->readvif;
    ResetState();
    return size;
}

qword_t* SubmitCommand(bool flush)
{
    FlushProgram(flush);

    CloseDMATag(flush);

    if (flush) DispatchDrawBuffers();
    
    g_Manager.drawBuffers->currentvif = sg_DrawBufferPtr;
    ResetState();
    return g_Manager.drawBuffers->currentvif;
}

void PrintOut()
{
    #include "math/ps_misc.h"
    qword_t *dump = g_Manager.drawBuffers->readvif;
    dump_packet(dump, g_Manager.drawBuffers->currentvif-g_Manager.drawBuffers->readvif, 0);
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
    if ((u32)q & 0x0000000F) { 
        ERRORLOG("misaligned boundary for pushing qword");
        return;    
    }
    offset = GetOffsetIntoHeader(offset);
    qword_t *temp = DetermineVIFHeader(&offset);
    temp += 1 + offset;
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(temp), "r"(q)
        : "memory");
}

void PushMatrix(float *mat, int offset, int size)
{
    offset = GetOffsetIntoHeader(offset);
    qword_t *temp = DetermineVIFHeader(&offset);
    temp += 1 + offset;
    int iter = size>>4;
    for (int i = iter; i>0; i--)
    {
        asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(temp), "r"(mat)
        : "memory");
        temp += 1;
        mat += 4;
    }
}

void PushFloats(float *floats, int offset, int size)
{
    offset = GetOffsetIntoHeader(offset);
    qword_t *temp = DetermineVIFHeader(&offset);
    temp += 1 + offset;
    memcpy(temp, floats, size);
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
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(sg_DrawBufferPtr), "r"(v)
        : "memory");
    sg_DrawBufferPtr++;
}

void DrawColor(Color c)
{
    if (!sg_VIFProgramUpload && !sg_VIFDirectDraw)
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
    if (!sg_VIFDirectDraw && !sg_VIFProgramUpload)
        return;
    sg_DrawBufferPtr->dw[0] = a;
    sg_DrawBufferPtr->dw[1] = b;
    sg_DrawBufferPtr++;
}

void StartVertexShader()
{
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, 0, VIF_CODE(GetProgramAddressVU1Manager(g_Manager.vu1Manager, sg_ShaderInUse[0]), 0, VIF_CMD_MSCAL, 0), 0, 0);        
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
    sg_VU1LoadOffset = offset;
    FlushProgram(false);
    u32 outSize = size;
    qword_t **out = &sg_VIFHeaderUpload;
    if (sg_OpenTextureGap && sg_GapCount > 1)
    {   
        u32 gapSize = outSize;
        if (gapSize >= sg_GapCount)
        {
            gapSize = sg_GapCount-1;
        }
        sg_VIFHeaderUpload = ChangeGapSize(gapSize);
        gapSize = (sgc_TextureSyncSize-(sg_VifCodeUploadCount+sg_GapCount))-1; 
       // DEBUGLOG("%d", gapSize);
        ReadUnpackData(sg_VIFHeaderUpload, offset, gapSize, 0, VIF_CMD_UNPACK(0, 3, 0));
        outSize -= gapSize;
        if (!outSize) { DEBUGLOG("HERE"); return;}
        if (!sg_OpenDMATag) {
            sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT,
                0, VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), 0, 0);
        }
        out = &sg_SplitHeaderUpload;
    }
    
    CloseDMATag(false);
    *out = sg_DrawBufferPtr++;
    ReadUnpackData(*out, offset+(size-outSize), outSize, 0, VIF_CMD_UNPACK(0, 3, 0));
    memset(sg_DrawBufferPtr, 0, 16*outSize);
    sg_DrawBufferPtr += outSize; 
}

void BindTexture(Texture *tex, bool immediate)
{
    if (immediate) UploadTextureDrawing(tex);

    CloseDMATag(false);

    sg_VU1ProgramEnd = NULL;

    sg_GapCount = sgc_TextureSyncSize;

    sg_OpenTextureGap = sg_DrawBufferPtr;
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, sg_GapCount, VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), VIF_CODE(0x0000, 0, VIF_CMD_MSKPATH3, 0), 0, 0);
    sg_VIFCodeUpload = sg_DrawBufferPtr;
    memset(sg_DrawBufferPtr, 0, sizeof(qword_t)*sg_GapCount);
    sg_DrawBufferPtr += sg_GapCount;
    
}

void UploadTextureDrawing(Texture *tex)
{
    if (!sg_GifDrawPtr) sg_GifDrawPtr = g_Manager.drawBuffers->currentgif;
    sg_GifDrawPtr = CreateTextureUploadChain(tex, sg_GifDrawPtr, false, false);
    g_Manager.drawBuffers->currentgif = sg_GifDrawPtr; //advance pointer;
}

qword_t *GetGlobalDrawPointer()
{
    return sg_DrawBufferPtr;
}

qword_t *GetDrawBegin()
{
    return g_Manager.drawBuffers->readvif;
}

void StitchDrawBuffer(bool textures)
{
    u32 gifsize = g_Manager.drawBuffers->currentgif - g_Manager.drawBuffers->readgif;
    u32 vifsize = g_Manager.drawBuffers->currentvif - g_Manager.drawBuffers->readvif;
    if (gifsize && textures) g_Manager.drawBuffers->currentgif = StitchDMAChain(g_Manager.drawBuffers->readgif, g_Manager.drawBuffers->currentgif, false);
    if (vifsize)  g_Manager.drawBuffers->currentvif = StitchDMAChain(g_Manager.drawBuffers->readvif, g_Manager.drawBuffers->currentvif, true);
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
    sg_VU1LoadOffset = (top->sw[3] & 0x00003FFF);
    sg_VifCodeUploadCount = sgc_TextureSyncSize - (top->sw[0] & 0x0000FFFF) - count - 1;
    sg_VU1SplitTopSize = sgc_TextureSyncSize - (sg_GapCount+sg_VifCodeUploadCount);
   // DEBUGLOG("%d %d %d", sg_GapCount, sg_VifCodeUploadCount, sg_VU1SplitTopSize);
}

void CallCommand(qword_t *q, bool delay)
{
    BeginCommand();
    sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CALL, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0, 0, (u32)q);
    if (delay) { g_Manager.drawBuffers->currentvif++; }
    else sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_END, 0, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0, 0);
}

static void AddVIFCode(u32 a1, u32 a2)
{
    if (!sg_VIFCodeUpload)
        return;
    sg_VIFCodeUpload->sw[2] = a1;
    sg_VIFCodeUpload->sw[3] = a2;
    sg_VIFCodeUpload++;
    sg_VifCodeUploadCount++;
    if (sg_GapCount) sg_GapCount--;
}

static void FlushProgram(bool end)
{
    if (!sg_VU1ProgramEnd)
        return;
    u32 code = (6 * end) + 1;
    sg_VU1ProgramEnd = CreateDMATag(sg_VU1ProgramEnd, code, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, end), 0, 0, 0);  
    sg_DrawBufferPtr++;
    sg_VU1ProgramEnd = NULL;
}

static qword_t* ChangeGapSize(u32 size)
{
    sg_GapCount -= (size+1);
    if (sg_GapCount <= sg_VifCodeUploadCount)
    {
       sg_GapCount = 0;
    }
   // DEBUGLOG("%d %d", sg_GapCount, sg_VifCodeUploadCount);
    AddSizeToDMATag(sg_OpenTextureGap, sg_GapCount + sg_VifCodeUploadCount);
    sg_VU1SplitTopSize = (sgc_TextureSyncSize - (sg_GapCount + sg_VifCodeUploadCount));
    return sg_OpenTextureGap + sg_GapCount + sg_VifCodeUploadCount + 1;
}

static qword_t *DetermineVIFHeader(int *offset)
{
    if (sg_SplitHeaderUpload && *offset >= sg_VU1SplitTopSize) { 
        *offset = *offset - sg_VU1SplitTopSize;
        return sg_SplitHeaderUpload;
    }
    return sg_VIFHeaderUpload;
}

static qword_t* BindLocation()
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

static void InitProgramUpload(int vertexNum, int unpacksize)
{
    if ((!sg_OpenDMATag && !sg_VIFHeaderUpload) && sg_OpenTextureGap)
    {
        sg_DrawBufferPtr = CreateDMATag(sg_DrawBufferPtr, DMA_CNT, 0, VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0), 0, 0);
    }

    CloseDMATag(false);
    sg_OpenTextureGap = NULL;
    sg_VIFHeaderUpload = NULL;
    sg_VIFCodeUpload = NULL;
    sg_VIFProgramUpload = sg_DrawBufferPtr;

    sg_DrawBufferPtr = ReadUnpackData(sg_DrawBufferPtr, 0, unpacksize+1, 1, VIF_CMD_UNPACK(0, 3, 0));
    for (int i = 2; i >= 0; i--) sg_DrawBufferPtr->sw[i] = sg_ShaderInUse[i+1];
    sg_DrawBufferPtr->sw[3] = vertexNum;
    sg_DrawBufferPtr++;
}

static int GetOffsetIntoHeader(int offset)
{
    if (offset < sg_VU1LoadOffset)
    {
        ERRORLOG("Pushing with too high offset %d %d", offset, sg_VU1LoadOffset);
        return -1;
    }
    return offset - sg_VU1LoadOffset;
}

static void OpenDMATag()
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
       
    sg_DrawBufferPtr = CreateDMATag(sg_OpenDMATag, DMA_CNT, 0, arg1, arg2, 0, 0);
    sg_OpenDirectTag = sg_DrawBufferPtr;
    sg_DrawBufferPtr = CreateDirectTag(sg_OpenDirectTag, 0, 0);
}

static void CloseDMATag(bool end)
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

static void CloseGSSetTag()
{
    if (!sg_OpenGIFTag)
        return;
    
    AddSizeToGSSetTag(sg_OpenGIFTag, sg_DrawBufferPtr-sg_OpenGIFTag-1);
    sg_OpenGIFTag = NULL;
}

static void OpenGSSetTag()
{
    if (sg_OpenGIFTag)
        return;
    
    sg_OpenGIFTag = sg_DrawBufferPtr++;
    CreateGSSetTag(sg_OpenGIFTag, 0, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);
}