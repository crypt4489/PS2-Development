// This include will allow us to avoid reincluding other headers
#include "irx_imports.h"

// Maybe we have important typedefs here!
#include "proc_texture.h"




void read_from_iop(unsigned char* iop_addr, int bytes, unsigned char* ee_buffer)
{
    int id;
    SifDmaTransfer_t dma_trans;

    dma_trans.attr = 0;
    dma_trans.size = bytes;
    dma_trans.dest = (void *)ee_buffer;
    dma_trans.src = (void *)iop_addr;

    while((id = SifSetDma(&dma_trans, 1)) == 0);
	while(SifDmaStat(id) >= 0);
}

/*
void SetupTimer()
{
    iop_sys_clock_t ClockTicks;
    USec2SysClock(5000 * 1000, &ClockTicks);
    SetAlarm(&ClockTicks, &TimerHandler, 0);
}
*/
