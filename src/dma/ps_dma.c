#include "dma/ps_dma.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <vif_codes.h>
#include <kernel.h>

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

qword_t *CreateDMATag(qword_t *q, u32 code, u32 size, u32 w2, u32 w3, u32 spr, ...)
{
    va_list tag_args;
    va_start(tag_args, spr);
    u32 addr = 0;
    if ((DMA_REFE == code) || (DMA_REFS == code) || (DMA_REF == code) || (DMA_CALL == code) || (DMA_NEXT == code))
    {
        addr = va_arg(tag_args, u32);
    }

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
    q->sw[0] = (q->sw[0] | (size & 0x0000FFFF));
    return q;
}

qword_t *AddSizeToDirectTag(qword_t *q, u32 size)
{
    q->sw[3] = (q->sw[3] | (size & 0x0000FFFF));
    return q;
}

void ParsePipelineDMA(void *data, qword_t *pipelineData)
{
    qword_t *q = pipelineData;
    DMA_DCODE_STRUCT decode;
    int loop = 1;
    int channel, qwc, tte, type;
    while (loop)
    {
        decode.code = q->sw[0];
        if (decode.code == DMA_DCODE_END)
        {
            loop = 0;
            break;
        }
        else
        {
            channel = decode.chann;
            qwc = decode.qwc;
            tte = decode.tte;
            type = decode.type;
            q++;
             // ERRORLOG("%d %d %d", channel, qwc, tte);
            //  dump_packet(pipe->q);
            // while(1);

            SubmitToDMAController(q, channel, type, qwc, tte);
            q += qwc;
        }
    }
}

DrawBuffers *CreateDrawBuffers(u32 size)
{
    DrawBuffers *buffer = (DrawBuffers *)malloc(sizeof(DrawBuffers));
    buffer->context = 0;
    buffer->size = size;
    for (int i = 1; i>=0; i--)
    {
        buffer->gifupload[i] = (qword_t*)calloc(size,sizeof(qword_t));
        buffer->vifupload[i] = (qword_t*)calloc(size, sizeof(qword_t));
    }
    
    buffer->currentgif = buffer->gifupload[0];
    buffer->currentvif = buffer->vifupload[0];
    
    return buffer;
}

void SwitchDrawBuffers(DrawBuffers *bufferstruct)
{
    u32 context = (bufferstruct->context ^= 1);
    bufferstruct->currentvif = bufferstruct->vifupload[context];
    bufferstruct->currentgif = bufferstruct->gifupload[context];
}

void DestroyDrawBuffers(DrawBuffers *buff)
{
    for (int i = 1; i>=0; i--)
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
    FlushCache(0);
    if (type == 0)
    {
        dma_channel_send_normal(channel, q, qwc, tte, 0);
    }
    else
    {
        dma_channel_send_chain(channel, q, qwc, tte, 0);
    }
}

void SubmitDrawBuffersToController(qword_t *q, u32 channel, u32 type, bool tte)
{
    qword_t *send;
    switch(channel)
    {
        case DMA_CHANNEL_GIF:
        send = g_Manager.drawBuffers->currentgif;
        g_Manager.drawBuffers->currentgif = q;
        break;
        case DMA_CHANNEL_VIF1:
        send = g_Manager.drawBuffers->currentvif;
         g_Manager.drawBuffers->currentvif = q;
        break;
        default:
        ERRORLOG("Unsupposerted channel for submitting draw buffer");
        return;
    }
    
    u32 size = q - send;
    SubmitToDMAController(send, channel, type, size, tte);
}

void SubmitDMABuffersAsPipeline(qword_t *q, void *data)
{
    //ParsePipelineDMA(data, g_Manager.dmabuffers->currPointer);
   // g_Manager.dmabuffers->currPointer = q;
}
