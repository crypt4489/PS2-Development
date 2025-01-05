#ifndef PS_SPR_H
#define PS_SPR_H
#include "ps_global.h"
#include <dma_registers.h>
#include <dma.h>
void SendPacketSPR(qword_t *data);
void ReceiveSPRNormal(void *dest, u32 qwc);
void ReceiveSPRChain(u32 sprstart, void* data, u32 size);
void Ultimatememcpy(void *from_data, u32 totalSize, void *to_data);
void SendSPRNormal(qword_t *q, u32 qwc);
#endif
