#include "ps_font.h"
#include <string.h>
#include "ps_gs.h"
#include "ps_texture.h"
#include "ps_misc.h"
#include "ps_file_io.h"
#include "ps_dma.h"
#include <stdlib.h>
#include <stdio.h>
void PrintText(Font *fontStruct, const char *text, int x, int y)
{

    UploadTextureViaManagerToVRAM(fontStruct->fontTex);

    qword_t *q = InitializeDMAObject();

    u32 sizeOfPipeline;

    qword_t *dcode_tag_vif1 = q;
    q++;

    blend_t blender;
    blender.color1 = BLEND_COLOR_SOURCE;
    blender.color2 = BLEND_COLOR_SOURCE;
    blender.color3 = BLEND_COLOR_SOURCE;
    blender.alpha = BLEND_ALPHA_DEST;
    blender.fixed_alpha = 0x80;

    q = CreateDMATag(q, DMA_CNT, 4, 0, 0, 0);

    q = CreateDirectTag(q, 3, 1);

    q = CreateGSSetTag(q, 2, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    q = SetupZTestGS(q, 1, 1, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

    q = SetupAlphaGS(q, &blender, g_Manager.gs_context);

    q = RenderL(q, fontStruct, x, y, text, g_Manager.gs_context);

    sizeOfPipeline = q - dcode_tag_vif1 - 1;

    CreateDCODEDmaTransferTag(dcode_tag_vif1, DMA_CHANNEL_VIF1, 0, 1, sizeOfPipeline);

    CreateDCODETag(q, DMA_DCODE_END);

    SubmitDMABuffersAsPipeline(q, fontStruct);
}

unsigned char *RewriteAlphaTexBuffer(unsigned char *buffer, int dimX, int dimY)
{
    int totalPix = dimX * dimY * 4;
    for (int i = 0; i < totalPix; i += 4)
    {
        char temp = buffer[i];
        if (temp == 0)
        {
            buffer[i] = buffer[i + 1] = buffer[i + 2] = 0xFF;
            buffer[i + 3] = 0;
        }
        else
        {
            buffer[i + 3] = temp >> 3;
        }
    }
    return buffer;
}

Font *CreateFontStruct(const char *fontName, const char *fontData, int read_type)
{
    prim_t prim;
    color_t color;

    Texture *myFontTex = NULL;
    Font *font = (Font *)malloc(sizeof(Font));

    prim.type = PRIM_TRIANGLE_STRIP;
    prim.shading = PRIM_SHADE_GOURAUD;
    prim.mapping = DRAW_ENABLE;
    prim.fogging = DRAW_DISABLE;
    prim.blending = DRAW_ENABLE;
    prim.antialiasing = DRAW_DISABLE;
    prim.mapping_type = PRIM_MAP_UV;
    prim.colorfix = PRIM_UNFIXED;

    color.r = 0xFF;
    color.g = 0xFF;
    color.b = 0xFF;
    color.a = 0x80;
    color.q = 1.0f;

    font->color = color;
    font->prim = prim;

    myFontTex = AddAndCreateTexture(fontName, read_type, 1, 0x80, TEX_ADDRESS_CLAMP);

    if (myFontTex->psm == GS_PSM_8)
    {
        myFontTex->clut_buffer = RewriteAlphaClutBuffer(myFontTex->clut_buffer);
    }
    else if (myFontTex->psm == GS_PSM_32)
    {
        myFontTex->pixels = RewriteAlphaTexBuffer(myFontTex->pixels, myFontTex->width, myFontTex->height);
    }

    font->fontTex = myFontTex;
    font->fontWidths = NULL;

    CreateFontWidths(font, fontData);

    CreateTexStructs(myFontTex, myFontTex->width, myFontTex->psm, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE, 1);

    CreateClutStructs(myFontTex, 16, GS_PSM_32);

    myFontTex->texbuf.address = g_Manager.textureInVram->texbuf.address;
    myFontTex->clut.address = g_Manager.textureInVram->clut.address;

    return font;
}

void CleanFontStruct(Font *font)
{
    if (font)
    {
        if (font->fontWidths)
        {
            free(font->fontWidths);
        }

        if (font->fontTex)
        {
            CleanTextureStruct(font->fontTex);
        }

        free(font);
    }
}

void CreateFontWidths(Font *font_struct, const char *filePath)
{
    u32 size;
    char _file[MAX_FILE_NAME];
    Pathify(filePath, _file);
    u8 *buffer = ReadFileInFull(_file, &size);

    if (buffer == NULL)
    {
        return;
    }

    u8 *ptr = buffer;

    u32 temp = *((u32 *)buffer);

    font_struct->picWidth = (u16)temp;

    buffer += 4;

    temp = *((u32 *)buffer);

    font_struct->picHeight = (u16)temp;

    buffer += 4;

    temp = *((u32 *)buffer);

    font_struct->cellWidth = (u8)temp;

    buffer += 4;

    temp = *((u32 *)buffer);

    font_struct->cellHeight = (u8)temp;

    buffer += 4;

    font_struct->startingChar = *buffer;

    buffer++;

    u32 charSize = (font_struct->picWidth / font_struct->cellWidth) * (font_struct->picHeight / font_struct->cellHeight);

    // DEBUGLOG(" %d %d %d %d %d %d", charSize, font_struct->startingChar, font_struct->picHeight, font_struct->picWidth, font_struct->cellHeight, font_struct->cellWidth);

    char *fontWidths = (char *)malloc(charSize);
    // DEBUGLOG("size %d", size);
    memcpy(fontWidths, buffer, charSize);
    font_struct->widthSize = charSize;
    font_struct->fontWidths = fontWidths;
    free(ptr);
}

int WidthOfString(Font *font_struct, const char *text)
{
    size_t textlen = strlen(text);
    int size = 0;

    for (u32 letter = 0; letter < textlen; letter++)
    {
        if (text[letter] == '\n')
            break;

        size += font_struct->fontWidths[text[letter]];
    }

    return size;
}

unsigned char *RewriteAlphaClutBuffer(unsigned char *buffer)
{
    for (int i = 0; i < 1024; i += 4)
    {
        char temp = buffer[i];
        if (temp == 0)
        {
            buffer[i] = buffer[i + 1] = buffer[i + 2] = 0xFF;
            buffer[i + 3] = 0;
        }
        else
        {
            buffer[i + 3] = temp >> 3;
        }
    }
    return buffer;
}

#define reglist ((u64)DRAW_UV_REGLIST) << 8 | DRAW_UV_REGLIST

qword_t *RenderL(qword_t *q, Font *font_struct, int x, int y, const char *text, int context)
{
    size_t textlen = strlen(text);

    if (textlen == 0)
        return q;

    qword_t *ret = q;

    u32 qwSize = (textlen * 6) + 6;

    u8 red = font_struct->color.r;
    u8 green = font_struct->color.g;
    u8 blue = font_struct->color.b;
    u8 alpha = font_struct->color.a;

    qword_t *dmatag = ret;
    ret++;

    ret = CreateDirectTag(ret, qwSize, 1);

    ret = CreateGSSetTag(ret, 4, 1, GIF_FLG_PACKED, 1, GIF_REG_AD);

    PACK_GIFTAG(ret, GS_SET_PRIM(font_struct->prim.type, font_struct->prim.shading, font_struct->prim.mapping, font_struct->prim.fogging, font_struct->prim.blending, font_struct->prim.antialiasing, font_struct->prim.mapping_type, context, font_struct->prim.colorfix), GS_REG_PRIM);
    ret++;

    PACK_GIFTAG(ret, GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_REG_RGBAQ);
    ret++;

    PACK_GIFTAG(ret, GS_SET_TEX1(font_struct->fontTex->lod.calculation, font_struct->fontTex->lod.max_level, font_struct->fontTex->lod.mag_filter, font_struct->fontTex->lod.min_filter, font_struct->fontTex->lod.mipmap_select, font_struct->fontTex->lod.l, (int)(font_struct->fontTex->lod.k * 16.0f)), (context == 0) ? GS_REG_TEX1 : GS_REG_TEX1_2);
    ret++;

    PACK_GIFTAG(ret, GS_SET_TEX0(font_struct->fontTex->texbuf.address >> 6, font_struct->fontTex->texbuf.width >> 6, font_struct->fontTex->texbuf.psm, font_struct->fontTex->texbuf.info.width, font_struct->fontTex->texbuf.info.height, font_struct->fontTex->texbuf.info.components, font_struct->fontTex->texbuf.info.function, font_struct->fontTex->clut.address >> 6, font_struct->fontTex->clut.psm, font_struct->fontTex->clut.storage_mode, font_struct->fontTex->clut.start, font_struct->fontTex->clut.load_method), (context == 0) ? GS_REG_TEX0 : GS_REG_TEX0_2);
    ret++;

    u32 regCount = 3;

    u64 regFlag = regCount == 3 ? DRAW_RGBAQ_UV_REGLIST : ((u64)DRAW_UV_REGLIST) << 8 | DRAW_UV_REGLIST;

    ret = CreateGSSetTag(ret, textlen * 4, 1, GIF_FLG_REGLIST, regCount, regFlag);

    u32 lastx = x;
    u32 line = 0;
    u32 uvOffset = 8;
    u32 lineHeight = 20;
    // u32 lastLine = 0;

    u32 cellX = font_struct->cellWidth;
    // u32 cellWHalf = cellX / 2;
    u32 cellY = font_struct->cellHeight;
    char start = font_struct->startingChar;

    int LETTERSPERROW = font_struct->picWidth / cellX;

    for (u32 letter = 0; letter < textlen; letter++)
    {

        char cLetter = text[letter] - start;

        // If the letter is 'Newline' move down a line
        if (cLetter == '\n')
        {
            // lastLine = line;
            ++line;
            lastx = x;
            continue;
        }

        int letterwidth = font_struct->fontWidths[cLetter];

        // DEBUGLOG("%d %d %d %d", letterwidth, cLetter, text[letter], start);

        int tx = (cLetter % LETTERSPERROW);
        int ty = (cLetter / LETTERSPERROW);

        int cx = (tx * cellX);

        int u0 = cx << 4;
        int v0 = ((ty * cellY) << 4) + uvOffset;

        int u1 = ((letterwidth) << 4) + u0;
        int v1 = ((cellY) << 4) + v0 - uvOffset;

        int left = lastx;
        int right = lastx + letterwidth;
        int top = y + (line * lineHeight);
        int bottom = y + (line * lineHeight) + lineHeight;

        PACK_GIFTAG(ret, GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u0, v0));
        ret++;

        PACK_GIFTAG(ret, GIF_SET_XYZ(CreateGSScreenCoordinates(left, +), CreateGSScreenCoordinates(top, +), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));
        ret++;
        PACK_GIFTAG(ret, GIF_SET_UV(u0, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(left, +), CreateGSScreenCoordinates(bottom, +), 0xFFFFFF));
        ret++;
        PACK_GIFTAG(ret, GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u1, v0));

        ret++;
        PACK_GIFTAG(ret, GIF_SET_XYZ(CreateGSScreenCoordinates(right, +), CreateGSScreenCoordinates(top, +), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));
        ret++;

        PACK_GIFTAG(ret, GIF_SET_UV(u1, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(right, +), CreateGSScreenCoordinates(bottom, +), 0xFFFFFF));

        ret++;

        lastx += letterwidth;
    }

    //   DEBUGLOG("---------------------------");

    CreateDMATag(dmatag, DMA_END, ret - dmatag - 1, 0, 0, 0);

    return ret;
}
