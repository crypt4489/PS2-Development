#include "ps_doublebuffer.h"
#include "ps_manager.h"
#include "ps_gs.h"
#include <graph.h>

void SwapManagerDrawBuffers()
{
    RenderTarget *tempTarget = g_Manager.targetBack;

    graph_set_framebuffer_filtered(g_Manager.targetBack->render->address, g_Manager.targetBack->render->width, g_Manager.targetBack->render->psm, 0, 0);

    // swap render targets
    g_Manager.targetBack = g_Manager.targetDisplay;
    g_Manager.targetDisplay = tempTarget;

    g_Manager.gs_context = (g_Manager.gs_context == 1) ? 0 : 1;
}