#ifndef PS_DMABUFFER_H
#define PS_DMABUFFER_H

#include "ps_global.h"
#include <dma_tags.h>
#include <dma.h>

DMABuffers *CreateDMABuffers(u32 size);
DMABuffers *SwitchDMABuffers(DMABuffers *bufferstruct);
qword_t *ResetBufferPointer(packet_t *pack);
void DestroyDMABuffers(DMABuffers *buff);
void SubmitToDMAController(qword_t* q, int channel, int type, int qwc, int tte);
void SubmitDMABuffersToController(qword_t *q, u32 channel, u32 type, u32 tte);
qword_t *InitializeDMAObject();
void ParsePipelineDMA(void *data, qword_t *pipelineData);
void SubmitDMABuffersAsPipeline(qword_t *q, void* data);
void InitializeDMAChannels();

qword_t *CreateDMATag(qword_t *q, u32 code, u32 size, u32 w2, u32 w3, u32 spr, ...);
qword_t *CreateDirectTag(qword_t *q, u32 size, u32 inte);
qword_t *AddSizeToDMATag(qword_t *q, u32 size);
qword_t *AddSizeToDirectTag(qword_t *q, u32 size);
qword_t *CreateDCODETag(qword_t *q, u32 code);
qword_t *CreateDCODEDmaTransferTag(qword_t *q, u32 channel, u32 tte, u32 type, u32 qwc);
qword_t *UpdateSizeOfDCODE(qword_t *q, u32 qwc);
qword_t *CreateLoadByIdDCODETag(qword_t *q, u32 id);
qword_t *CreateMaterialDCODETag(qword_t *q, u32 addr);
qword_t *CreateDCODEMeshUpload(qword_t *q, u32 channel, u32 tte, u32 type, u32 qwc);



#endif