#include "system/ps_vif.h"

#include <gs_gp.h>
#include <kernel.h>

#include "gs/ps_gs.h"
#include "dma/ps_dma.h"
#include "log/ps_log.h"

qword_t *InitDoubleBufferingQWord(qword_t *q, u16 base, u16 offset)
{
    qword_t *b = q;
    DMATAG_CNT(b, 0, 0, VIF_CODE(base, 0, VIF_CMD_BASE, 0), VIF_CODE(offset, 0, VIF_CMD_OFFSET, 0));
    b++;
    return b;
}

void UploadProgramToVU1(u32 *cStart, u32 *cEnd, u32 dest, u32 packetSize, u32 programSize)
{
    //packet_t *pack = packet_init(packetSize+1, PACKET_NORMAL);
    //qword_t pack[packetSize];
    qword_t *start = InitializeDMAObject();
    qword_t *q;
    u32 count = (cEnd - cStart) / 2;
    if (count & 1)
    {
        count++;
    }

    dma_channel_wait(DMA_CHANNEL_VIF1, -1);

    u32 *l_start = cStart;
    q = start;

    q+=1;
    while (count > 0)
    {
        u16 currCount = count > 256 ? 256 : count;
        int currhalf = currCount / 2;
        DMATAG_REF(q, currhalf, (u32)l_start, 0, VIF_CODE(0, 0, VIF_CMD_NOP, 0), VIF_CODE(dest, currCount & 0xFF, VIF_CMD_MPG, 1));
        q++;
        //DEBUGLOG("%d", count);
        l_start += currCount * 2;
        count -= currCount;
        dest += currCount;
    }
    CreateDMATag(start, DMA_CNT, 0, 0, 0, 0);
    SubmitDMABuffersToController(q, DMA_CHANNEL_VIF1, 1, 1);
}

qword_t *UploadVectorsVU0(qword_t *q, void *vectors, u32 offset, u32 *dest, u32 size)
{
    q = add_unpack_data(q, 4, &(((qword_t*)vectors)[offset]), size, 0, VIF_CMD_UNPACK(0, 3, 0));

    *dest += size;

    return q;
}

u32 UploadStartProgram(u32 startCode, u32 startAddress, u32 inte)
{
    return VIF_CODE(startAddress, 0, startCode, inte);
}

u32 UploadFlushTag(u32 inte)
{
    return VIF_CODE(0, 0, VIF_CMD_FLUSH, inte);
}

qword_t *add_unpack_data(qword_t *q, u32 dest_address, void *data, u32 qwSize, u8 use_top, u32 vif_pack)
{
    if (qwSize > 256)
    {
        qwSize = 256;
        ERRORLOG("Passed too many QWords in AddUnPack");
    }
    DMATAG_REF(q, qwSize, (u32)data, 0, 0, 0);
    q->sw[2] = VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0);
    q->sw[3] = VIF_CODE((dest_address | (0 << 14) | ((u32)use_top << 15)), qwSize, vif_pack, 0);
    q++;
    return q;
}


qword_t *ReadUnpackData(qword_t *q, u32 dest_address, u32 qwSize, u8 use_top, u32 vif_pack)
{
    u32 pack_size = qwSize;
    if (qwSize > 256)
    {
        ERRORLOG("Passed too many QWords in ReadUnPack");
        pack_size = 256;
    }
    DMATAG_CNT(q, pack_size, 0, 0, 0);
    q->sw[2] = VIF_CODE(0x0101, 0, VIF_CMD_STCYCL, 0);
    q->sw[3] = VIF_CODE((dest_address | (0 << 14) | ((u32)use_top << 15)), pack_size, vif_pack, 0);
    q++;
    return q;
}

qword_t *add_start_program_vu1(qword_t *q, u32 address)
{
    DMATAG_CNT(q, 0, 0, 0, 0);
    q->sw[3] = VIF_CODE(address, 0, VIF_CMD_MSCAL, 0);
    q++;
    DMATAG_END(q, 0, 0, 0, 0);
    q->sw[3] = VIF_CODE(0, 0, VIF_CMD_FLUSH, 1);
    q++;
    return q;
}

qword_t *VIFSetupScaleVector(qword_t *b)
{
    qword_t *q = b;
    ((float *)q->sw)[0] = 2048.0f;
    ((float *)q->sw)[1] = 2048.0f;
    ((float *)q->sw)[2] = ((float)0xFFFFFF) / 32.0f;
    q->sw[3] = 0;
    q++;
    return q;
}

qword_t *set_tex_address_mode(qword_t *q, u32 mode, u32 context)
{
    qword_t *b = q;
    DMATAG_CNT(b, 3, 0, 0, 0);
    b++;

    b->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[3] = VIF_CODE(2, 0, VIF_CMD_DIRECT, 0);
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

qword_t *set_alpha_registers(qword_t *q, blend_t *blend, int context)
{
    qword_t *b = q;
    DMATAG_CNT(b, 4, 0, 0, 0);
    b++;

    b->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    b->sw[3] = VIF_CODE(3, 0, VIF_CMD_DIRECT, 0);
    b++;

    PACK_GIFTAG(b, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    b++;

    PACK_GIFTAG(b, GS_SET_ALPHA(blend->color1, blend->color2, blend->alpha, blend->color3, blend->fixed_alpha), (context == 0) ? GS_REG_ALPHA : GS_REG_ALPHA_2);
    b++;

    PACK_GIFTAG(b, GS_SET_PABE(DRAW_ENABLE), GS_REG_PABE);
    b++;

    /* PACK_GIFTAG(b,GS_SET_COLCLAMP(GS_DISABLE),GS_REG_COLCLAMP);
     b++; */
    return b;
}

#define step 128

void DownloadFrameBuffer(framebuffer_t *frame, unsigned char *frame_buffer_local)
{
    int hei = frame->height;
    int offset = 0;
    int steps = hei / step;
    for (int i = 0; i < steps; i++)
    {
        CopyVRAMToMemory(frame->address, frame->width, step, 0, (i * step), GS_PSM_32, &((u32 *)frame_buffer_local)[offset]);
        offset += (frame->width * step);
    }
}

void LoadFrameBufferVIF(unsigned char *pixels, framebuffer_t *frame, int width, int height, int psm)
{

    packet_t *packet = packet_init(200, PACKET_NORMAL);

    u32 totalSize = frame->height * frame->width * 4;

    u32 totalQwordBlocks = totalSize >> 4;

    u32 i;

    u32 dataSize = 1 << 18;

    u32 qwSize = dataSize >> 4;

    i = totalQwordBlocks / qwSize;

    qword_t *q = packet->data;

    qword_t *start = q;

    DMATAG_CNT(q, 6, 0, 0, 0);
    q++;

    q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    ;
    q->sw[3] = VIF_CODE(5, 0, VIF_CMD_DIRECT, 0);
    q++;

    PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, frame->address >> 6, frame->width >> 6, psm), GS_REG_BITBLTBUF);
    q++;
    PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
    q++;
    PACK_GIFTAG(q, GS_SET_TRXREG(frame->width, frame->height), GS_REG_TRXREG);
    q++;
    PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
    q++;

    while (i-- > 0)
    {
        DMATAG_CNT(q, 2, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[3] = VIF_CODE(qwSize + 1, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(qwSize, 0, 0, 0, GIF_FLG_IMAGE, 0), 0);
        q++;

        DMATAG_REF(q, qwSize, (u32)pixels, 0, 0, 0);
        q++;

        pixels += dataSize;
    }

    DMATAG_END(q, 3, 0, 0, 0);
    q++;

    q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[3] = VIF_CODE(2, 0, VIF_CMD_DIRECT, 0);
    q++;

    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH);
    q++;

    dma_channel_wait(DMA_CHANNEL_VIF1, -1);

    FlushCache(0);

    dma_channel_send_chain(DMA_CHANNEL_VIF1, packet->data, q - start, 0, 0);

    packet_free(packet);
}

qword_t *load_texture_vif(qword_t *q, Texture *tex, void *pixels, unsigned char *clut_buffer)
{
    u32 dataSize, totalQwordBlocks, i, qwSize;

    // i = totalQwordBlocks / qwSize;

    unsigned char *buf_ptr;

    // DMATAG_CNT(q, 0, 0, 0, 0);
    // q->sw[3] = VIF_CODE(0, 0, VIF_CMD_FLUSH, 0);
    // q++;

    if (clut_buffer)
    {
        u32 clut_size = 16 * 16 * 4; // total number of bytes 1024

        totalQwordBlocks = clut_size >> 4; // 64 total number of q words

        qwSize = 8; // number of q words per read

        i = totalQwordBlocks / qwSize; // number of iteration

        dataSize = qwSize * 16;

        buf_ptr = tex->clut_buffer;

        DMATAG_CNT(q, 6, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = VIF_CODE(0, 0, 0, 0);
        q->sw[3] = VIF_CODE(5, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
        q++;
        PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, tex->clut.address >> 6, 1, tex->clut.psm), GS_REG_BITBLTBUF);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXREG(16, 16), GS_REG_TRXREG);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
        q++;

        while (i-- > 0)
        {
            DMATAG_CNT(q, 2, 0, 0, 0);
            q++;

            q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
            q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
            q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
            q->sw[3] = VIF_CODE(qwSize + 1, 0, VIF_CMD_DIRECT, 0);
            q++;

            PACK_GIFTAG(q, GIF_SET_TAG(qwSize, 0, 0, 0, GIF_FLG_IMAGE, 0), 0);
            q++;

            DMATAG_REF(q, qwSize, (u32)buf_ptr, 0, 0, 0);
            q++;

            buf_ptr += dataSize;
        }

        DMATAG_CNT(q, 3, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = 0; // VIF_CODE(0, 0, VIF_CMD_FLUSH, 0);
        q->sw[3] = VIF_CODE(2, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
        q++;
        PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH);
        q++;
    }

    if (pixels)
    {
        u32 psm = 1;

        u32 image_size = tex->width * tex->height * psm;

        totalQwordBlocks = image_size >> 4; // total number of qwords

        qwSize = totalQwordBlocks; // total qwords sent per iteration

        DMATAG_CNT(q, 6, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = VIF_CODE(0, 0, 0, 0);
        q->sw[3] = VIF_CODE(5, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
        q++;
        PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, tex->texbuf.address >> 6, 256 >> 6, 0x13), GS_REG_BITBLTBUF);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXREG(256, 256), GS_REG_TRXREG);
        q++;
        PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
        q++;

        // qwSize = (image_size >> 4) + 3;

        // DMATAG_CALL(q, qwSize, (u32)pixels, 0, 0, 0);
        // q++;

        DMATAG_CNT(q, 2, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[3] = VIF_CODE(qwSize + 1, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(qwSize, 0, 0, 0, GIF_FLG_IMAGE, 0), 0);
        q++;
        DMATAG_REF(q, qwSize, (u32)pixels, 0, 0, 0);
        q++;

        DMATAG_CNT(q, 3, 0, 0, 0);
        q++;

        q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
        q->sw[2] = 0; // VIF_CODE(0, 0, VIF_CMD_FLUSH, 0);
        q->sw[3] = VIF_CODE(2, 0, VIF_CMD_DIRECT, 0);
        q++;

        PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
        q++;
        PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH);
        q++;
    }

    return q;
}

qword_t *vif_setup_rgbaq(qword_t *b, color_t color)
{
    qword_t *q = b;
    DMATAG_CNT(q, 3, 0, 0, 0);
    q++;

    q->sw[0] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[1] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[2] = VIF_CODE(0, 0, VIF_CMD_NOP, 0);
    q->sw[3] = VIF_CODE(2, 0, VIF_CMD_DIRECT, 0);
    q++;

    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GIF_SET_RGBAQ(color.r, color.g, color.b, color.a, 1), GIF_REG_RGBAQ);
    q++;
    return q;
}
