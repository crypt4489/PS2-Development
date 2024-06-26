#include "textures/ps_texture.h"

#include <dma.h>
#include <gs_gp.h>
#include <graph.h>
#include <draw2d.h>
#include <draw3d.h>
#include <draw.h>

#include <string.h>
#include <stdlib.h>

#include "gamemanager/ps_manager.h"
#include "gs/ps_gs.h"
#include "io/ps_texture_io.h"
#include "math/ps_fast_maths.h"
#include "dma/ps_dma.h"
#include "math/ps_misc.h"
#include "log/ps_log.h"
#include "util/ps_linkedlist.h"

static void SamplePaletteImpl(Texture *tex, Color *rgba, int x, int y);
static void SampleNonPaletteImpl(Texture *tex, Color *rgba, int x, int y, int stride);
static inline void PackColor(Color *rgba, u8* buffer, int stride);

static void StripFilePath(const char *filepath, char *texname)
{
    const char *temp = strrchr(filepath, 92);

    if (temp == NULL)
    {
        temp = filepath;
    }
    else
    {
        temp++;
    }

    int i = 0;
    while (*temp != 0)
    {
        texname[i] = *temp;
        temp++;
        i++;
    }

    texname[i] = 0;
}

u32 GetTextureIDByName(const char *name, TexManager *texManager)
{
    Texture *tex = GetTexByName(texManager, name);
    if (tex == NULL)
        return 0;
    return tex->id;
}

Texture *GetTextureByID(u32 id, TexManager *texManager)
{
    LinkedList *node = texManager->list;
    Texture *tex = NULL;
    while (node != NULL)
    {
        tex = (Texture *)node->data;
        if (tex->id == id)
            return tex;
        node = node->next;
    }
    return NULL;
}

void AddStringNameToTexture(Texture *tex, const char *buffer)
{
    memcpy(tex->name, buffer, strnlen(buffer, MAX_CHAR_TEXTURE_NAME));
}

void CleanTextureStruct(Texture *tex)
{
    if (tex != NULL)
    {
        if (tex->clut_buffer != NULL)
        {
            free(tex->clut_buffer);
        }

        if (tex->pixels != NULL)
        {
            free(tex->pixels);
        }

        if (tex->upload != NULL)
        {
            free(tex->upload);
        }

        free(tex);
    }
}

Texture *AddAndCreateAlphaMap(const char *filePath, u32 readType, u32 mode)
{
    Texture *alphaMap = AddAndCreateTexture(filePath, readType, 0, 0, mode, 0);

    u32 size = 0;
    u8 *pixels = NULL;

    if (alphaMap->type == GS_PSM_8)
    {
        size = 256 * 4;
        pixels = alphaMap->clut_buffer;
    }
    else if (alphaMap->type == GS_PSM_32)
    {
        size = alphaMap->height * alphaMap->width * 4;
        pixels = alphaMap->pixels;
    }

    if (pixels == NULL || size == 0)
    {
        ERRORLOG("Something went wrong with the alpha map");
        return alphaMap;
    }

    for (int i = 0; i < size; i += 4)
    {
        // DEBUGLOG("%x %x %x %x", alpha->pixels[i], alpha->pixels[i+1], alpha->pixels[i+2], alpha->pixels[i+3]);
        if (pixels[i] < 0x80)
        {
            pixels[i + 3] = 0;
        }
        else
        {
            pixels[i + 3] = 0xFF;
        }
    }

    return alphaMap;
}

void AddMipMapTexture(Texture *tex, Texture *toAdd)
{
    LinkedList *node = CreateLinkedListItem((void *)toAdd);
    tex->mipMaps = AddToLinkedList(tex->mipMaps, node);
    tex->mipLevels++;
}

Texture *RemoveMipLevelFromTexture(Texture *tex, Texture *toRemove)
{
    LinkedList *iter = tex->mipMaps;
    Texture *comp = (Texture *)iter->data;
    while (comp != toRemove && iter != NULL)
    {
        iter = iter->next;
        comp = (Texture *)iter->data;
    }

    if (iter == NULL)
        return tex;

    INFOLOG("found");
    tex->mipMaps = RemoveNodeFromList(tex->mipMaps, iter);
    tex->mipLevels -= 1;
    return tex;
}

void InitTextureResources(Texture *tex, u32 mode)
{
    if (tex)
    {
        tex->mipLevels = 0;

        tex->mipMaps = NULL;

        tex->mode = mode;

        tex->type = PS_TEX_MEMORY;

        if (tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4)
        {
            CreateClutStructs(tex, GS_PSM_32);
        }

        u32 components;

        if (tex->psm == GS_PSM_24)
        {
            components = TEXTURE_COMPONENTS_RGB;
        } 
        else 
        {
            components = TEXTURE_COMPONENTS_RGBA;
        }

        CreateTexStructs(tex, tex->width, tex->psm, components, TEXTURE_FUNCTION_MODULATE, 0);

        tex->upload = (qword_t *)malloc(sizeof(qword_t) * 50);
    }
}

Texture *AddAndCreateTextureFromBuffer(u8 *fileData, u32 size, 
                                       const char *nameOfTex, u32 readType, 
                                       u8 useAlpha, u8 alpha, 
                                       u32 mode, u8 texFiltering)
{

    Texture *tex = (Texture*)malloc(sizeof(Texture));

    CreateTextureParams params;

    params.name = nameOfTex;
    params.readType = readType;
    params.alpha = alpha;
    params.useAlpha = useAlpha;

    CreateTextureFromFile(tex, &params, fileData, size);

    InitTextureResources(tex, mode);

    AddToManagerTexList(&g_Manager, tex);

    return tex;
}

Texture *AddAndCreateTexture(const char *filePath, u32 readType, u8 useProgrammedAlpha, u8 alphaVal, u32 mode, u8 texFiltering)
{
    char _file[MAX_FILE_NAME];
    char texName[MAX_CHAR_TEXTURE_NAME];
    Pathify(filePath, _file);
    StripFilePath(filePath, texName);

    Texture *tex = ReadTexFile(_file, texName, readType, alphaVal, useProgrammedAlpha);

    InitTextureResources(tex, mode);

    AddToManagerTexList(&g_Manager, tex);

    return tex;
}

qword_t *set_tex_address_mode_gif(qword_t *q, u32 mode, u32 context)
{
    qword_t *b = q;
    DMATAG_CNT(b, 2, 0, 0, 0);
    b++;

    PACK_GIFTAG(b, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    b++;

    texwrap_t wrap;
    wrap.horizontal = WRAP_CLAMP;
    wrap.vertical = WRAP_CLAMP;

    if (mode == TEX_ADDRESS_WRAP)
    {
        wrap.horizontal = WRAP_REPEAT;
        wrap.vertical = WRAP_REPEAT;
    }

    wrap.minu = wrap.maxu = 0;
    wrap.minv = wrap.maxv = 0;

    PACK_GIFTAG(b, GS_SET_CLAMP(wrap.horizontal, wrap.vertical, wrap.minu, wrap.maxu, wrap.minv, wrap.maxv), GS_REG_CLAMP + context);

    b++;

    return b;
}

qword_t *gif_setup_tex(qword_t *b, Texture *tex, u32 context)
{
    qword_t *q = b;
    DMATAG_CNT(q, 3, 0, 0, 0);
    q++;

    q->dw[0] = GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1);
    q->dw[1] = GIF_REG_AD;
    q++;
    q->dw[0] = GS_SET_TEX1(tex->lod.calculation, tex->lod.max_level, tex->lod.mag_filter, tex->lod.min_filter, tex->lod.mipmap_select, tex->lod.l, (int)(tex->lod.k * 16.0f));
    q->dw[1] = GS_REG_TEX1 + context;
    q++;
    q->dw[0] = GS_SET_TEX0(
        tex->texbuf.address >> 6,
        tex->texbuf.width >> 6,
        tex->texbuf.psm,
        tex->texbuf.info.width,
        tex->texbuf.info.height,
        tex->texbuf.info.components,
        tex->texbuf.info.function,
        tex->clut.address >> 6,
        tex->clut.psm,
        tex->clut.storage_mode,
        tex->clut.start,
        tex->clut.load_method);
    q->dw[1] = GS_REG_TEX0 + context;
    q++;

    return q;
}

qword_t *CreateTexChain(qword_t *input, Texture *tex)
{

    qword_t *q = input;
    u32 sizeOfPipeline = 0;

    if (tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4)
    {
        qword_t *dcode_tag_gif_clut = q;

        q++;
        int width = (tex-> psm  == GS_PSM_8) ? 16 : 8;
        int height = (tex-> psm  == GS_PSM_8) ? 16 : 2;
        q = draw_texture_transfer(q, tex->clut_buffer, width, height, tex->clut.psm, tex->clut.address, 16);
        q = draw_texture_flush(q);

        sizeOfPipeline = q - dcode_tag_gif_clut - 1;

        CreateDCODEDmaTransferTag(dcode_tag_gif_clut, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);
    }

    qword_t *dcode_tag_gif_pixels = q;
    q++;

    q = draw_texture_transfer(q, tex->pixels, tex->width, tex->height, tex->psm, tex->texbuf.address, tex->texbuf.width);
    q = draw_texture_flush(q);

    sizeOfPipeline = q - dcode_tag_gif_pixels - 1;

    CreateDCODEDmaTransferTag(dcode_tag_gif_pixels, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);
    CreateDCODETag(q, DMA_DCODE_END);
    return q;
}

static qword_t *CreateTexChainMipLevels(qword_t *input, Texture *tex, u32 texBuffer, u32 clutBuffer)
{

    qword_t *q = input;
    u32 sizeOfPipeline = 0;

    if (tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4)
    {
        qword_t *dcode_tag_gif_clut = q;

        q++;

        q = draw_texture_transfer(q, tex->clut_buffer, 16, 16, tex->clut.psm, clutBuffer, 16);
        q = draw_texture_flush(q);

        sizeOfPipeline = q - dcode_tag_gif_clut - 1;

        CreateDCODEDmaTransferTag(dcode_tag_gif_clut, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);
    }

    qword_t *dcode_tag_gif_pixels = q;
    q++;

    q = draw_texture_transfer(q, tex->pixels, tex->width, tex->height, tex->psm, texBuffer, tex->texbuf.width);
    q = draw_texture_flush(q);

    sizeOfPipeline = q - dcode_tag_gif_pixels - 1;

    CreateDCODEDmaTransferTag(dcode_tag_gif_pixels, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);
    CreateDCODETag(q, DMA_DCODE_END);
    return q;
}

qword_t *CreateTexChainWOTAGS(qword_t *input, Texture *tex)
{

    qword_t *q = input;

    if (tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4)
    {
        int width = (tex-> psm  == GS_PSM_8) ? 16 : 8;
        int height = (tex-> psm  == GS_PSM_8) ? 16 : 2;
        q = draw_texture_transfer(q, tex->clut_buffer, 16, 16, tex->clut.psm, g_Manager.textureInVram->clut.address, 16);
        q = draw_texture_flush(q);
    }

    q = draw_texture_transfer(q, tex->pixels, tex->width, tex->height, tex->psm, g_Manager.textureInVram->texbuf.address, tex->texbuf.width);
    q = draw_texture_flush(q);

    return q;
}

void ParseTextureUpload(qword_t *in)
{
    DMA_DCODE_STRUCT decode;
    int loop = 1;
    int channel, qwc, tte, type;
    qword_t *q = in;
    while (loop)
    {
        decode.code = q->sw[0];
        if (decode.code == DMA_DCODE_END)
        {
            break;
        }
        else
        {
            channel = decode.chann;
            qwc = decode.qwc;
            tte = decode.tte;
            type = decode.type;
            if (channel != 0)
            {
                q++;
                SubmitToDMAController(q, channel, type, qwc, tte);
            }
            else
            {
                ERRORLOG("WE MADE IT HERE!");
                dump_packet(in, 256, 0);
                while (1)
                    ;
                loop = 0;
            }

            q += qwc;
        }
    }
}

void SetupTexLODStruct(Texture *tex, float _k, char _l, int max, int filter_min, int filter_mag)
{
    tex->lod.mag_filter = filter_mag; // when K < 0
    tex->lod.min_filter = filter_min; // when K >= 0;

    tex->lod.l = _l;
    tex->lod.k = _k;
    tex->lod.calculation = LOD_USE_K;
    tex->lod.max_level = max;
    tex->lod.mipmap_select = LOD_MIPMAP_REGISTER;
}

static void SetupTexRegistersGIF(Texture *tex)
{

    qword_t *q = InitializeDMAObject();

    q = set_tex_address_mode_gif(q, tex->mode, g_Manager.gs_context);
    q = gif_setup_tex(q, tex, g_Manager.gs_context);
    if (tex->type == PS_TEX_VRAM)
    {
        q = draw_texture_flush(q);
    }

    SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
}

#define GS_SET_MIPTBP(TBA1, TBW1, TBA2, TBW2, TBA3, TBW3)                     \
    (u64)((TBA1) & 0x00003FFF) << 0 | (u64)((TBW1) & 0x0000003F) << 14 |      \
        (u64)((TBA2) & 0x00003FFF) << 20 | (u64)((TBW2) & 0x0000003F) << 34 | \
        (u64)((TBA3) & 0x00003FFF) << 40 | (u64)((TBW3) & 0x0000003F) << 54

static void SetupMipMapRegistersGIF(u32 *tex_addresses, u32 *widths)
{

    qword_t *q = InitializeDMAObject();
    DMATAG_CNT(q, 3, 0, 0, 0);
    q++;
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_MIPTBP(tex_addresses[0] >> 6, widths[0] >> 6, tex_addresses[1] >> 6, widths[1] >> 6, tex_addresses[2] >> 6, widths[2] >> 6), GS_REG_MIPTBP1 + g_Manager.gs_context);
    q++;

    PACK_GIFTAG(q, GS_SET_MIPTBP(tex_addresses[3] >> 6, widths[3] >> 6, tex_addresses[4] >> 6, widths[4] >> 6, tex_addresses[5] >> 6, widths[5] >> 6), GS_REG_MIPTBP2 + g_Manager.gs_context);
    q++;

    SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
}

void UploadTextureToVRAM(Texture *tex)
{
    qword_t *q = tex->upload;
    q = CreateTexChain(q, tex);
    ParseTextureUpload(tex->upload);
    SetupTexRegistersGIF(tex);
    g_Manager.texManager->currIndex = tex->id;
}

void UploadTextureViaManagerToVRAM(Texture *tex)
{
    TexManager *texManager = g_Manager.texManager;

    if ((tex->id != texManager->currIndex && tex->type == PS_TEX_MEMORY))
    {
        qword_t *q = tex->upload;
        q = CreateTexChain(q, tex);
        ParseTextureUpload(tex->upload);

        texManager->currIndex = tex->id;

        if (tex->mipLevels >= 1)
        {
            LinkedList *list = tex->mipMaps;
            u32 currMap = 1;
            u32 addrs[6], widths[6];
            Texture *mipTex = tex;
            Texture *prev = tex;
            u32 lowestAddress = prev->texbuf.address;
            while (tex->mipLevels >= currMap)
            {
                u32 newAddress;
                if ((GRAPH_VRAM_MAX_WORDS < (newAddress = lowestAddress + graph_vram_size(prev->width,
                                                                                          prev->height, prev->psm, GRAPH_ALIGN_BLOCK)) ||
                     list == NULL))
                {
                    ERRORLOG("Too many mipmaps added to memory");
                    break;
                }

                addrs[currMap - 1] = newAddress;
                mipTex = (Texture *)list->data;
                qword_t *mipQ = mipTex->upload;
                widths[currMap - 1] = mipTex->texbuf.width;
                mipQ = CreateTexChainMipLevels(mipQ, mipTex, addrs[currMap - 1], 0);
                ParseTextureUpload(mipTex->upload);
                prev = mipTex;
                lowestAddress = addrs[currMap - 1];
                list = list->next;
                currMap++;
            }
            SetupMipMapRegistersGIF(addrs, widths);
        }
        SetupTexRegistersGIF(tex);
    }
    else
    {
        if (tex->type == PS_TEX_VRAM)
        {
            texManager->currIndex = -1; // invalidate the current texture upload identifier
        }

        SetupTexRegistersGIF(tex); // already uploaded, just reset the registers for the current context
    }
}

void SampleTextureByUV(Texture *tex, Color *rgba, float x, float y)
{
    int intx = x * tex->width; 
    int inty = y * tex->height;

    u32 psm = tex->texbuf.psm;

    if (psm == GS_PSM_8)
    {
        SamplePaletteImpl(tex, rgba, intx, inty);
    } 
    else 
    {
        int stride;
        switch(psm)
        {
            case GS_PSM_16:
                stride = 2;
                break;
            case GS_PSM_24:
                stride = 3;
                break;
            default:
                stride = 4;
                break;
        }
        SampleNonPaletteImpl(tex, rgba, intx, inty, stride);
    }
}


void SampleTextureByPixel(Texture *tex, Color *rgba, int x, int y)
{
    u32 psm = tex->texbuf.psm;
    if (psm == GS_PSM_8)
    {
        SamplePaletteImpl(tex, rgba, x, y);
    } 
    else 
    {
        int stride;
        switch(psm)
        {
            case GS_PSM_16:
                stride = 2;
                break;
            case GS_PSM_24:
                stride = 3;
                break;
            default:
                stride = 4;
                break;
        }
        SampleNonPaletteImpl(tex, rgba, x, y, stride);
    }
}

static void SamplePaletteImpl(Texture *tex, Color *rgba, int x, int y)
{
    
}

static void SampleNonPaletteImpl(Texture *tex, Color *rgba, int x, int y, int stride)
{
    u8 *buffer = tex->pixels;
    u32 offsetY = stride * tex->width * y;
    buffer += (offsetY + x * stride);
    PackColor(rgba, buffer, stride);
}

static inline void PackColor(Color *rgba, u8* buffer, int stride)
{
    switch(stride)
    {
        case 3:
            rgba->r = buffer[0];
            rgba->g = buffer[1];
            rgba->b = buffer[2];
            break;
        case 4:
            rgba->r = buffer[0];
            rgba->g = buffer[1];
            rgba->b = buffer[2];
            rgba->a = buffer[3];
            break;
    }
}
