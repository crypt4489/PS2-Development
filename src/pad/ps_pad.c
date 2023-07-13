#include "pad/ps_pad.h"

#include <loadfile.h>
#include <gs_privileged.h>

#include "log/ps_log.h"

void loadPadModules()
{
    int ret;

    ret = SifLoadModule("cdrom0:\\SIO2MAN.IRX", 0, NULL);
    if (ret < 0)
    {
        ERRORLOG("sifLoadModule sio failed: %d", ret);
        // SleepThread();
    }

    ret = SifLoadModule("cdrom0:\\PADMAN.IRX", 0, NULL);
    if (ret < 0)
    {
        ERRORLOG("sifLoadModule pad failed: %d", ret);
        // SleepThread();
    }
}

static void wait_vsync()
{
    // Enable the vsync interrupt.
    *GS_REG_CSR |= GS_SET_CSR(0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

    // Wait for the vsync interrupt.
    while (!(*GS_REG_CSR & (GS_SET_CSR(0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0))))
    {
    }

    // Disable the vsync interrupt.
    *GS_REG_CSR &= ~GS_SET_CSR(0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

static void padWait(int port)
{
    /* Wait for request to complete. */
    while (padGetReqState(port, 0) != PAD_RSTAT_COMPLETE)
        wait_vsync();

    /* Wait for pad to be stable. */
    while (padGetState(port, 0) != PAD_STATE_STABLE)
        wait_vsync();
}

void InitPad(int port, int slot, char *buff)
{
    int ret;
    int acts[2];
    u8 mTable[8];
    u32 ModeTableNum;

    acts[0] = 0;
    acts[1] = 0;

    loadPadModules();

    padInit(0);

    if ((ret = padPortOpen(port, slot, buff)) == 0)
    {
        ERRORLOG("padOpenPort failed: %d", ret);
        // SleepThread();
    }

    padWait(port);

    ModeTableNum = padInfoMode(port, slot, PAD_MODETABLE, -1);

    for (int i = 0; i < ModeTableNum; i++)
    {
        mTable[i] = padInfoMode(port, slot, PAD_MODETABLE, i);
    }

    if ((mTable[0] == PAD_TYPE_DIGITAL) && (mTable[1] == PAD_TYPE_DUALSHOCK) && (ModeTableNum == 2))
    {
        padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
        padWait(port);
    }

    acts[port] = padInfoAct(port, slot, -1, 0);

    if (acts[port] > 0)
    {
        char actAlign[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        u32 i;

        /* Set offsets for motor parameters for SetActDirect. */
        for (i = 0; i < acts[port]; i++)
            actAlign[i] = i;

        padSetActAlign(port, slot, actAlign);
        padWait(port);
    }

    padWait(port);
}
