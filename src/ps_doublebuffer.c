#include "ps_doublebuffer.h"
#include "ps_manager.h"
#include "ps_gs.h"
#include <graph.h>

void SwapManagerDrawBuffers()
{
    //framebuffer_t *f_temp = g_Manager.targetDisplay->render;

    RenderTarget *tempTarget = g_Manager.targetBack;

   // while(PollVU1DoneProcessing(&g_Manager) < 0);

    graph_set_framebuffer_filtered(g_Manager.targetBack->render->address, g_Manager.targetBack->render->width, g_Manager.targetBack->render->psm, 0, 0);
      
    //swap render targets
    g_Manager.targetBack = g_Manager.targetDisplay; 
    g_Manager.targetDisplay = tempTarget;

    g_Manager.gs_context = (g_Manager.gs_context == 1) ? 0 : 1;


}