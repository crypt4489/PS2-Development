#include "io/ps_texture_io.h"

#include <png.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "io/ps_file_io.h"
#include "textures/ps_texture.h"
#include "log/ps_log.h"
#include "gs/ps_gs.h"

static inline void BGR2RGB(u8 *buffer, u32 stride, u32 size)
{
    u8 temp;
    for (int i = 0; i < size; i += stride)
    {
        u8 *pPtr = &buffer[i];
        temp = pPtr[0];
        pPtr[0] = pPtr[2];
        pPtr[2] = temp;
    }
}

static inline void Swizzle(int *out, int *clut,
                    u8 useAlpha, u8 alpha)
{
    for (int i = 0; i < 256; i++)
    {
        int index = (i & 231) + ((i & 8) << 1) + ((i & 16) >> 1);
        u8 *pPtr = (u8 *)&out[index];
        if (useAlpha)
        {
            pPtr[3] = alpha;
        }
        else
        {
            if ((pPtr[0] == alpha) && (pPtr[1] == alpha) && (pPtr[2] == alpha))
            {
                pPtr[3] = 0;
            }
        }
        memcpy(&out[index], &clut[i], 3);
    }
}

static inline void CopyColorMap(u8 *colormap, u8 *copyTo, u32 size, u32 clutStride)
{
    for (int i = 0, k = 0; i < size; k += 4, i += clutStride)
    {
        memcpy(&copyTo[k], &colormap[i], clutStride);
        if (clutStride == 3)
        {
            copyTo[k + 3] = 0xFF;
        }
    }
}

void LoadBitmap(u8 *buffer, Texture *tex, bool useAlpha, unsigned char alpha)
{
    BitmapFileHeader bmfh;
    BitmapInfoHeader bmih;

    u8 *iter = buffer;

    memcpy(&bmfh, iter, sizeof(BitmapFileHeader));

    if (bmfh.bfType != BITMAP_ID)
    {
        ERRORLOG("not a valid bitmap file");
        return;
    }

    iter += sizeof(BitmapFileHeader);

    memcpy(&bmih, iter, sizeof(BitmapInfoHeader));

    iter += sizeof(BitmapInfoHeader);

    tex->width = bmih.biWidth;
    tex->height = bmih.biHeight;

    int image_depth = bmih.biBitCount;

    switch (image_depth)
    {
    case 4:
        tex->psm = GS_PSM_4;
        break;
    case 8:
        tex->psm = GS_PSM_8;
        break;
    case 24:
        tex->psm = GS_PSM_24;
        break;
    case 32:
        tex->psm = GS_PSM_32;
        break;
    default:
        ERRORLOG("unsupported bit depth %d", image_depth);
        return;
    }

    if (image_depth == 8)
    {
        tex->clut_buffer = memalign(32, 1024);
        BGR2RGB(iter, 4, 4 * 256);
        Swizzle((int *)tex->clut_buffer, (int *)iter, useAlpha, alpha);
    }
    else if (image_depth == 4)
    {
        tex->clut_buffer = memalign(32, 64);
        BGR2RGB(iter, 4, 4 * 16);
        memcpy(tex->clut_buffer, iter, 64);
    }

    int image_size;

    if (bmih.biSizeImage != 0)
    {
        image_size = bmih.biSizeImage;
    }
    else
    {
        image_size = bmfh.bfSize - bmfh.bfOffBits;
    }

    tex->pixels = memalign(32, image_size); 

    unsigned int uLine;
    unsigned int bottomLine = tex->height - 1;
    int i = 0;

    int bytesPerRow = tex->width * ((float)bmih.biBitCount / 8);

    int offset = 0;

    uLine = bottomLine;

    u8 *currline = buffer + ((bmfh.bfOffBits) + (uLine * bytesPerRow));

    for (i = 0; i <= bottomLine; i++)
    {

        memcpy(&(tex->pixels[offset]), currline, bytesPerRow);

        --uLine;

        currline = buffer + ((bmfh.bfOffBits) + (uLine * bytesPerRow));

        offset += bytesPerRow;
    }

    if (image_depth == 24 || image_depth == 32)
    {
        int stride = (image_depth == 24) ? 3 : 4;
        BGR2RGB(tex->pixels, stride, image_size);
    }
}

void LoadPng(u8 *data, Texture *tex, u32 size, bool useAlpha, u8 alphaVal)
{
    u8 colormap[1024];
    png_image image;
    memset(&image, 0, sizeof(png_image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&image, data, size))
    {
        ERRORLOG("Error processing png!");
        return;
    }

    tex->width = image.width;
    tex->height = image.height;
    int imageSize = PNG_IMAGE_SIZE(image);

    int clutStride = 4, texStride = 4, useBGR = 0;

    switch (image.format & 0x0F)
    {
    case PNG_FORMAT_RGB:
        tex->pixels = (u8 *)malloc(imageSize * 3);
        tex->psm = GS_PSM_24;
        texStride = 3;
        break;
    case PNG_FORMAT_RGBA:
        tex->pixels = (u8 *)malloc(imageSize * 4);
        tex->psm = GS_PSM_32;
        break;
    case PNG_FORMAT_RGB_COLORMAP:
        clutStride = 3;
    case PNG_FORMAT_RGBA_COLORMAP:
        tex->pixels = (u8 *)malloc(imageSize * 2);
        if (image.colormap_entries == 256)
        {
            tex->clut_buffer = (u8 *)malloc(256 * 4);
            tex->psm = GS_PSM_8;
        }
        else
        {
            tex->clut_buffer = (u8 *)malloc(4 * 16);
            tex->psm = GS_PSM_4;
        }
        break;
    default:
        ERRORLOG("Unsupported bit depth and color format");
        return;
    }

    if (!png_image_finish_read(&image, NULL, tex->pixels, 0, colormap))
    {
        ERRORLOG("Cannot load png image");
        free(tex->pixels);
        if (tex->clut_buffer)
            free(tex->clut_buffer);
        return;
    }

    if (useBGR)
    {
        if ((tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4) && useBGR)
            BGR2RGB(colormap, clutStride, image.colormap_entries * clutStride);
        else
            BGR2RGB(tex->pixels, texStride, imageSize * texStride);
    }

    if (tex->psm == GS_PSM_8)
    {
        u8 clut[256 * 4];

        CopyColorMap(clut, colormap, image.colormap_entries * clutStride, clutStride);

        Swizzle((int *)tex->clut_buffer, (int *)clut, useAlpha, alphaVal);
    }
    else if (tex->psm == GS_PSM_4)
    {
        for (int i = 0, k = 0; i < imageSize; i++, k += 2)
        {
            tex->pixels[i] = (tex->pixels[k] << 4) | tex->pixels[k + 1];
        }

        tex->pixels = (u8*)realloc(tex->pixels, imageSize>>1);

        CopyColorMap(colormap, tex->clut_buffer, image.colormap_entries * clutStride, clutStride);
    }
}

void CreateTextureFromFile(void *object, void *arg, u8 *buffer, u32 bufferLen)
{
    Texture *tex = (Texture *)object;

    CreateTextureParams *params = (CreateTextureParams *)arg;

    AddStringNameToTexture(tex, params->name);
    tex->clut_buffer = NULL;
    tex->pixels = NULL;

    if (params->readType == READ_BMP)
    {
        LoadBitmap(buffer, tex, params->useAlpha, params->alpha);
    }
    else if (params->readType == READ_PNG)
    {
        LoadPng(buffer, tex, bufferLen, params->useAlpha, params->alpha);
    }
}

Texture *ReadTexFile(const char *fileName, char *nameOfTex, u32 readType, u8 alpha, bool useAlpha)
{
    u32 size;

    u8 *buffer = ReadFileInFull(fileName, &size);

    if (buffer == NULL)
    {
        ERRORLOG("Texture file returned empty %s", fileName);
        return NULL;
    }

    Texture *tex = (Texture *)malloc(sizeof(Texture));

    CreateTextureParams params;

    params.name = nameOfTex;
    params.readType = readType;
    params.alpha = alpha;
    params.useAlpha = useAlpha;

    CreateTextureFromFile(tex, &params, buffer, size);

    free(buffer);

    return tex;
}
