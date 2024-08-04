#ifndef PS_DMABUFFER_H
#define PS_DMABUFFER_H

#include "ps_global.h"
#include <dma_tags.h>
#include <dma.h>

DrawBuffers *CreateDrawBuffers(u32 size);
void SwitchDrawBuffers(DrawBuffers *bufferstruct);
qword_t *ResetBufferPointer(packet_t *pack);
void DestroyDrawBuffers(DrawBuffers *buff);
void SubmitToDMAController(qword_t* q, int channel, int type, int qwc, bool tte);
void SubmitDrawBuffersToController(qword_t *q, u32 channel, u32 type, bool tte);
void ParsePipelineDMA(void *data, qword_t *pipelineData);
void SubmitDMABuffersAsPipeline(qword_t *q, void* data);
void InitializeDMAChannels();

qword_t *CreateDMATag(qword_t *q, u32 code, u32 size, u32 w2, u32 w3, u32 spr, ...);
qword_t *CreateDirectTag(qword_t *q, u32 size, u32 inte);
qword_t *CreateDirectHLTag(qword_t *q, u32 size, u32 inte);
qword_t *AddSizeToDMATag(qword_t *q, u32 size);
qword_t *AddSizeToDirectTag(qword_t *q, u32 size);

inline qword_t *CreateDCODETag(qword_t *q, u32 code)
{
    q->sw[0] = code;
    q->sw[1] = code;
    q->sw[2] = code;
    q->sw[3] = code;
    q++;
    return q;
};

inline qword_t *CreateMaterialDCODETag(qword_t *q, u32 addr)
{
    q->sw[0] = DMA_DCODE_LOAD_MATERIAL;
    q->sw[1] = addr;
    q->sw[2] = DMA_DCODE_LOAD_MATERIAL;
    q->sw[3] = DMA_DCODE_LOAD_MATERIAL;
    q++;
    return q;
};

inline qword_t *CreateLoadByIdDCODETag(qword_t *q, u64 id)
{
    q->sw[0] = DMA_DCODE_LOAD_ID_TEXTURE;
    q->dw[1] = id;
    
    q++;
    return q;
};

inline qword_t *CreateDCODEDmaTransferTag(qword_t *q, u32 channel, u32 tte, u32 type, u32 qwc)
{
    q->sw[0] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[1] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[2] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[3] = DMA_DCODE(channel, qwc, tte, type);
    q++;
    return q;
};

inline qword_t *CreateDCODEMeshUpload(qword_t *q, u32 channel, u32 tte, u32 type, u32 qwc)
{
    q->sw[0] = DMA_DCODE_UPLOAD_MESH;
    q->sw[1] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[2] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[3] = DMA_DCODE(channel, qwc, tte, type);
    q++;
    return q;
};

inline qword_t *CreateDCODEDrawFinish(qword_t *q, u32 channel, u32 tte, u32 type, u32 qwc)
{
    q->sw[0] = DMA_DCODE_DRAW_FINISH;
    q->sw[1] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[2] = DMA_DCODE(channel, qwc, tte, type);
    q->sw[3] = DMA_DCODE(channel, qwc, tte, type);
    q++;
    return q;
};

inline qword_t *UpdateSizeOfDCODE(qword_t *q, u32 qwc)
{
    q->sw[0] = (q->sw[0] | ((qwc & 0x00007FFF) << 1));
    return q;
};


#endif
