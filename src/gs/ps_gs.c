#include "gs/ps_gs.h"

#include <graph.h>
#include <kernel.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

#include "gamemanager/ps_manager.h"
#include "dma/ps_dma.h"
#include "system/ps_vif.h"

static int renderTargetVRAM = 0;

void InitGS(GameManager *manager, framebuffer_t *frame, zbuffer_t *z, int context)
{
	// Define a 32-bit 640x480 framebuffer.
	frame->width = manager->ScreenWidth;
	frame->height = manager->ScreenHeight;
	frame->mask = 0;
	frame->psm = GS_PSM_32;
	frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);

	// Enable the zbuffer.
	z->enable = DRAW_ENABLE;
	z->mask = 0;
	z->method = ZTEST_METHOD_GREATER_EQUAL;
	z->zsm = GS_ZBUF_24;
	z->address = graph_vram_allocate(frame->width, frame->height, z->zsm, GRAPH_ALIGN_PAGE);

	manager->bgkc.r = 0x80;
	manager->bgkc.g = 0x70;
	manager->bgkc.b = 0x05;
	manager->bgkc.a = 0x00;
	manager->bgkc.q = 0.0f;

	// Set a default interlaced video mode with flicker filter.
	graph_set_mode(GRAPH_MODE_INTERLACED, graph_get_region(), GRAPH_MODE_FIELD, GRAPH_ENABLE);

	// Set the screen up
	graph_set_screen(0, 0, frame->width, frame->height);

	// Set black background
	graph_set_bgcolor(manager->bgkc.r, manager->bgkc.g, manager->bgkc.b);

	// graph_set_framebuffer_filtered(frame->address, frame->width, frame->psm, 0, 0);

	graph_enable_output();
}

void InitFramebuffer(framebuffer_t *frame, int width, int height, int psm)
{
	frame->width = width;
	frame->height = height;
	frame->mask = 0;
	frame->psm = psm;
	frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);
}

void CreateTexBuf(Texture *texture, int width, int psm)
{
	// Allocate some vram for the texture buffer
	texture->texbuf.width = width;
	texture->texbuf.psm = psm;
	texture->texbuf.address = graph_vram_allocate(texture->texbuf.width, texture->texbuf.width, texture->texbuf.psm, GRAPH_ALIGN_BLOCK);
}

void CreateClutBuf(clutbuffer_t *clut, int width, int psm)
{
	clut->start = 0;
	clut->load_method = CLUT_LOAD;
	clut->psm = psm;
	clut->storage_mode = CLUT_STORAGE_MODE1;
	clut->address = graph_vram_allocate(width, width, psm, GRAPH_ALIGN_BLOCK);
}

void LoadFrameBuffer(framebuffer_t *frame, unsigned char *pixels, int width, int height, int psm)
{

	packet_t *packet = packet_init(50, PACKET_NORMAL);

	qword_t *q = packet->data;

	q = draw_texture_transfer(q, pixels, width, height, psm, frame->address, frame->width);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);

	packet_free(packet);
}

qword_t *SetFrameBufferMask(qword_t *q, framebuffer_t *frame, u32 mask, u32 context)
{
	PACK_GIFTAG(q, GS_SET_FRAME(frame->address >> 11, frame->width >> 6, frame->psm, mask), GS_REG_FRAME + context);
	q++;
	return q;
}

qword_t *SetZBufferMask(qword_t *q, zbuffer_t *z, u32 mask, u32 context)
{
	PACK_GIFTAG(q, GS_SET_ZBUF(z->address >> 11, z->zsm, mask), GS_REG_ZBUF + context);
	q++;
	return q;
}

qword_t *CreateGSSetTag(qword_t *q, u32 count, u32 eop, u32 type, u32 nreg, u32 regaddr)
{
	PACK_GIFTAG(q, GIF_SET_TAG(count, eop, 0, 0, type, nreg), regaddr);
	q++;
	return q;
}

void InitDrawingEnvironment(framebuffer_t *frame, zbuffer_t *z, int hheight, int hwidth, int context, int waitFinish)
{

	qword_t *q = InitializeDMAObject();

	qword_t *dmatag = q;

	q = CreateDMATag(q, DMA_CNT, 0, 0, 0, 0);

	// This will setup a default drawing environment.

	q = draw_setup_environment(q, context, frame, z);

	// PACK_GIFTAG(q,GS_SET_COLCLAMP(GS_DISABLE),GS_REG_COLCLAMP);
	// q++;

	// Now reset the primitive origin to 2048-width/2,2048-height/2.
	q = draw_primitive_xyoffset(q, context, (2048 - hwidth), (2048 - hheight));

	// Finish setting up the environment.
	q = draw_finish(q);

	// Now send the packet, no need to wait since it's the first.

	AddSizeToDMATag(dmatag, q - dmatag - 1);

	SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
}

void CopyVRAMToVRAM(int srcAd, int srcH, int srcW, int dstAd, int dstH, int dstW, int psm)
{

	packet_t *packet = packet_init(5000, PACKET_NORMAL);

	qword_t *q = packet->data;

	int address1 = srcAd;
	int width1 = srcW;
	int height1 = srcH;

	int address2 = dstAd;
	int width2 = dstW;

	DMATAG_CNT(q, 5, 0, 0, 0);
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(address1 >> 6, width1 >> 6, psm, address2 >> 6, width2 >> 6, psm), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(width1, height1), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(2), GS_REG_TRXDIR);
	q++;

	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

void CopyVRAMToMemory(int address, int width, int height, int x, int y, int psm, u32 *buffer)
{
	volatile u32 *vif1_stat = (volatile u32 *)0x10003c00;
	packet_t *pack = packet_init(1, PACKET_NORMAL);
	packet_t *packet = packet_init(10, PACKET_NORMAL);
	qword_t *q = packet->data;
	qword_t *q1 = pack->data;
	u32 prevImr = 0;
	u32 qWordSize = (width * height * 4);
	q1->sw[3] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
	q1->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
	q1->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
	q1->sw[0] = VIF_CODE(0, 0, VIF_CMD_MSKPATH3, 0);
	q1++;

	q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
	q->sw[1] = VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0);
	q->sw[2] = VIF_CODE(0, 0, VIF_CMD_FLUSHA, 0);
	q->sw[3] = VIF_CODE(6, 0, VIF_CMD_DIRECT, 0);
	q++;

	PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(address >> 6, width >> 6, psm, 0, 0, psm), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(x, y, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(width, height), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, 0, GS_REG_FINISH);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(1), GS_REG_TRXDIR);
	q++;
	prevImr = GsPutIMR(GsGetIMR() | 0x0200);
	*GS_REG_CSR |= 2;

	dma_channel_wait(DMA_CHANNEL_VIF1, -1);

	asm __volatile__("di\n");

	FlushCache(0);

	dma_channel_send_normal(DMA_CHANNEL_VIF1, packet->data, q - packet->data, 0, 0);

	asm __volatile__(" sync.l \n");

	dma_channel_wait(DMA_CHANNEL_VIF1, -1);

	while (!(*GS_REG_CSR & 2))
		;

	asm __volatile__("ei\n");

	while ((*vif1_stat & (0x1f000000)))
		; // wait for fifo to be empty

	*vif1_stat |= 0x00800000; // flip bus direction
	*GS_REG_BUSDIR ^= 1;

	dma_channel_receive_chain(DMA_CHANNEL_VIF1, buffer, qWordSize, 0, 0);

	asm __volatile__(" sync.l \n");

	dma_channel_wait(DMA_CHANNEL_VIF1, -1);

	*vif1_stat &= 0x00000000;
	*GS_REG_BUSDIR ^= 1;

	GsPutIMR(prevImr);
	*GS_REG_CSR = 0x02;

	dma_channel_send_normal(DMA_CHANNEL_VIF1, pack->data, q1 - pack->data, 0, 0);
	dma_channel_wait(DMA_CHANNEL_VIF1, -1);

	FlushCache(0);

	packet_free(pack);
	packet_free(packet);
}

void CreateClutStructs(Texture *tex, int width, int psm)
{
	tex->clut.start = 0;
	tex->clut.load_method = CLUT_LOAD;
	tex->clut.psm = psm;
	tex->clut.storage_mode = CLUT_STORAGE_MODE1;
	tex->clut.address = g_Manager.textureInVram->clut.address;
}

void CreateTexStructs(Texture *tex, int width, int psm, u32 components, u32 function, u32 texfilter)
{

	if (texfilter == 0)
	{
		tex->lod.mag_filter = LOD_MAG_NEAREST;
		tex->lod.min_filter = LOD_MIN_NEAREST;

		tex->lod.l = 0;
		tex->lod.k = 0;
		tex->lod.calculation = LOD_USE_K;
		tex->lod.max_level = 0;
	}
	else
	{
		tex->lod.mag_filter = LOD_MAG_LINEAR;
		tex->lod.min_filter = LOD_MIN_LINEAR;

		tex->lod.l = 0;
		tex->lod.k = 0.0f;
		tex->lod.calculation = LOD_USE_K;
		tex->lod.max_level = 0;
	}

	tex->texbuf.info.width = draw_log2(width);
	tex->texbuf.info.height = draw_log2(width);
	tex->texbuf.info.components = components;
	tex->texbuf.info.function = function;

	tex->texbuf.width = width;
	tex->texbuf.psm = psm;

	tex->texbuf.address = g_Manager.textureInVram->texbuf.address;
}

void SetupRenderTarget(RenderTarget *target, int context, int wait)
{
	int hHeight, hWidth;
	hHeight = target->render->height / 2.0;
	hWidth = target->render->width / 2.0;

	while (PollVU1DoneProcessing(&g_Manager) < 0)
	{
	}

	InitDrawingEnvironment(target->render, target->z, hHeight, hWidth, context, wait);
}

RenderTarget *AllocRenderTarget()
{
	RenderTarget *target = (RenderTarget *)malloc(sizeof(RenderTarget));
	target->render = (framebuffer_t *)malloc(sizeof(framebuffer_t));
	target->z = (zbuffer_t *)malloc(sizeof(zbuffer_t));
	return target;
}

void InitZBuffer(zbuffer_t *z, int width, int height, int zsm, int method)
{
	z->enable = DRAW_ENABLE;
	z->mask = 0;
	z->method = method;
	z->zsm = zsm;
	z->address = graph_vram_allocate(width, height, z->zsm, GRAPH_ALIGN_PAGE);
}

RenderTarget *CreateRenderTarget(int height, int width, int zsm, int zmethod, int psm)
{
	RenderTarget *target = AllocRenderTarget();
	target->render->width = width;
	target->render->height = height;

	if (renderTargetVRAM == 0)
	{
		target->render->address = g_Manager.textureInVram->texbuf.address;
		renderTargetVRAM = g_Manager.textureInVram->texbuf.address;
	}
	else
	{
		target->render->address = renderTargetVRAM;
	}

	target->render->psm = psm;
	target->render->mask = 0;

	int zAddress = target->render->address + graph_vram_size(width, height, psm, GRAPH_ALIGN_PAGE);

	target->z->address = zAddress;
	target->z->enable = DRAW_ENABLE;
	target->z->method = zmethod;
	target->z->zsm = zsm;
	target->z->mask = 0;

	renderTargetVRAM = zAddress + graph_vram_size(width, height, zsm, GRAPH_ALIGN_PAGE);

	return target;
}

void DestroyRenderTarget(RenderTarget *target)
{
	free(target->render);
	free(target->z);
	free(target);
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

void ClearScreen(RenderTarget *target, int context, int r, int g, int b, int a)
{

	qword_t *q = InitializeDMAObject();

	qword_t *dmatag = q;

	q++;

	float xOff = 2048.0f - (target->render->width / 2.0);
	float yOff = 2048.0f - (target->render->height / 2.0);

	q = draw_disable_tests_alpha(q, context, a);
	q = draw_clear_alpha(q, context, xOff, yOff, target->render->width, target->render->height, r, g, b, a);
	q = draw_enable_tests_alpha(q, context, a, target->z->method);
	q = draw_finish(q);

	DMATAG_END(dmatag, q - dmatag - 1, 0, 0, 0);

	SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
	draw_wait_finish();
}

qword_t *draw_clear_alpha(qword_t *q, int context, float x, float y, float width, float height, int r, int g, int b, int a)
{

	rect_t rect;

	union
	{
		float fvalue;
		u32 ivalue;
	} q0 = {
		1.0f};

	rect.v0.x = x;
	rect.v0.y = y;
	rect.v0.z = 0x00000000;

	rect.color.rgbaq = GS_SET_RGBAQ(r, g, b, a, q0.ivalue);

	rect.v1.x = x + width - 0.9375f;
	rect.v1.y = y + height - 0.9375f;
	rect.v1.z = 0x00000000;

	PACK_GIFTAG(q, GIF_SET_TAG(2, 0, 0, 0, 0, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODECONT(PRIM_OVERRIDE_ENABLE), GS_REG_PRMODECONT);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODE(0, 0, 0, 0, 0, 0, context, 1), GS_REG_PRMODE);
	q++;

	q = draw_rect_filled_strips(q, context, &rect);

	PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, 0, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODECONT(PRIM_OVERRIDE_DISABLE), GS_REG_PRMODECONT);
	q++;

	return q;
}

qword_t *draw_disable_tests_alpha(qword_t *q, int context, int alpha)
{

	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	if (alpha == 0)
	{
		PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_EQUAL, 0x00, ATEST_KEEP_FRAMEBUFFER, DRAW_DISABLE, DRAW_DISABLE, DRAW_ENABLE, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);
	}
	else
	{
		PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_NOTEQUAL, 0x00, ATEST_KEEP_FRAMEBUFFER, DRAW_DISABLE, DRAW_DISABLE, DRAW_ENABLE, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);
	}

	q++;

	return q;
}

qword_t *draw_enable_tests_alpha(qword_t *q, int context, int alpha, u32 method)
{

	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_NOTEQUAL, 0x00, ATEST_KEEP_FRAMEBUFFER, DRAW_DISABLE, DRAW_DISABLE, DRAW_ENABLE, method), GS_REG_TEST + context);
	q++;

	return q;
}

qword_t *SetupZTestGS(qword_t *q, int z_test_method, int z_test_enable, char alphaValue, char alpha_test_method, char frameBufferTest, char alpha_test_enable, char alpha_test, int context)
{
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, alpha_test_method, alphaValue, frameBufferTest, alpha_test_enable, alpha_test, z_test_enable, z_test_method),  GS_REG_TEST + context);
	q++;
	return q;
}
qword_t *SetupRGBAQGS(qword_t *b, color_t color)
{
	PACK_GIFTAG(b, GIF_SET_RGBAQ(color.r, color.g, color.b, color.a, (int)color.q), GIF_REG_RGBAQ);
	b++;
	return b;
}
qword_t *SetupAlphaGS(qword_t *q, blend_t *blend, int context)
{
	PACK_GIFTAG(q, GS_SET_ALPHA(blend->color1, blend->color2, blend->alpha, blend->color3, blend->fixed_alpha), GS_REG_ALPHA + context);
	q++;

	/*  PACK_GIFTAG(q, GS_SET_PABE(DRAW_ENABLE), GS_REG_PABE);
	  q++; */

	return q;
}
