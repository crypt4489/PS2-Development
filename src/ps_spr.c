#include "ps_spr.h"
#include <dma_registers.h>
#include <dma.h>
// Channel Control
static u32 dma_chcr[10] = {0x10008000, 0x10009000, 0x1000A000, 0x1000B000, 0x1000B400, 0x1000C000, 0x1000C400, 0x1000C800, 0x1000D000, 0x1000D400};
// Quadword Count
static u32 dma_qwc[10] = {0x10008020, 0x10009020, 0x1000A020, 0x1000B020, 0x1000B420, 0x1000C020, 0x1000C420, 0x1000C820, 0x1000D020, 0x1000D420};
// Memory Address
static u32 dma_madr[10] = {0x10008010, 0x10009010, 0x1000A010, 0x1000B010, 0x1000B410, 0x1000C010, 0x1000C410, 0x1000C810, 0x1000D010, 0x1000D410};
// Tag Address
// static u32 dma_tadr[10] = { 0x10008030, 0x10009030, 0x1000A030, 0x1000B030, 0x1000B430, 0x1000C030, 0x1000C430, 0x1000C830, 0x1000D030, 0x1000D430 };
// Tag Address Save 0
// static u32 dma_asr0[10] = { 0x10008040, 0x10009040, 0x1000A040, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
// Tag Address Save 1
// static u32 dma_asr1[10] = { 0x10008050, 0x10009050, 0x1000A050, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
// SPR Transfer Address
static u32 dma_sadr[10] = {0x10008080, 0x10009080, 0x1000A080, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1000D080, 0x1000D480};

void sendPacketSPR(void *data, u32 qwc)
{
	dma_channel_send_normal(DMA_CHANNEL_toSPR, data, qwc, 0, 1);
}

void sendPacketChainSPR(void *data)
{

	dma_channel_send_chain(DMA_CHANNEL_toSPR, data, 0, DMA_FLAG_TRANSFERTAG, 1);
}

void receiveSPR(void *data, void *dest, u32 qwc)
{

	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

	// Set the size of the data, in quadwords.
	*(vu32 *)dma_qwc[DMA_CHANNEL_fromSPR] = DMA_SET_QWC(qwc);

	*(vu32 *)dma_sadr[DMA_CHANNEL_fromSPR] = DMA_SET_SADR((u32)data);

	// Set the address of the data.
	*(vu32 *)dma_madr[DMA_CHANNEL_fromSPR] = DMA_SET_MADR((u32)dest, 0);

	// Start the transfer.
	*(vu32 *)dma_chcr[DMA_CHANNEL_fromSPR] = DMA_SET_CHCR(0, 0, 0, 0, 0, 1, 0);
}

void ultimate_memcpy(void *from_data, u32 qwc, void *to_data)
{
	u32 totalSize = qwc;
	while (totalSize > 0)
	{

		u32 sendSize;

		if (totalSize >= 1024)
		{
			sendSize = 1024;
			totalSize -= sendSize;
		}
		else
		{
			sendSize = totalSize;
			totalSize -= sendSize;
		}

		sendPacketSPR(from_data, sendSize);
		dma_channel_wait(DMA_CHANNEL_toSPR, -1);
		receiveSPR(0, to_data, sendSize);
		dma_channel_wait(DMA_CHANNEL_fromSPR, -1);
	}
}
