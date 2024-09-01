#ifndef PS_SPR_H
#define PS_SPR_H
#include "ps_global.h"
#include <dma_registers.h>
#include <dma.h>
void SendPacketSPR(qword_t *data);
void ReceiveSPRNormal(void *dest, u32 qwc);
void ReceiveSPRChain(u32 sprstart);
void Ultimatememcpy(void *from_data, u32 qwc, void *to_data);
#endif
