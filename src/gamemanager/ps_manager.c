#include "gamemanager/ps_manager.h"

#include <graph.h>

#include <string.h>
#include <stdlib.h>

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
extern u32 port;
extern u32 slot;
extern char padBuf[256];

void InitializeSystem()
{
    InitializeDMAChannels();

    InitDVDDrive();

    InitPad(port, slot, padBuf);

    InitializeManager(640, 480, 1, 1000, 10);

    SetupVU1INTEHandler();
}

void CreateManagerRenderTargets()
{
    g_Manager.targetBack = AllocRenderTarget();
    g_Manager.targetDisplay = AllocRenderTarget();

    if (g_Manager.targetBack == NULL || g_Manager.targetDisplay == NULL)
    {
        ERRORLOG("failed to allocate the rendertargets manager");
    }

    InitGS(&g_Manager, g_Manager.targetBack->render, g_Manager.targetBack->z, 0);

    InitFramebuffer(g_Manager.targetDisplay->render, g_Manager.targetBack->render->width, g_Manager.targetBack->render->height, g_Manager.targetBack->render->psm);

    g_Manager.targetDisplay->z = g_Manager.targetBack->z;

    SetupRenderTarget(g_Manager.targetDisplay, 1, 0);

    SetupRenderTarget(g_Manager.targetBack, 0, 0);
}

void InitializeManager(u32 width, u32 height, u32 doubleBuffer, u32 bufferSize, u32 programSize)
{

    g_Manager.ScreenHeight = height;
    g_Manager.ScreenWidth = width;
    g_Manager.ScreenHHalf = height / 2;
    g_Manager.ScreenWHalf = width / 2;
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

    CreateManagerRenderTargets();

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

    CreateClutBuf(&g_Manager.textureInVram->clut, 16, GS_PSM_32);

    g_Manager.textureInVram->clut.start = 0;
    g_Manager.textureInVram->clut.load_method = CLUT_LOAD;
    g_Manager.textureInVram->clut.psm = GS_PSM_32;
    g_Manager.textureInVram->clut.storage_mode = CLUT_STORAGE_MODE1;
    g_Manager.textureInVram->clut.address = g_Manager.textureInVram->texbuf.address + graph_vram_size(256, 256, GS_PSM_8, GRAPH_ALIGN_BLOCK );
}

void EndFrame()
{
    static u32 frameCounter = 0;
    static u8 init = 0;

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
            // DEBUGLOG("frames per second %d", g_Manager.FPS);
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
    if (!(manager->vu1DoneProcessing))
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
    if (manager->texManager)
        free(manager->texManager);
    if (manager->textureInVram)
        free(manager->textureInVram);
    if (manager->dmabuffers->dma_chains[0])
        packet_free(manager->dmabuffers->dma_chains[0]);
    if (manager->dmabuffers->dma_chains[1])
        packet_free(manager->dmabuffers->dma_chains[1]);
    if (manager->dmabuffers)
        free(manager->dmabuffers);
    if (manager->timer)
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
