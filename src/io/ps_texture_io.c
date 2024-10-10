#include "io/ps_texture_io.h"

#include <png.h>

#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "io/ps_file_io.h"
#include "textures/ps_texture.h"
#include "log/ps_log.h"
#include "gs/ps_gs.h"

static void BGR2RGB(u8 *buffer, u32 stride, u32 size)
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

static void Swizzle(int *out, int *clut,
                    bool useAlpha, u8 alpha)
{
    for (int i = 0; i < 256; i++)
    {
         int index = (i & 231) + ((i & 8) << 1) + ((i & 16) >> 1);
        u8* pPtr = (u8*)&out[index];
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

static void CopyColorMap(u8 *colormap, u8 *copyTo, u32 size, u32 clutStride)
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
    int imageOffset = bmfh.bfOffBits;

    int image_size;

    if (bmih.biSizeImage)
    {
        image_size = bmih.biSizeImage;
    }
    else
    {
        image_size = bmfh.bfSize - bmfh.bfOffBits;
    }

    int bytesPerRow;

    switch (image_depth)
    {
    case 4:
        tex->psm = GS_PSM_4;
        bytesPerRow = tex->width>>1;
        break;
    case 8:
        tex->psm = GS_PSM_8;
        bytesPerRow = tex->width;
        break;
    case 24:
        tex->psm = GS_PSM_24;
        bytesPerRow = tex->width*3;
        break;
    case 32:
        tex->psm = GS_PSM_32;
        bytesPerRow = tex->width*4;
        break;
    default:
        ERRORLOG("unsupported bit depth %d", image_depth);
        return;
    }

    if (image_depth == 8)
    {
        tex->clut_buffer = (u8*)memalign(128, 1024);
        BGR2RGB(iter, 4, 4 * 256);    
        Swizzle((int *)tex->clut_buffer, (int *)iter, useAlpha, alpha);
    }
    else if (image_depth == 4)
    {
        tex->clut_buffer = (u8*)memalign(128, 64);
        BGR2RGB(iter, 4, 4 * 16);
        memcpy(tex->clut_buffer, iter, 64);
    }
    
    tex->pixels = (u8*)memalign(128, image_size); 

    unsigned int uLine;
    unsigned int bottomLine = tex->height - 1;
    int i = 0;

    int offset = 0;

    uLine = bottomLine;

    u8 *currline = buffer + ((imageOffset) + (uLine * bytesPerRow));

    for (i = 0; i <= bottomLine; i++)
    {
        u8 *copy = tex->pixels+offset;

        for (int j = 0; j<bytesPerRow; j++) copy[j] = currline[j];

        currline -= bytesPerRow;

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

    u8 *colormap = NULL;
    u8 *clut = NULL;

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
        tex->pixels = (u8 *)memalign(128, imageSize);
        tex->psm = GS_PSM_24;
        texStride = 3;
        break;
    case PNG_FORMAT_RGBA:
        tex->pixels = (u8 *)memalign(128, imageSize);
        tex->psm = GS_PSM_32;
        break;
    case PNG_FORMAT_RGB_COLORMAP:
        clutStride = 3;
    case PNG_FORMAT_RGBA_COLORMAP:
        if (image.colormap_entries == 256)
        {
            tex->clut_buffer = (u8 *)memalign(128, 256 * 4);
            tex->psm = GS_PSM_8;
        }
        else
        {
            tex->clut_buffer = (u8 *)memalign(128, 4 * 16);
            tex->psm = GS_PSM_4;
        }
        tex->pixels = (u8 *)memalign(128, imageSize);
        colormap = (u8 *)memalign(128, 1024);
        break;
    default:
        ERRORLOG("Unsupported bit depth and color format");
        return;
    }

    int ret = png_image_finish_read(&image, NULL, tex->pixels, 0, colormap);

    if (!ret)
    {
        ERRORLOG("Cannot load png image %d", image.warning_or_error);
        free(tex->pixels);
        if (tex->clut_buffer) free(tex->clut_buffer);
        goto pngend;
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
        clut = (u8*)memalign(128, 1024);
        CopyColorMap(colormap, clut, image.colormap_entries * clutStride, clutStride);
        Swizzle((int *)tex->clut_buffer, (int *)clut, useAlpha, alphaVal);
        free(clut);
    }
    
    if (tex->psm == GS_PSM_4)
    {
        for (int i = 0, k = 0; i < imageSize; i++, k += 2)
        {
            tex->pixels[i] = (tex->pixels[k] << 4) | tex->pixels[k + 1];
        }

        tex->pixels = (u8*)realloc(tex->pixels, imageSize>>1);

        CopyColorMap(colormap, tex->clut_buffer, image.colormap_entries * clutStride, clutStride);
    }
pngend:
    if (colormap) free(colormap);
    png_image_free(&image);
}

void CreateTextureFromFile(void *object, void *arg, u8 *buffer, u32 bufferLen)
{
    Texture *tex = (Texture *)object;

    CreateTextureParams *params = (CreateTextureParams *)arg;

    AddStringNameToTexture(tex, params->name);
    tex->clut_buffer = NULL;
    tex->pixels = NULL;

    switch(params->readType)
    {
        case READ_BMP:
            LoadBitmap(buffer, tex, params->useAlpha, params->alpha);
            break;
        case READ_PNG:
            LoadPng(buffer, tex, bufferLen, params->useAlpha, params->alpha);
            break;
        default:
            ERRORLOG("Unhandled image type %d", params->readType);
            break;
    }
}

Texture *ReadTexFile(const char *fileName, char *nameOfTex, u32 readType, u8 alpha, bool useAlpha)
{
    u32 size;

    u8 *buffer = ReadFileInFull(fileName, &size);

    if (!buffer)
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
