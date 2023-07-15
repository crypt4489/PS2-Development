#include "system/ps_timer.h"

#include <kernel.h>
#include <ee_regs.h>
#include <stdlib.h>

float getTicks(TimerStruct *ts)
{
    float ticks = (*R_EE_T0_COUNT + (ts->ctr << 16)) * (1000.0f / (147456000.0f / 256.0f));
    return ticks;
}

int TimerZeroInterrupt(s32 cause, void *arg, void *addr)
{
    TimerStruct *ts = (TimerStruct *)arg;
    ts->ctr++;
    *R_EE_T0_MODE |= 1 << 11;
    return -1;
}

TimerStruct *TimerZeroEnable()
{
    TimerStruct *timer0 = (TimerStruct *)malloc(sizeof(TimerStruct));
    *R_EE_T0_HOLD = 0;
    timer0->ctr = 0;
    s32 id = AddIntcHandler2(9, TimerZeroInterrupt, 0, timer0);
    timer0->id = id;
    EnableIntc(9);
    *R_EE_T0_COUNT = 0;
    *R_EE_T0_MODE |= 0x0282;
    return timer0;
}

void TimerZeroDisable(TimerStruct *timer0)
{
    *R_EE_T0_MODE = 0;
    if (timer0->id >= 0)
    {
        DisableIntc(9);
        RemoveIntcHandler(9, timer0->id);
    }
    free(timer0);
}
