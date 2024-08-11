#include "textures/ps_font.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <draw2d.h>
#include <draw3d.h>

#include "log/ps_log.h"
#include "gs/ps_gs.h"
#include "textures/ps_texture.h"
#include "math/ps_misc.h"
#include "io/ps_file_io.h"
#include "dma/ps_dma.h"
#include "graphics/ps_drawing.h"

#define MAX_DISPLAY_TEXT 100

void PrintText(Font *fontStruct, const char *text, int x, int y, TextAlign alignment)
{
    

    blend_t blender;
    blender.color1 = BLEND_COLOR_SOURCE;
    blender.color2 = BLEND_COLOR_SOURCE;
    blender.color3 = BLEND_COLOR_SOURCE;
    blender.alpha = BLEND_ALPHA_DEST;
    blender.fixed_alpha = 0x80;

    BeginCommand();
    BindTexture(fontStruct->fontTex, true);
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0);
    BlendingEquation(&blender);
    int ret = -1;
    
    switch (alignment)
    {
    case LEFT:
        ret = RenderL(fontStruct, x, y, text);
        break;
    
    default:
        ERRORLOG("Invalid Text Alignment used");
        return;
    }

    //if (ret) return;
   
   // 
  // PrintOut();
    EndCommand();
    
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

Font *CreateFontStructFromBuffer(const char *fontName, u8 *fontPic,
                                 u8 *fontData, int read_type,
                                 u32 picSize, u32 dataSize)
{
    prim_t prim;
    Color color;

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

    myFontTex = AddAndCreateTextureFromBuffer(fontPic, picSize,
                                              fontName, read_type,
                                              1, 0x80,
                                              TEX_ADDRESS_CLAMP);

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

    CreateFontWidthsFromFile(font, NULL, fontData, dataSize);
    myFontTex->lod.mag_filter = LOD_MAG_LINEAR;
	myFontTex->lod.min_filter = LOD_MIN_LINEAR;

    return font;
}

Font *CreateFontStruct(const char *fontName, const char *fontData, int read_type)
{
    prim_t prim;
    Color color;

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
    color.g = 0x00;
    color.b = 0x00;
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

    LoadFontWidths(font, fontData);


   
	myFontTex->lod.mag_filter = LOD_MAG_LINEAR;
	myFontTex->lod.min_filter = LOD_MIN_LINEAR;


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

void CreateFontWidthsFromFile(void *object, void *params, u8 *buffer, u32 bufferLen)
{

    Font *font_struct = (Font *)object;

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

    u8 start = font_struct->startingChar = *buffer;

    buffer++;

    u32 charSize = (font_struct->picWidth / font_struct->cellWidth) * (font_struct->picHeight / font_struct->cellHeight);

    //  DEBUGLOG(" %d %d %d %d %d %d", charSize, font_struct->startingChar, font_struct->picHeight, font_struct->picWidth, font_struct->cellHeight, font_struct->cellWidth);

    char *fontWidths = (char *)malloc(charSize);
    // DEBUGLOG("size %d", size);
    memcpy(fontWidths, buffer + start, charSize);
    font_struct->widthSize = charSize;
    font_struct->fontWidths = fontWidths;
}

void LoadFontWidths(Font *font_struct, const char *filePath)
{
    u32 size;
    char _file[MAX_FILE_NAME];
    Pathify(filePath, _file);
    u8 *buffer = ReadFileInFull(_file, &size);

    if (!buffer)
    {
        ERRORLOG("Cannot open font widths file %s", filePath);
        return;
    }

    CreateFontWidthsFromFile(font_struct, NULL, buffer, size);

    free(buffer);
}

int WidthOfString(Font *font_struct, const char *text)
{
    int textlen = strnlen(text, MAX_DISPLAY_TEXT);
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
        if (!temp)
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


int RenderL(Font *font_struct, int x, int y, const char *text)
{

    size_t textlen = strnlen(text, MAX_DISPLAY_TEXT);

    if (!textlen)
    {
        ERRORLOG("Text length of zero for render left");
        return -1;
    }

    u8 red = font_struct->color.r;
    u8 green = font_struct->color.g;
    u8 blue = font_struct->color.b;
    u8 alpha = font_struct->color.a;

    PrimitiveColor(font_struct->color);
    PrimitiveTypeStruct(font_struct->prim);

    SetRegSizeAndType(3, DRAW_RGBAQ_UV_REGLIST);
    DrawCountDirect(textlen * 4);

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

        DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u0, v0));

        DrawPairU64(GIF_SET_XYZ(CreateGSScreenCoordinates(left, +), CreateGSScreenCoordinates(top, +), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));

        DrawPairU64(GIF_SET_UV(u0, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(left, +), CreateGSScreenCoordinates(bottom, +), 0xFFFFFF));

        DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_UV(u1, v0));

        DrawPairU64(GIF_SET_XYZ(CreateGSScreenCoordinates(right, +), CreateGSScreenCoordinates(top, +), 0xFFFFFF), GIF_SET_RGBAQ(red, green, blue, alpha, 1));

        DrawPairU64(GIF_SET_UV(u1, v1), GIF_SET_XYZ(CreateGSScreenCoordinates(right, +), CreateGSScreenCoordinates(bottom, +), 0xFFFFFF));

        lastx += (letterwidth);
    }

   // EndVifImmediate();

    return 0;
}
