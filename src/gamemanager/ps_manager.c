#include "gamemanager/ps_manager.h"

#include <graph.h>

#include <string.h>
#include <stdlib.h>
#include <sifrpc.h>

#include "gs/ps_gs.h"
#include "gameobject/ps_gameobject.h"
#include "gamemanager/ps_doublebuffer.h"
#include "textures/ps_texture.h"
#include "dma/ps_dma.h"
#include "system/ps_vumanager.h"
#include "pad/ps_pad.h"
#include "io/ps_file_io.h"
#include "log/ps_log.h"
#include "system/ps_timer.h"
#include "util/ps_linkedlist.h"
#include "textures/ps_texturemanager.h"
#include "graphics/ps_rendertarget.h"
#include "gs/ps_vrammanager.h"

GameManager g_Manager;

// pad;

char padBuf[256] __attribute__((aligned(64)));
u32 port = 0;
u32 slot = 0;
void InitializeSystem(bool useZBuffer, u32 width, u32 height, u32 psm)
{
    InitializeDMAChannels();

    SifInitRpc(0);

    InitDVDDrive();

    InitPad(port, slot, padBuf);

    InitializeManager(width, height, true, 1000, 10, useZBuffer, psm);

    SetupVU1INTEHandler();
}

void CreateManagerRenderTargets(bool useZBuffer, u32 psm)
{
    g_Manager.targetBack = AllocRenderTarget(true);
    g_Manager.targetDisplay = AllocRenderTarget(false);

    if (g_Manager.targetBack == NULL || g_Manager.targetDisplay == NULL)
    {
        ERRORLOG("failed to allocate the rendertargets manager");
    }

    g_Manager.targetBack->z->enable = useZBuffer;

    InitGS(&g_Manager, g_Manager.targetBack->render, g_Manager.targetDisplay->render, g_Manager.targetBack->z, psm);

    g_Manager.targetDisplay->z = g_Manager.targetBack->z;

    SetupRenderTarget(g_Manager.targetDisplay, 1, false);

    SetupRenderTarget(g_Manager.targetBack, 0, false);
}

void CreateManagerStruct(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize) 
{
    g_Manager.ScreenHeight = height;
    g_Manager.ScreenWidth = width;
    g_Manager.ScreenHHalf = height >> 1;
    g_Manager.ScreenWHalf = width >> 1;
    g_Manager.gs_context = 0;

    g_Manager.texManager = CreateTextureManager();

    g_Manager.vramManager = CreateVRAMManager();

    g_Manager.vu1DoneProcessing = true;
    g_Manager.enableDoubleBuffer = doubleBuffer;
    g_Manager.targetBack = NULL;
    g_Manager.targetDisplay = NULL;
    g_Manager.dmabuffers = NULL;
    g_Manager.mainCam = NULL;
    g_Manager.vu1Manager = NULL;

    g_Manager.dmabuffers = CreateDMABuffers(bufferSize);
    g_Manager.vu1Manager = CreateVU1Manager(programSize);
    g_Manager.FPS = 0;
    g_Manager.timer = TimerZeroEnable();
    g_Manager.currentTime = g_Manager.lastTime = getTicks(g_Manager.timer);
}

void InitializeManager(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize, u32 useZBuffer, u32 psm)
{
    CreateManagerStruct(width, height, doubleBuffer, bufferSize, programSize);

    CreateManagerRenderTargets(useZBuffer, psm);
}


void EndFrame(bool useVsync)
{
    static u32 frameCounter = 0;
    static u8 init = 0;
    if (useVsync)
        graph_wait_vsync();

    if (g_Manager.enableDoubleBuffer)
        SwapManagerDrawBuffers();

    SwapManagerDMABuffers();

    if (init)
    {
        g_Manager.currentTime = getTicks(g_Manager.timer);

        if (g_Manager.currentTime > (g_Manager.lastTime + 1000.0f))
        {
            g_Manager.FPS = frameCounter;
            //DEBUGLOG("frames per second %d", g_Manager.FPS);
            g_Manager.lastTime = g_Manager.currentTime;
            frameCounter = 0;
        }
    }
    else
    {
        g_Manager.lastTime = getTicks(g_Manager.timer);
        init = 1;
    }
    frameCounter++;
}


int PollVU1DoneProcessing(GameManager *manager)
{
    DisableIntc(5);
    bool cached = manager->vu1DoneProcessing;
    EnableIntc(5);
    if (!cached)
    {
        return -1;
    }
    return 0;
}

void AddToManagerTexList(GameManager *manager, Texture *tex)
{
    AddToTextureManager(manager->texManager, tex);
}


void ClearManagerStruct(GameManager *manager)
{
    if (manager->vramManager)
        free(manager->vramManager);
    if (manager->texManager)
        free(manager->texManager);
    if (manager->dmabuffers->dma_chains[0])
        packet_free(manager->dmabuffers->dma_chains[0]);
    if (manager->dmabuffers->dma_chains[1])
        packet_free(manager->dmabuffers->dma_chains[1]);
    if (manager->dmabuffers)
        free(manager->dmabuffers);
    if (manager->timer)
        TimerZeroDisable(g_Manager.timer);
}

void SwapManagerDMABuffers()
{
    g_Manager.dmabuffers = SwitchDMABuffers(g_Manager.dmabuffers);
}
