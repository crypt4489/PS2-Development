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

#define GS_SET_MIPTBP(TBA1, TBW1, TBA2, TBW2, TBA3, TBW3)                     \
    (u64)((TBA1) & 0x00003FFF) << 0 | (u64)((TBW1) & 0x0000003F) << 14 |      \
        (u64)((TBA2) & 0x00003FFF) << 20 | (u64)((TBW2) & 0x0000003F) << 34 | \
        (u64)((TBA3) & 0x00003FFF) << 40 | (u64)((TBW3) & 0x0000003F) << 54


qword_t* MipMapRegisters(qword_t *q, u32 *texAddresses, u32 *widths)
{
    PACK_GIFTAG(q, GS_SET_MIPTBP(texAddresses[0] >> 6, widths[0] >> 6, texAddresses[1] >> 6, widths[1] >> 6, texAddresses[2] >> 6, widths[2] >> 6), GS_REG_MIPTBP1 + g_Manager.gs_context);
    q++;

    PACK_GIFTAG(q, GS_SET_MIPTBP(texAddresses[3] >> 6, widths[3] >> 6, texAddresses[4] >> 6, widths[4] >> 6, texAddresses[5] >> 6, widths[5] >> 6), GS_REG_MIPTBP2 + g_Manager.gs_context);
    q++;
    return q;
}

static void StripFilePath(const char *filepath, char *texname)
{
    const char *temp = strrchr(filepath, 92);

    if (!temp)
        temp = filepath;
    else
        temp++;

    int i = 0;
    while (*temp) texname[i++] = *temp++;
    texname[i] = 0;
}

void AddStringNameToTexture(Texture *tex, const char *buffer)
{
    memcpy(tex->name, buffer, strnlen(buffer, MAX_CHAR_TEXTURE_NAME));
}

void CleanTextureStruct(Texture *tex)
{
    if (tex)
    {
        if (tex->clut_buffer)
        {
            free(tex->clut_buffer);
        }

        if (tex->pixels)
        {
            free(tex->pixels);
        }


        LinkedList *item = tex->mipMaps;
        for (int i = 0; i<tex->mipLevels; i++)
        {
            Texture *data = (Texture*)item->data;
            CleanTextureStruct(data);
            item = CleanLinkedListNode(item);
        }

        free(tex);
    }
}

Texture *AddAndCreateAlphaMap(const char *filePath, u32 readType, u32 mode)
{
    Texture *alphaMap = AddAndCreateTexture(filePath, readType, false, 0, mode);

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
            pixels[i + 3] = 0;
        else
            pixels[i + 3] = 0xFF;
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
    while (comp != toRemove && iter)
    {
        iter = iter->next;
        comp = (Texture *)iter->data;
    }

    if (iter) {
        tex->mipMaps = RemoveNodeFromList(tex->mipMaps, iter);
        tex->mipLevels -= 1;
    }

    //INFOLOG("found");
    
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

        //tex->upload = (qword_t *)malloc(sizeof(qword_t) * 50);
    }
}

Texture *AddAndCreateTextureFromBuffer(u8 *fileData, u32 size, 
                                       const char *nameOfTex, u32 readType, 
                                       bool useAlpha, u8 alpha, 
                                       u32 mode)
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

Texture *AddAndCreateTexture(const char *filePath, u32 readType, 
bool useProgrammedAlpha, u8 alphaVal, u32 mode)
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

qword_t *FlushTextureCache(qword_t *q, bool end, bool direct, int size)
{
    if (end) q = CreateDMATag(q, DMA_END, 2+size+direct, 0, 0, 0);
    else q = CreateDMATag(q, DMA_CNT, 2+size+direct, 0, 0, 0);
    if (direct) q = CreateDirectTag(q, 2+size, end);
	PACK_GIFTAG(q,GIF_SET_TAG(1+size,end,0,0,GIF_FLG_PACKED,1),GIF_REG_AD);
	q++;
	PACK_GIFTAG(q,1,GS_REG_TEXFLUSH);
	q++;

	return q;
}

static int UploadSize(int height, int width, int psm, int *remaining)
{
    int i;
	int qwords = 0;

	switch (psm)
	{
		case GS_PSM_8:
		{
			qwords = (width*height)>>4;
			break;
		}

		case GS_PSM_32:
		case GS_PSM_24:
		{
			qwords = (width*height)>>2;
			break;
		}

		case GS_PSM_4:
		{
			qwords = (width*height)>>5;
			break;
		}

		case GS_PSM_16:
		case GS_PSM_16S:
		{
			qwords = (width*height)>>3;
			break;
		}

		default:
		{
			switch (psm)
			{
				case GS_PSM_8H:
				{
					qwords = (width*height)>>4;
					break;
				}

				case GS_PSMZ_32:
				case GS_PSMZ_24:
				{
					qwords = (width*height)>>2;
					break;
				}

				case GS_PSMZ_16:
				case GS_PSMZ_16S:
				{
					qwords = (width*height)>>3;
					break;
				}

				case GS_PSM_4HL:
				case GS_PSM_4HH:
				{
					qwords = (width*height)>>5;
					break;
				}
			}
			break;
		}
	}

    i = qwords / GIF_BLOCK_SIZE;

	*remaining  = qwords % GIF_BLOCK_SIZE;

    return i;
}

qword_t *TextureTransfer(qword_t *q, void *src, int width, int height, 
                         int psm, int dest, int dest_width, bool usevif)
{
    int directacct = (int)usevif;
    int remaining = 0;
    int i = UploadSize(height, width, psm, &remaining);
    q = CreateDMATag(q, DMA_CNT, 5+directacct, 0, 0, 0);
    if (usevif)
        q = CreateDirectTag(q, 5, 0);
    q = CreateGSSetTag(q, 4, 0, GIF_FLG_PACKED, 1, GIF_REG_AD);    
    PACK_GIFTAG(q,GS_SET_BITBLTBUF(0,0,0,dest>>6,dest_width>>6,psm),GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q,GS_SET_TRXPOS(0,0,0,0,0),GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q,GS_SET_TRXREG(width,height),GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q,GS_SET_TRXDIR(0),GS_REG_TRXDIR);
    q++;

    while (i-- > 0)
    {
        q= CreateDMATag(q, DMA_CNT, 1+directacct, 0, 0, 0);

        if (usevif)
            q = CreateDirectTag(q, GIF_BLOCK_SIZE+1, 0);

        q = CreateGSSetTag(q, GIF_BLOCK_SIZE, 0, GIF_FLG_IMAGE, 0, 0);

        q = CreateDMATag(q, DMA_REF, GIF_BLOCK_SIZE, 0, 0, 0, (u32)src);
        src = (void *)((u8 *)src + (GIF_BLOCK_SIZE*16));
    }

    if (remaining)
    {
        q = CreateDMATag(q, DMA_CNT, 1+directacct, 0, 0, 0);

        if (usevif)
            q = CreateDirectTag(q, remaining+1, 0);

        q = CreateGSSetTag(q, remaining, 0, GIF_FLG_IMAGE, 0, 0);

        q = CreateDMATag(q, DMA_REF, remaining, 0, 0, 0, (u32)src);
    }

    return q;
}

static qword_t *TextureUploadGS(qword_t *q, Texture *tex, u32 texAddress, u32 clutAddress, bool usevif)
{
    if ((tex->psm == GS_PSM_8 || tex->psm == GS_PSM_4))
    {
        int width = (tex-> psm  == GS_PSM_8) ? 16 : 8;
        int height = (tex-> psm  == GS_PSM_8) ? 16 : 2;
        q = TextureTransfer(q, tex->clut_buffer, width, height, tex->clut.psm, clutAddress, 16, usevif);
        q = FlushTextureCache(q, false, usevif, 0);
    }

    q = TextureTransfer(q, tex->pixels, tex->width, tex->height, tex->psm, texAddress, tex->texbuf.width, usevif);
    
    return q;
}

static void BaseTextureAddresses(u32 *texAddress, u32 *clutAddress, u16 mipSize, u32 height, u32 width, u32 bpp)
{
    bool clut = false;
    int cw = 0, ch = 0;
    if (bpp == GS_PSM_4 || bpp == GS_PSM_8) 
    {
        cw = (bpp  == GS_PSM_8) ? 16 : 8;
        ch = (bpp  == GS_PSM_8) ? 16 : 2;
        clut = true;
    }

    int base = g_Manager.vramManager->currentTextureBasePtr;
   
    for (int i = 0; i<mipSize+1; i++, height>>=1, width>>=1)
    {
        texAddress[i] = base;
        base += graph_vram_size(width, height, bpp, GRAPH_ALIGN_BLOCK);
        if (!i && clut)
        {
            *clutAddress = base;
            base += graph_vram_size(cw, ch, GS_PSM_32, GRAPH_ALIGN_BLOCK);
        }
    }

}

qword_t *SetupTextureGSState(qword_t *q, Texture *tex, int size, u32 *texaddresses, u32 clutAddress, u32 *widths)
{
    
    q = SetTextureWrap(q, tex->mode);
    q = SetTextureRegisters(q, &tex->lod, &tex->texbuf, &tex->clut, texaddresses[0], clutAddress); 

    if (size>3)
    {
        q = MipMapRegisters(q, texaddresses+1, widths);
    }

    return q;
}

qword_t *CreateTextureUploadChain(Texture *tex, qword_t *q, bool direct, bool end)
{
    int size = 3;
    u32 texaddrs[7], clutaddrs = 0, widths[6];
    u16 mipSize = tex->mipLevels;
    BaseTextureAddresses(texaddrs, &clutaddrs, mipSize, tex->height, tex->width, tex->psm);    
    LinkedList *iter = tex->mipMaps;
    q = TextureUploadGS(q, tex, texaddrs[0], clutaddrs, direct);
    if (mipSize > 0) {
        q = FlushTextureCache(q, false, direct, 0);
        size += 2;
    }

    iter = tex->mipMaps;
    for (int i = 0; i<mipSize; i++)
    {
        Texture *mip = (Texture*)iter->data;
        widths[i] = mip->texbuf.width;
         q = TextureTransfer(q, mip->pixels, mip->width, mip->height, mip->psm, 
            texaddrs[i+1], mip->texbuf.width, direct);
        if (i<(mipSize-1))
            q = FlushTextureCache(q, false, direct, 0);
        iter = iter->next;
    }      

    q = FlushTextureCache(q, end, direct, size);

    q = SetupTextureGSState(q, tex, size, texaddrs, clutaddrs, widths);

    return q;

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
    u32 texaddrs[7], clutaddrs = 0, widths[6];
    u16 mipSize = tex->mipLevels;
    BaseTextureAddresses(texaddrs, &clutaddrs, mipSize, tex->height, tex->width, tex->psm);  
    qword_t *q = GetDMABasePointer();

    q = FlushTextureCache(q, true, false, 3);

    q = SetupTextureGSState(q, tex, 3, texaddrs, clutaddrs, widths);

    SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
}



void UploadTextureToVRAM(Texture *tex)
{
    qword_t *q = GetDMABasePointer();
    qword_t *dcodetag = q++;
    q = CreateTextureUploadChain(tex, q, false, true); 
    u32 size = q - dcodetag -1;
    CreateDCODEDmaTransferTag(dcodetag, DMA_CHANNEL_GIF, 0, 1, size);  
    CreateDCODETag(q, DMA_DCODE_END);
    SubmitDMABuffersAsPipeline(q, NULL);
    g_Manager.texManager->currIndex = tex->id;
}

void UploadTextureViaManagerToVRAM(Texture *tex)
{
    TextureManager *texManager = g_Manager.texManager;

    if (tex->id != texManager->currIndex && tex->type == PS_TEX_MEMORY)
    {
        qword_t *q = GetDMABasePointer();
        qword_t *dcodetag = q++;
        q = CreateTextureUploadChain(tex, q, true, true); 
        u32 size = q - dcodetag -1;
        CreateDCODEDmaTransferTag(dcodetag, DMA_CHANNEL_VIF1, 0, 1, size);  
        CreateDCODETag(q, DMA_DCODE_END);
        SubmitDMABuffersAsPipeline(q, NULL);
        texManager->currIndex = tex->id;
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
