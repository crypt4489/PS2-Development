#include "gs/ps_gs.h"


#include <kernel.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

#include "gamemanager/ps_manager.h"
#include "dma/ps_dma.h"
#include "system/ps_vif.h"
#include "util/ps_linkedlist.h"
#include "gs/ps_vrammanager.h"


void InitGS(GameManager *manager, framebuffer_t *frame1, framebuffer_t *frame2, zbuffer_t *z, u32 psm)
{
	InitFramebuffer(frame1, manager->ScreenWidth, manager->ScreenHeight, psm, true);

	if (z->enable) InitZBuffer(z, manager->ScreenWidth, manager->ScreenHeight, GS_ZBUF_24, ZTEST_METHOD_GREATER_EQUAL, true);

	if (frame2) InitFramebuffer(frame2, manager->ScreenWidth, manager->ScreenHeight, psm, true);
	
	manager->bgkc.r = 0x80;
	manager->bgkc.g = 0x70;
	manager->bgkc.b = 0x05;
	manager->bgkc.a = 0x00;
	manager->bgkc.q = 0.0f;

	SetGraph(manager);
}

void SetGraph(GameManager *manager)
{
	// Set a default interlaced video mode with flicker filter.
	graph_set_mode(GRAPH_MODE_INTERLACED, graph_get_region(), GRAPH_MODE_FIELD, GRAPH_ENABLE);

	// Set the screen up
	graph_set_screen(0, 0, manager->ScreenWidth, manager->ScreenHeight);

	// Set black background
	graph_set_bgcolor(manager->bgkc.r, manager->bgkc.g, manager->bgkc.b);

	// graph_set_framebuffer_filtered(frame->address, frame->width, frame->psm, 0, 0);

	graph_enable_output();
}

void InitFramebuffer(framebuffer_t *frame, int width, int height, int psm, bool systemMemory)
{
	frame->width = width;
	frame->height = height;
	frame->mask = 0;
	frame->psm = psm;
	frame->address = AllocateVRAM(g_Manager.vramManager, width, height, psm, systemMemory);
}

void InitZBuffer(zbuffer_t *z, int width, int height, int zsm, int method, bool systemMemory)
{
	z->enable = DRAW_ENABLE;
	z->mask = 0;
	z->method = method;
	z->zsm = zsm;
	z->address = AllocateVRAM(g_Manager.vramManager, width, height, zsm, systemMemory);
}

void LoadFrameBuffer(framebuffer_t *frame, unsigned char *pixels, int width, int height, int psm)
{
	qword_t *q = GetDMABasePointer();

	q = draw_texture_transfer(q, pixels, width, height, psm, frame->address, frame->width);
	q = draw_texture_flush(q);

	SubmitDMABuffersToController(q, DMA_CHANNEL_GIF, 1, 0);
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

qword_t *AddSizeToGSSetTag(qword_t *q, u32 count)
{
	q->sw[0] &= 0xFFFF8000;
	q->sw[0] |= (count & 0x00007fff);
	return q;
}

void InitDrawingEnvironment(framebuffer_t *frame, zbuffer_t *z, int hheight, int hwidth, int context)
{

	qword_t *q = GetDMABasePointer();

	qword_t *dmatag = q;

	q = CreateDMATag(q, DMA_CNT, 0, 0, 0, 0);

	q = draw_setup_environment(q, context, frame, z);

	q = draw_primitive_xyoffset(q, context, (2048 - hwidth), (2048 - hheight));

	q = draw_finish(q);

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

void CreateClutStructs(Texture *tex, int psm)
{
	tex->clut.start = 0;
	tex->clut.load_method = CLUT_LOAD;
	tex->clut.psm = psm;
	tex->clut.storage_mode = CLUT_STORAGE_MODE1;
	//tex->clut.address = g_Manager.textureInVram->clut.address;
}

void CreateTexStructs(Texture *tex, int width, int psm, u32 components, u32 function, bool texfilter)
{

	if (texfilter)
	{
		tex->lod.mag_filter = LOD_MAG_LINEAR;
		tex->lod.min_filter = LOD_MIN_LINEAR;
	}
	else
	{
		tex->lod.mag_filter = LOD_MAG_NEAREST;
		tex->lod.min_filter = LOD_MIN_NEAREST;	
	}

	tex->lod.l = 0;
	tex->lod.k = 0;
	tex->lod.calculation = LOD_USE_K;
	tex->lod.max_level = 0;

	tex->texbuf.info.width = draw_log2(width);
	tex->texbuf.info.height = draw_log2(width);
	tex->texbuf.info.components = components;
	tex->texbuf.info.function = function;

	tex->texbuf.width = width;
	tex->texbuf.psm = psm;

	//tex->texbuf.address = g_Manager.textureInVram->texbuf.address;
}


qword_t *SetTextureWrap(qword_t *b, u16 mode)
{
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

    PACK_GIFTAG(b, GS_SET_CLAMP(wrap.horizontal, wrap.vertical, wrap.minu, wrap.maxu, wrap.minv, wrap.maxv), GS_REG_CLAMP + g_Manager.gs_context);
    b++;

    return b;
}

qword_t *SetTextureRegisters(qword_t *q, lod_t *lod, texbuffer_t *texbuf, clutbuffer_t *clut, 
							u32 texAddress, u32 clutAddress)
{
    int context = g_Manager.gs_context;
    q->dw[0] = GS_SET_TEX1(lod->calculation, 
						   lod->max_level, 
						   lod->mag_filter, 
						   lod->min_filter, 
						   lod->mipmap_select, 
						   lod->l, 
						   (int)(lod->k * 16.0f));

    q->dw[1] = GS_REG_TEX1 + context;
    q++;

    q->dw[0] = GS_SET_TEX0(
        texAddress >> 6,
        texbuf->width >> 6,
        texbuf->psm,
        texbuf->info.width,
        texbuf->info.height,
        texbuf->info.components,
        texbuf->info.function,
        clutAddress >> 6,
        clut->psm,
        clut->storage_mode,
        clut->start,
        clut->load_method);

    q->dw[1] = GS_REG_TEX0 + context;
    q++;
    return q;
}

void ClearScreen(RenderTarget *target, int context, int r, int g, int b, int a)
{

	qword_t *q = GetDMABasePointer();

	qword_t *dmatag = q;

	int alpha = ATEST_METHOD_NOTEQUAL;

	q++;

	float xOff = 2048.0f - (target->render->width >> 1);
	float yOff = 2048.0f - (target->render->height >> 1);
	if (!a)
		alpha = ATEST_METHOD_EQUAL;

	q = draw_disable_tests_alpha(q, context, alpha);
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

	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, alpha, 0x00, ATEST_KEEP_FRAMEBUFFER, DRAW_DISABLE, DRAW_DISABLE, DRAW_ENABLE, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);

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
qword_t *SetupRGBAQGS(qword_t *b, Color color)
{
	PACK_GIFTAG(b, GIF_SET_RGBAQ(color.r, color.g, color.b, color.a, (int)color.q), GIF_REG_RGBAQ);
	b++;
	return b;
}
qword_t *SetupAlphaGS(qword_t *q, blend_t *blend, int context)
{
	PACK_GIFTAG(q, GS_SET_ALPHA(blend->color1, blend->color2, blend->alpha, blend->color3, blend->fixed_alpha), GS_REG_ALPHA + context);
	q++;

	return q;
}
