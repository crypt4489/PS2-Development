#include "dma/ps_dma.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <vif_codes.h>
#include <kernel.h>
#include <malloc.h>

#include "gamemanager/ps_manager.h"
#include "log/ps_log.h"

void InitializeDMAChannels()
{
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);

    dma_channel_initialize(DMA_CHANNEL_VIF0, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_toSPR, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_fromSPR, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    dma_channel_fast_waits(DMA_CHANNEL_VIF1);
    dma_channel_fast_waits(DMA_CHANNEL_VIF0);
    dma_channel_fast_waits(DMA_CHANNEL_toSPR);
    dma_channel_fast_waits(DMA_CHANNEL_fromSPR);
}

qword_t *CreateDMATag(qword_t *q, u32 code, u32 size, u32 w2, u32 w3, u32 spr, u32 addr)
{
    q->dw[0] = DMATAG(size, 0, code, 0, addr, spr);
    q->sw[2] = w2;
    q->sw[3] = w3;
    q++;
    return q;
}

qword_t *CreateDirectTag(qword_t *q, u32 size, u32 inte)
{
    q->sw[0] = q->sw[1] = q->sw[2] = 0;
    q->sw[3] = VIF_CODE(size, 0, VIF_CMD_DIRECT, inte);
    q++;
    return q;
}

qword_t *CreateDirectHLTag(qword_t *q, u32 size, u32 inte)
{
    q->sw[0] = q->sw[1] = q->sw[2] = 0;
    q->sw[3] = VIF_CODE(size, 0, VIF_CMD_DIRECTHL, inte);
    q++;
    return q;
}

qword_t *AddSizeToDMATag(qword_t *q, u32 size)
{
    q->sw[0] = ((q->sw[0] & 0xFFFF0000) | (size & 0x0000FFFF));
    return q;
}

qword_t *AddSizeToDirectTag(qword_t *q, u32 size)
{
    q->sw[3] = (q->sw[3] | (size & 0x0000FFFF));
    return q;
}

DrawBuffers *CreateDrawBuffers(u32 size)
{
    DrawBuffers *buffer = (DrawBuffers *)malloc(sizeof(DrawBuffers));
    buffer->context = 0;
    buffer->size = size;
    for (int i = 1; i >= 0; i--)
    {
        buffer->gifupload[i] = (qword_t *)memalign(128, size*sizeof(qword_t));
        memset(buffer->gifupload[i], 0, sizeof(qword_t)*size);
        buffer->vifupload[i] = (qword_t *)memalign(128, size*sizeof(qword_t));
        memset(buffer->vifupload[i], 0, sizeof(qword_t)*size);
    }

    buffer->currentgif = buffer->gifupload[0];
    buffer->currentvif = buffer->vifupload[0];
    buffer->readgif = buffer->gifupload[0];
    buffer->readvif = buffer->vifupload[0];

    return buffer;
}

DMABuffers *CreateDMABuffers()
{
    DMABuffers *dma = (DMABuffers*)malloc(sizeof(DMABuffers));
    dma->tospr = dma->tosprtape;
    return dma;
}

void SwitchDrawBuffers(DrawBuffers *bufferstruct)
{
    u32 context = (bufferstruct->context ^= 1);
    bufferstruct->currentvif = bufferstruct->vifupload[context];
    bufferstruct->currentgif = bufferstruct->gifupload[context];
    bufferstruct->readvif = bufferstruct->vifupload[context];
    bufferstruct->readgif = bufferstruct->gifupload[context];
}

void DestroyDrawBuffers(DrawBuffers *buff)
{
    for (int i = 1; i >= 0; i--)
    {
        free(buff->gifupload[i]);
        free(buff->vifupload[i]);
    }
    free(buff);
}

void SubmitToDMAController(qword_t *q, int channel, int type, int qwc, bool tte)
{
    while (PollVU1DoneProcessing(&g_Manager) < 0);
    if (channel == DMA_CHANNEL_GIF)
    {
        dma_channel_wait(DMA_CHANNEL_GIF, -1);
    }
    else if (channel == DMA_CHANNEL_VIF1)
    {
        g_Manager.vu1DoneProcessing = false;
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
    }
    else if (channel == DMA_CHANNEL_VIF0)
    {
    }
    else
    {
        ERRORLOG("%d %d %d", channel, qwc, tte);
        ERRORLOG("invalid channel for DMA submission");
        while (1)
        {
        }
    }
    
    if (!type)
    {
        FlushCache(0);
        dma_channel_send_normal(channel, q, qwc, tte, 0);
    }
    else
    {
        // just let the chain do the work, make qwc max for call dma
        dma_channel_send_chain(channel, q, 65536, tte, 0);
    }
}

void SubmitDrawBuffersToController(qword_t *q, u32 channel, u32 type, bool tte)
{
    qword_t *send;
    switch (channel)
    {
    case DMA_CHANNEL_GIF:
        send = g_Manager.drawBuffers->readgif;
        g_Manager.drawBuffers->currentgif = q;
        g_Manager.drawBuffers->readgif = q;
        break;
    case DMA_CHANNEL_VIF1:
        send = g_Manager.drawBuffers->readvif;
        g_Manager.drawBuffers->currentvif = q;
        g_Manager.drawBuffers->readvif = q;
        break;
    default:
        ERRORLOG("Unsupposerted channel for submitting draw buffer");
        return;
    }

    u32 size = q - send;
    SubmitToDMAController(send, channel, type, size, tte);
}

static inline void SetINTVIFCode(qword_t *q, int index)
{
    q->sw[index] |= 0x80000000;
}

qword_t *StitchDMAChain(qword_t *q, qword_t *end, bool vif)
{
    qword_t *traverse = q;
    qword_t *lasttag = q;
    s32 code = -1;
    bool traveling = true;
    bool jumpref = false;
    while (traveling)
    {
        if (traverse == end) break;
        if (code == DMA_END)
        {
            SetDMACode(lasttag, DMA_CNT);
        } 
        else if (code == DMA_REFE)
        {
            SetDMACode(lasttag, DMA_REF);
        }
        jumpref = false;
        u32 stride = (traverse->sw[0] & 0x0000FFFF);
        code = GetDMACode(traverse);
        switch (code)
        {
        case DMA_REF:
        case DMA_REFS:
        case DMA_REFE:
            stride=0;
        case DMA_CALL:
        case DMA_NEXT:
            jumpref = true;
            break;
        case DMA_CNT:
        case DMA_END:
            break;
        case DMA_RET:
        default:
            traveling = false;
            break;
        }

        

        if (traveling) {
            lasttag = traverse;
            traverse+=(stride+1);
        }
    }
    if (traveling) {
        if (jumpref) 
        {
            if (code == DMA_REF)
            {
                SetDMACode(lasttag, DMA_REFE);
            } else { 
              end = CreateDMATag(end, DMA_END, 0, VIF_CODE(0, 0, VIF_CMD_FLUSH, 1), 0, 0, 0);
              return end;
            }
        }
        if (code == DMA_CNT) { SetDMACode(lasttag, DMA_END); }
        if (vif && code != DMA_CALL) {
            int i = 2;
            if (lasttag->sw[3]) i = 3;
            SetINTVIFCode(lasttag, i);
        }
    } else {
        if (code != DMA_REF && code != DMA_CALL)
        {
            if (!traverse->sw[2] && !traverse->sw[3])
            {
                SetDMACode(lasttag, DMA_RET);
                end--; // remove ret tag;
            }
        } 
    }
    return end;
}
