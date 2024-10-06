#include "system/ps_spr.h"

#include <dma_registers.h>
#include <dma.h>

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
	dma_channel_send_chain(DMA_CHANNEL_toSPR, g_Manager.dmaBuffers->tospr, qwc, 0, 0);
	g_Manager.dmaBuffers->tospr += qwc;
}

void ReceiveSPRNormal(void *dest, u32 qwc)
{

	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

	// Set the size of the data, in quadwords.
	*(vu32 *)0x1000D020 = DMA_SET_QWC(qwc);

	// Set address of data in SPR
	*(vu32 *)0x1000D080 = DMA_SET_SADR(0);

	// Set the address of the data.
	*(vu32 *)0x1000D010 = DMA_SET_MADR((u32)dest, 0);

	// Start the transfer.
	*(vu32 *)0x1000D000 = DMA_SET_CHCR(0, 0, 0, 0, 0, 1, 0);
}

void ReceiveSPRChain(u32 sprstart)
{

	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

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

	qword_t *qwrdptrfrom = (qword_t*)from_data;
	qword_t *qwrdptrto = (qword_t*)to_data;

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
		q = CreateDMATag(q, sprtag, sendSize, 0, 0, 0, (u32)qwrdptrto);
		q = CreateDMATag(q, refcode, sendSize, 0, 0, 0, (u32)qwrdptrfrom);

		qwrdptrfrom += sendSize;
		qwrdptrto += sendSize;
		totalSize -= sendSize;
	}
	
	SendPacketSPR(q);
	//ReceiveSPRChain(0);
}
