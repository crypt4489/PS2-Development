#include "graphics/ps_rendertarget.h" 

#include <stdlib.h>

#include "gamemanager/ps_manager.h"
#include "gs/ps_gs.h"
#include "gs/ps_vrammanager.h"

void DestroyRenderTarget(RenderTarget *target, bool system)
{
	if (target)
	{
        if (!system)
		    RemoveRenderTargetVRAMManager(g_Manager.vramManager, target);
		if (target->render)
			free(target->render);
		if (target->z)
			free(target->z);
		free(target);
	}
}

Texture *CreateTextureFromRenderTarget(RenderTarget *target, u32 filter, u32 function)
{
	Texture *tex = (Texture *)malloc(sizeof(Texture));
	tex->psm = GS_PSM_32;
	tex->width = target->render->width;
	tex->height = target->render->height;
	tex->texbuf.address = target->render->address;
	tex->mode = TEX_ADDRESS_WRAP;
	tex->type = PS_TEX_VRAM;
	CreateTexStructs(tex, tex->width, tex->psm, TEXTURE_COMPONENTS_RGBA, function, filter);
	return tex;
}

void SetupRenderTarget(RenderTarget *target, int context, bool wait)
{
	CalculateTextureBasePointer(g_Manager.vramManager, target);

	int hHeight, hWidth;
	hHeight = target->render->height >> 1;
	hWidth = target->render->width >> 1;

	PollVU1DoneProcessing(&g_Manager);

	InitDrawingEnvironment(target->render, target->z, hHeight, hWidth, context);
}

RenderTarget *AllocRenderTarget(bool useZBuffer)
{
	RenderTarget *target = (RenderTarget *)malloc(sizeof(RenderTarget));
	target->render = (framebuffer_t *)malloc(sizeof(framebuffer_t));
	if (useZBuffer) target->z = (zbuffer_t *)malloc(sizeof(zbuffer_t));
	return target;
}

RenderTarget *CreateRenderTarget(int height, int width, int zsm, int zmethod, int psm)
{
	RenderTarget *target = AllocRenderTarget(true);

	InitFramebuffer(target->render, width, height, psm, false);

	InitZBuffer(target->z, width, height, zsm, zmethod, false);

	target->memInVRAM = (graph_vram_size(width, height, psm, GRAPH_ALIGN_PAGE) 
									   +  graph_vram_size(width, height, zsm, GRAPH_ALIGN_PAGE));

	AddRenderTargetVRAMManager(g_Manager.vramManager, target);

	return target;
}