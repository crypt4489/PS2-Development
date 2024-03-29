#include "io/ps_texture_io.h"

#include <png.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "io/ps_file_io.h"
#include "textures/ps_texture.h"
#include "log/ps_log.h"
#include "gs/ps_gs.h"

void LoadBitmap(u8 *buffer, Texture *tex, unsigned char useAlpha, unsigned char alpha)
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

    if (image_depth == 8)
    {
        tex->psm = GS_PSM_8;
    }
    else if (image_depth == 24)
    {
        tex->psm = GS_PSM_24;
    }
    else if (image_depth == 32)
    {
        tex->psm = GS_PSM_32;
    }
    else
    {
        ERRORLOG("unsupported bit depth %d", image_depth);
        return;
    }

    unsigned char temp = 0;

    int clut[256], swizz_clut[256];

    if (image_depth == 8)
    {
        tex->clut_buffer = memalign(32, 1024); //(unsigned char *)malloc(4 * 256);
        memcpy(clut, iter, sizeof(int) * 256);

        for (int i = 0; i < 256; i++)
        {
            int index = (i & 231) + ((i & 8) << 1) + ((i & 16) >> 1);
            unsigned char *pPtr = (unsigned char *)&clut[i];
            temp = pPtr[0];
            pPtr[0] = pPtr[2];
            pPtr[2] = temp;
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
            ((u32 *)swizz_clut)[index] = clut[i];
        }

        for (int i = 0; i < 256; i++)
        {
            ((int *)tex->clut_buffer)[i] = swizz_clut[i];
        }
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

    tex->pixels = memalign(32, image_size); //(unsigned char *)malloc(sizeof(unsigned char) * image_size);

    unsigned int uLine;
    unsigned int bottomLine = tex->height - 1;
    int i = 0;

    int bytesPerRow = tex->width * (bmih.biBitCount / 8);

    int offset = 0; // uLine * bytesPerRow;

    uLine = bottomLine;

    u8 *currline = buffer + ((bmfh.bfOffBits) + (uLine * bytesPerRow));

    for (i = 0; i <= bottomLine; i++)
    {

        memcpy(&(tex->pixels[offset]), currline, bytesPerRow);

        --uLine;

        currline = buffer + ((bmfh.bfOffBits) + (uLine * bytesPerRow));

        offset += bytesPerRow;
    }

    temp = 0;

    if (image_depth == 24 || image_depth == 32)
    {
        int stride = (image_depth == 24) ? 3 : 4;
        for (int j = 0; j < image_size; j += stride)
        {
            temp = tex->pixels[j];
            tex->pixels[j] = tex->pixels[j + 2];
            tex->pixels[j + 2] = temp;
        }
    }
}

void LoadPngRedux(u8 *data, Texture *tex, u32 size)
{
    png_image image;
    memset(&image, 0, sizeof(png_image));
    image.version = PNG_IMAGE_VERSION;
}

void LoadPng(u8 *data, Texture *tex, u32 size)
{
    png_image image;
    memset(&image, 0, sizeof(png_image));
    image.version = PNG_IMAGE_VERSION;

    if (!(png_image_begin_read_from_memory(&image, data, size)))
    {
        ERRORLOG("cannot decode png image!");
        return;
    }

    tex->width = image.width;
    tex->height = image.height;

    tex->psm = GS_PSM_32;
    image.format = PNG_FORMAT_RGBA;

    tex->pixels = (unsigned char *)malloc(PNG_IMAGE_SIZE(image));

    if (png_image_finish_read(&image, NULL, tex->pixels, 0, NULL) != 0)
    {
        return;
    }

    if (tex)
    {
        free(tex->pixels);
        free(tex);
        ERRORLOG("couldn't write png to memory.");
    }

    return;
}

void CreateTextureFromFile(void* object, void* arg, u8 *buffer, u32 bufferLen)
{
    Texture *tex = (Texture*)object;

    CreateTextureParams *params = (CreateTextureParams*)arg;

    AddStringNameToTexture(tex, params->name);
    tex->clut_buffer = NULL;
    tex->pixels = NULL;

    if (params->readType == READ_BMP)
    {
        LoadBitmap(buffer, tex, params->useAlpha, params->alpha);
    }
    else if (params->readType == READ_PNG)
    {
        LoadPng(buffer, tex, bufferLen);
    }
}

Texture* ReadTexFile(const char *fileName, char *nameOfTex, u32 readType, u8 alpha, u8 useAlpha)
{
    u32 size;

    u8 *buffer = ReadFileInFull(fileName, &size);

    if (buffer == NULL)
    {
        ERRORLOG("Texture file returned empty %s", fileName);
        return NULL;
    }

    Texture *tex = (Texture*)malloc(sizeof(Texture));

    CreateTextureParams params;

    params.name = nameOfTex;
    params.readType = readType;
    params.alpha = alpha;
    params.useAlpha = useAlpha;

    CreateTextureFromFile(tex, &params, buffer, size);

    free(buffer);

    return tex;
}
