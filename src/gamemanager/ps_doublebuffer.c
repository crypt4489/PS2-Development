#include "gamemanager/ps_doublebuffer.h"

#include <graph.h>

#include "gamemanager/ps_manager.h"
#include "gs/ps_gs.h"

void SwapManagerFrameBuffers()
{
    RenderTarget *tempTarget = g_Manager.targetBack;

    if (g_Manager.fsaaEnable)
    {
        *GS_REG_DISPFB1 =  GS_SET_DISPFB(g_Manager.targetBack->render->address>>11,g_Manager.targetBack->render->width>>6,g_Manager.targetBack->render->psm,0,0);

	    *GS_REG_DISPFB2 = GS_SET_DISPFB(g_Manager.targetDisplay->render->address>>11,g_Manager.targetDisplay->render->width>>6,g_Manager.targetDisplay->render->psm,0,1);
    } else {
        *GS_REG_DISPFB1 =  GS_SET_DISPFB(g_Manager.targetBack->render->address>>11,g_Manager.targetBack->render->width>>6,g_Manager.targetBack->render->psm,0,0);
        
        *GS_REG_DISPFB2 =  GS_SET_DISPFB(g_Manager.targetBack->render->address>>11,g_Manager.targetBack->render->width>>6,g_Manager.targetBack->render->psm,0,1);
    }
    // swap render targets
    g_Manager.targetBack = g_Manager.targetDisplay;
    g_Manager.targetDisplay = tempTarget;

    g_Manager.gs_context ^= 1;
}