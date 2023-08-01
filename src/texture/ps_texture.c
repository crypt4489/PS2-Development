#include "textures/ps_texture.h"

#include <string.h>
#include <dma.h>
#include <gs_gp.h>
#include <stdlib.h>

#include "gamemanager/ps_manager.h"
#include "gs/ps_gs.h"
#include "io/ps_texture_io.h"
#include "math/ps_fast_maths.h"
#include "dma/ps_dma.h"
#include "math/ps_misc.h"
#include "log/ps_log.h"

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
            break;
        node = node->next;
    }
    return tex;
}

void AddStringNameToTexture(Texture *tex, const char *buffer)
{
    memcpy(tex->name, buffer, strnlen(buffer, MAX_CHAR_TEXTURE_NAME));
}

void CleanTextureStruct(Texture *tex)
{
    if (tex->clut_buffer)
    {
        free(tex->clut_buffer);
    }

    if (tex->pixels)
    {
        free(tex->pixels);
    }

    free(tex);
}

Texture *AddAndCreateAlphaMap(const char *filePath, u32 readType, u32 mode)
{
    Texture *alphaMap = AddAndCreateTexture(filePath, readType, 0, 0, mode);

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

Texture *AddAndCreateTexture(const char *filePath, u32 readType, u8 useProgrammedAlpha, u8 alphaVal, u32 mode)
{
    char _file[MAX_FILE_NAME];
    char texName[MAX_CHAR_TEXTURE_NAME];
    Pathify(filePath, _file);
    StripFilePath(filePath, texName);
    Texture *tex = ReadTexFile(_file, texName, readType, alphaVal, useProgrammedAlpha);

    if (tex)
    {

        tex->mode = mode;

        tex->type = PS_TEX_MEMORY;

        AddToManagerTexList(&g_Manager, tex);

        if (tex->psm == GS_PSM_8)
        {
            CreateClutStructs(tex, 16, GS_PSM_32);
        }

        CreateTexStructs(tex, tex->width, tex->psm, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE, 1);

        tex->upload_dma = (qword_t *)malloc(sizeof(qword_t) * 50);
        qword_t *q = tex->upload_dma;
        q = CreateTexChain(q, tex);
        CreateDCODETag(q, DMA_DCODE_END);
    }

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
    wrap.minu = wrap.maxu = 0;
    wrap.minv = wrap.maxv = 0;

    if (mode == TEX_ADDRESS_WRAP)
    {
        wrap.horizontal = WRAP_REPEAT;
        wrap.vertical = WRAP_REPEAT;
    }

    PACK_GIFTAG(b, GS_SET_CLAMP(wrap.horizontal, wrap.vertical, wrap.minu, wrap.maxu, wrap.minv, wrap.maxv), GS_REG_CLAMP + context);

    b++;

    return b;
}

qword_t *gif_setup_tex(qword_t *b, Texture *tex, u32 context)
{
    qword_t *q = b;
    DMATAG_CNT(q, 4, 0, 0, 0);
    q++;

    q->dw[0] = GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1);
    q->dw[1] = GIF_REG_AD;
    q++;
    q->dw[0] = GS_SET_TEX1(tex->lod.calculation, tex->lod.max_level, tex->lod.mag_filter, tex->lod.min_filter, tex->lod.mipmap_select, tex->lod.l, (int)(tex->lod.k * 16.0f));
    q->dw[1] = context == 0 ? GS_REG_TEX1 : GS_REG_TEX1_2;
    q++;
    q->dw[0] = GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1);
    q->dw[1] = GIF_REG_AD;
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
    q->dw[1] = context == 0 ? GS_REG_TEX0 : GS_REG_TEX0_2;
    q++;

    return q;
}

qword_t *CreateTexChain(qword_t *input, Texture *tex)
{

    qword_t *q = input;
    u32 sizeOfPipeline = 0;

    if (tex->psm == GS_PSM_8)
    {
        qword_t *dcode_tag_gif_clut = q;

        q++;

        q = draw_texture_transfer(q, tex->clut_buffer, 16, 16, tex->clut.psm, g_Manager.textureInVram->clut.address, 16);
        q = draw_texture_flush(q);

        sizeOfPipeline = q - dcode_tag_gif_clut - 1;

        CreateDCODEDmaTransferTag(dcode_tag_gif_clut, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);
    }

    qword_t *dcode_tag_gif_pixels = q;
    q++;

    q = draw_texture_transfer(q, tex->pixels, tex->width, tex->height, tex->psm, g_Manager.textureInVram->texbuf.address, tex->texbuf.width);
    q = draw_texture_flush(q);

    sizeOfPipeline = q - dcode_tag_gif_pixels - 1;

    CreateDCODEDmaTransferTag(dcode_tag_gif_pixels, DMA_CHANNEL_GIF, 0, 1, sizeOfPipeline);

    return q;
}

qword_t *CreateTexChainWOTAGS(qword_t *input, Texture *tex)
{

    qword_t *q = input;

    if (tex->psm == GS_PSM_8)
    {

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
            loop = 0;
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

void SetupTexRegistersGIF(Texture *tex)
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

void UploadTextureViaManagerToVRAM(Texture *tex)
{
    TexManager *texManager = g_Manager.texManager;

    if (tex->id != texManager->currIndex && tex->type == PS_TEX_MEMORY)
    {
        ParseTextureUpload(tex->upload_dma);
        SetupTexRegistersGIF(tex);
        texManager->currIndex = tex->id;
    }
    else
    {

        if (tex->type == PS_TEX_VRAM)
        {
            texManager->currIndex = -1; // invalidate the current texture upload identifier
        }

        SetupTexRegistersGIF(tex); // already uploaded, just revalidate the registers for the current context
    }
}

Texture *SetTextureFilter(Texture *tex, u8 filter)
{
    if (filter == PS_FILTER_NNEIGHBOR)
    {
    }
    else if (filter == PS_FILTER_BILINEAR)
    {
    }

    tex->lod.mag_filter = filter;

    return tex;
}
