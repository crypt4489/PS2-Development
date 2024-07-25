#include "gamemanager/ps_doublebuffer.h"

#include <graph.h>

#include "gamemanager/ps_manager.h"
#include "gs/ps_gs.h"

void SwapManagerDrawBuffers()
{
    RenderTarget *tempTarget = g_Manager.targetBack;

    graph_set_framebuffer_filtered(g_Manager.targetBack->render->address, g_Manager.targetBack->render->width, g_Manager.targetBack->render->psm, 0, 0);

    // swap render targets
    g_Manager.targetBack = g_Manager.targetDisplay;
    g_Manager.targetDisplay = tempTarget;

    g_Manager.gs_context = g_Manager.gs_context ^ 0x01;
}