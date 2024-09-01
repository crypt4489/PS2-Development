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
void InitializeDMAChannels();
qword_t *StitchDMAChain(qword_t *q, qword_t *end, bool vif);
qword_t *CreateDMATag(qword_t *q, u32 code, u32 size, u32 w2, u32 w3, u32 spr, u32 addr);
qword_t *CreateDirectTag(qword_t *q, u32 size, u32 inte);
qword_t *CreateDirectHLTag(qword_t *q, u32 size, u32 inte);
qword_t *AddSizeToDMATag(qword_t *q, u32 size);
qword_t *AddSizeToDirectTag(qword_t *q, u32 size);
DMABuffers *CreateDMABuffers();

inline u32 GetDMACode(qword_t *q)
{
    return (q->sw[0] & (7<<28)) >> 28;
}

inline void SetDMACode(qword_t *q, u32 code)
{
    q->sw[0] = (q->sw[0] & ~(7<<28)) | (code<<28);
}

#endif
