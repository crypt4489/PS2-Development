#include "dma/ps_dma.h"

#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vif_codes.h>
#include <kernel.h>

#include "gamemanager/ps_manager.h"
#include "log/ps_log.h"

void InitializeDMAChannels()
{
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
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

    q->dw[0] = DMATAG(size, 0, code, 0, addr, 0);
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
            //  ERRORLOG("%d %d %d", channel, qwc, tte);
            //  dump_packet(pipe->q);
            // while(1);

            SubmitToDMAController(q, channel, type, qwc, tte);
            q += qwc;
        }
    }
}

qword_t *InitializeDMAObject()
{
    return g_Manager.dmabuffers->currPointer;
}

DMABuffers *CreateDMABuffers(u32 size)
{
    DMABuffers *buffer = (DMABuffers *)malloc(sizeof(DMABuffers));
    buffer->bufferId = 0;
    buffer->dma_chains[0] = packet_init(size, PACKET_NORMAL);
    buffer->dma_chains[1] = packet_init(size, PACKET_NORMAL);
    packet_reset(buffer->dma_chains[0]);
    packet_reset(buffer->dma_chains[1]);
    buffer->currPointer = buffer->dma_chains[0]->data;
    return buffer;
}

DMABuffers *SwitchDMABuffers(DMABuffers *bufferstruct)
{
    if (bufferstruct->bufferId == 0)
    {
        bufferstruct->bufferId = 1;
        bufferstruct->currPointer = ResetBufferPointer(bufferstruct->dma_chains[1]);
    }
    else
    {
        bufferstruct->bufferId = 0;
        bufferstruct->currPointer = ResetBufferPointer(bufferstruct->dma_chains[0]);
    }

    return bufferstruct;
}

qword_t *ResetBufferPointer(packet_t *pack)
{
    packet_reset(pack);
    return pack->data;
}

void DestroyDMABuffers(DMABuffers *buff)
{
    free(buff->dma_chains[0]);
    free(buff->dma_chains[1]);
    free(buff);
}

void SubmitToDMAController(qword_t *q, int channel, int type, int qwc, int tte)
{
    while (PollVU1DoneProcessing(&g_Manager) < 0);
        ;
    if (channel == DMA_CHANNEL_GIF)
    {
        dma_channel_wait(DMA_CHANNEL_GIF, -1);
    }
    else if (channel == DMA_CHANNEL_VIF1)
    {
        g_Manager.vu1DoneProcessing = 0;
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
        FlushCache(0);
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

    if (type == 0)
    {
        FlushCache(0);
        dma_channel_send_normal(channel, q, qwc, tte, 0);
        INFOLOG(" in normal");
    }
    else
    {
        dma_channel_send_chain(channel, q, qwc, tte, 0);
    }
}

void SubmitDMABuffersToController(qword_t *q, u32 channel, u32 type, u32 tte)
{

    u32 size = q - g_Manager.dmabuffers->currPointer;
    SubmitToDMAController(g_Manager.dmabuffers->currPointer, channel, type, size, tte);

    g_Manager.dmabuffers->currPointer = q;
}

void SubmitDMABuffersAsPipeline(qword_t *q, void *data)
{
    ParsePipelineDMA(data, g_Manager.dmabuffers->currPointer);
    g_Manager.dmabuffers->currPointer = q;
}
