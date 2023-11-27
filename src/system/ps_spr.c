#include "system/ps_spr.h"

#include <dma_registers.h>
#include <dma.h>

#include "dma/ps_dma.h"
#include "system/ps_timer.h"
void sendPacketSPR(void *data, u32 qwc)
{
	qword_t *q = InitializeDMAObject();
	DMATAG_REF(q, qwc, (u32)data, 0, 0, 0);
	dma_channel_send_chain(DMA_CHANNEL_toSPR, q, 1, 0, 1);
}

void receiveSPR(void *data, void *dest, u32 qwc)
{

	*DMA_REG_STAT = DMA_SET_STAT(1 << DMA_CHANNEL_fromSPR, 0, 0, 0, 0, 0, 0);

	// Set the size of the data, in quadwords.
	*(vu32 *)0x1000D020 = DMA_SET_QWC(qwc);

	*(vu32 *)0x1000D080 = DMA_SET_SADR(0);

	// Set the address of the data.
	*(vu32 *)0x1000D010 = DMA_SET_MADR((u32)dest, 0);

	// Start the transfer.
	*(vu32 *)0x1000D000 = DMA_SET_CHCR(0, 0, 0, 0, 0, 1, 0);
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
		}
		else
		{
			sendSize = totalSize;
		}

		totalSize -= sendSize;
	
		sendPacketSPR(from_data, sendSize);
		dma_channel_wait(DMA_CHANNEL_toSPR, -1);
		receiveSPR(0, to_data, sendSize);
		//dma_channel_wait(DMA_CHANNEL_fromSPR, -1);

		to_data += sendSize * 4;
		from_data += sendSize * 4;
	}
}
