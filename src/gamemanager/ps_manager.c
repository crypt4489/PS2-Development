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

GameManager g_Manager;

// pad;

char padBuf[256] __attribute__((aligned(64)));
u32 port = 0;
u32 slot = 0;
void InitializeSystem(u32 useZBuffer, u32 width, u32 height, u32 psm)
{
    InitializeDMAChannels();

    SifInitRpc(0);

    InitDVDDrive();

    InitPad(port, slot, padBuf);

    InitializeManager(width, height, 1, 1000, 10, useZBuffer, psm);

    SetupVU1INTEHandler();
}

void CreateManagerRenderTargets(u32 useZBuffer, u32 psm)
{
    g_Manager.targetBack = AllocRenderTarget(1);
    g_Manager.targetDisplay = AllocRenderTarget(0);

    if (g_Manager.targetBack == NULL || g_Manager.targetDisplay == NULL)
    {
        ERRORLOG("failed to allocate the rendertargets manager");
    }

    g_Manager.targetBack->z->enable = useZBuffer;

    InitGS(&g_Manager, g_Manager.targetBack->render, g_Manager.targetBack->z, 0, psm);

    InitFramebuffer(g_Manager.targetDisplay->render, g_Manager.targetBack->render->width, g_Manager.targetBack->render->height, g_Manager.targetBack->render->psm);

    g_Manager.targetDisplay->z = g_Manager.targetBack->z;

    SetupRenderTarget(g_Manager.targetDisplay, 1, 0);

    SetupRenderTarget(g_Manager.targetBack, 0, 0);
}

void CreateManagerStruct(u32 width, u32 height, u32 doubleBuffer, u32 bufferSize, u32 programSize) 
{
     g_Manager.ScreenHeight = height;
    g_Manager.ScreenWidth = width;
    g_Manager.ScreenHHalf = height >> 1;
    g_Manager.ScreenWHalf = width >> 1;
    g_Manager.gs_context = 0;

    g_Manager.textureInVram = (Texture *)malloc(sizeof(Texture));
    g_Manager.textureInVram->width = 256;
    g_Manager.textureInVram->height = 256;
    g_Manager.textureInVram->psm = GS_PSM_32;
    g_Manager.textureInVram->id = 0;

    g_Manager.texManager = (TexManager *)malloc(sizeof(TexManager));
    g_Manager.texManager->count = 0;
    g_Manager.texManager->globalIndex = 0;
    g_Manager.texManager->currIndex = -1;

    g_Manager.vu1DoneProcessing = 1;
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

void InitializeManager(u32 width, u32 height, u32 doubleBuffer, u32 bufferSize, u32 programSize, u32 useZBuffer, u32 psm)
{

    CreateManagerStruct(width, height, doubleBuffer, bufferSize, programSize);

    CreateManagerRenderTargets(useZBuffer, psm);

    SetupManagerTexture();
}

void UpdateCurrentTexNameInGS(GameManager *manager, const char *name)
{
    memcpy(manager->textureInVram->name, name, strnlen(name, MAX_CHAR_TEXTURE_NAME));
}

void SetupManagerTexture()
{
    CreateTexBuf(g_Manager.textureInVram, 256, GS_PSM_32);

    CreateTexStructs(g_Manager.textureInVram, g_Manager.textureInVram->width, g_Manager.textureInVram->psm, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE, 0);

    //CreateClutBuf(&g_Manager.textureInVram->clut, 16, GS_PSM_32);

    g_Manager.textureInVram->clut.start = 0;
    g_Manager.textureInVram->clut.load_method = CLUT_LOAD;
    g_Manager.textureInVram->clut.psm = GS_PSM_32;
    g_Manager.textureInVram->clut.storage_mode = CLUT_STORAGE_MODE1;
    g_Manager.textureInVram->clut.address = g_Manager.textureInVram->texbuf.address + graph_vram_size(256, 256, GS_PSM_8, GRAPH_ALIGN_BLOCK );
}

void EndFrame(u32 useVsync)
{
    static u32 frameCounter = 0;
    static u8 init = 0;
    if (useVsync)
        graph_wait_vsync();

    if (g_Manager.enableDoubleBuffer != 0)
        SwapManagerDrawBuffers();

    SwapManagerDMABuffers();

    if (init)
    {
        g_Manager.currentTime = getTicks(g_Manager.timer);

        if (g_Manager.currentTime > (g_Manager.lastTime + 1000.0f))
        {
            g_Manager.FPS = frameCounter;
            DEBUGLOG("frames per second %d", g_Manager.FPS);
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

Texture *GetTexByName(TexManager *manager, const char *name)
{
    LinkedList *iter = manager->list;
    int strLength = strnlen(name, MAX_CHAR_TEXTURE_NAME);
    while (iter != NULL)
    {
        Texture *comp = (Texture *)iter->data;
        if (strncmp(comp->name, name, strLength) == 0)
        {
            return comp;
        }
        iter = iter->next;
    }

    return NULL;
}



int PollVU1DoneProcessing(GameManager *manager)
{
    DisableIntc(5);
    int cached = manager->vu1DoneProcessing;
    EnableIntc(5);
    if (!(cached))
    {
        return -1;
    }
    return 0;
}

void AddToManagerTexList(GameManager *manager, Texture *tex)
{
    tex->clut.address = manager->textureInVram->clut.address;
    tex->texbuf.address = manager->textureInVram->texbuf.address;
    manager->texManager->count++;
    manager->texManager->globalIndex++;
    tex->id = manager->texManager->globalIndex;
    LinkedList *newTex = CreateLinkedListItem(tex);
    manager->texManager->list = AddToLinkedList(manager->texManager->list, newTex);
}

void ClearManagerTexList(GameManager *manager)
{
    LinkedList *iter = manager->texManager->list;

    while (iter != NULL)
    {
        LinkedList *cleanLL = iter;
        iter = iter->next;
        CleanTextureStruct((Texture *)cleanLL->data);
        CleanLinkedListNode(cleanLL);
    }

    manager->texManager->list = NULL;
    manager->texManager->count = 0;
    manager->texManager->globalIndex = 0;
}

void ClearManagerStruct(GameManager *manager)
{
    if (manager->texManager != NULL)
        free(manager->texManager);
    if (manager->textureInVram != NULL)
        free(manager->textureInVram);
    if (manager->dmabuffers->dma_chains[0] != NULL)
        packet_free(manager->dmabuffers->dma_chains[0]);
    if (manager->dmabuffers->dma_chains[1] != NULL)
        packet_free(manager->dmabuffers->dma_chains[1]);
    if (manager->dmabuffers != NULL)
        free(manager->dmabuffers);
    if (manager->timer != NULL)
        TimerZeroDisable(g_Manager.timer);
}

Texture *GetTexObjFromTexList(GameManager *manager, int index)
{
    LinkedList *iter = manager->texManager->list;
    int curr = index;

    while (curr > 0)
    {
        iter = iter->next;
        curr--;
    }

    return (Texture *)iter->data;
}

void SwapManagerDMABuffers()
{
    g_Manager.dmabuffers = SwitchDMABuffers(g_Manager.dmabuffers);
}
