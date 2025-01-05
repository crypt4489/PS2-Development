#include "system/ps_spr.h"

#include <limits.h>

#include <dma_registers.h>
#include <dma.h>
#include <kernel.h>

#include "dma/ps_dma.h"
#include "log/ps_log.h"

#define MAX_SPR_CHAIN_DATA_QWC 1023


static qword_t *StartSPRTransfer()
{
	return g_Manager.dmaBuffers->tospr;
}

void SendPacketSPR(qword_t *q)
{
	u32 qwc = q - g_Manager.dmaBuffers->tospr;

	dma_channel_wait(DMA_CHANNEL_toSPR, -1);
	
	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_toSPR,0,0,0,0,0,0);
	
	FlushCache(0);

	SyncDCache(g_Manager.dmaBuffers->tospr, (void *)((u8 *)g_Manager.dmaBuffers->tospr + (qwc<<4)));

	*(vu32 *)0x1000D480 = DMA_SET_SADR(0);

	*(vu32 *)0x1000D420 = DMA_SET_QWC(0);

	*(vu32 *)0x1000D430 = DMA_SET_TADR((u32)g_Manager.dmaBuffers->tospr, 0);

	*(vu32 *)0x1000D400 = DMA_SET_CHCR(0, 1, 0, 0, 0, 1, 0);

	g_Manager.dmaBuffers->tospr += qwc;
}

void SendSPRNormal(qword_t *q, u32 qwc)
{
	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_toSPR,0,0,0,0,0,0);

	//SyncDCache(q, (void *)((u8 *)q + (qwc<<4)));

	*(vu32 *)0x1000D480 = DMA_SET_SADR(0);

	*(vu32 *)0x1000D420 = DMA_SET_QWC(qwc);

	*(vu32 *)0x1000D410 = DMA_SET_MADR((u32)q, 0);

	*(vu32 *)0x1000D400 = DMA_SET_CHCR(0, 0, 0, 0, 0, 1, 0);
}

void ReceiveSPRNormal(void *dest, u32 qwc)
{
	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

	//SyncDCache(dest, (void *)((u8 *)dest + (qwc<<4)));

	// Set the size of the data, in quadwords.
	*(vu32 *)0x1000D020 = DMA_SET_QWC(qwc);

	// Set address of data in SPR
	*(vu32 *)0x1000D080 = DMA_SET_SADR(0);

	// Set the address of the data.
	*(vu32 *)0x1000D010 = DMA_SET_MADR((u32)dest, 0);

	// Start the transfer.
	*(vu32 *)0x1000D000 = DMA_SET_CHCR(0, 0, 0, 0, 0, 1, 0);
}

void ReceiveSPRChain(u32 sprstart, void* data, u32 size)
{

	dma_channel_wait(DMA_CHANNEL_fromSPR, -1);


	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

	//FlushCache(0);

	//SyncDCache(data, (void *)((u8 *)data + (size<<4)));

	*(vu32 *)0x1000D080 = DMA_SET_SADR(sprstart);

	*(vu32 *)0x1000D020 = DMA_SET_QWC(0);

	*(vu32 *)0x1000D010 = DMA_SET_MADR((u32)data, 0);

	// Start the transfer.
	*(vu32 *)0x1000D000 = DMA_SET_CHCR(0, 1, 0, 0, 0, 1, 0);
}

void Ultimatememcpy(void *from_data, u32 totalSize, void *to_data)
{
	if (((u32)from_data | (u32)to_data) & 0x0000000F)
	{
		ERRORLOG("Misaligned data being copied to SPR and back from=%d to=%d", (u32)from_data, (u32)to_data);
		return;
	}
	
	qword_t *q = StartSPRTransfer();

	qword_t *qwrdto = (qword_t*)to_data;
	qword_t *qwrdfrom = (qword_t*)from_data;

	u32 refcode = DMA_REF;
	u32 sprtag = DMA_CNT;
	u32 sendSize = MAX_SPR_CHAIN_DATA_QWC;

	while (totalSize)
	{
		if (totalSize <= MAX_SPR_CHAIN_DATA_QWC)
		{
			sendSize = totalSize;
			refcode = DMA_REFE;
			sprtag = DMA_END;
		}

		q = CreateDMATag(q, DMA_CNT, 1, 0, 0, 0, 0);
		q = CreateDMATag(q, sprtag, sendSize, 0, 0, 0, (u32)qwrdto);
		q = CreateDMATag(q, refcode, sendSize, 0, 0, 0, (u32)qwrdfrom);

		qwrdfrom += sendSize;
		qwrdto += sendSize;
		totalSize -= sendSize;
	}
	
	SendPacketSPR(q);
	ReceiveSPRChain(0, to_data, sendSize);
}
