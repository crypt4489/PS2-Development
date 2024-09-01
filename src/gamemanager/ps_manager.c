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
#include "gameobject/ps_ltm.h"
#include "math/ps_matrix.h"
#include "math/ps_plane.h"
#include "math/ps_vector.h"
#include "camera/ps_camera.h"

GameManager g_Manager;

extern u32 VU1_LightStage3_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_LightStage3_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_GenericMorphTargetStage13D_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_GenericMorphTargetStage13D_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_EnvMapStage2_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_EnvMapStage2_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_AnimTexStage2_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_AnimTexStage2_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_SpecularLightStage3_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_SpecularLightStage3_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ClippingStage_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ClippingStage_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ClipStage4_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ClipStage4_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_GenericBonesAnimStage1_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_GenericBonesAnimStage1_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ShadowExtrusion_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ShadowExtrusion_CodeEnd __attribute__((section(".vudata")));

// pad;

char padBuf[256] __attribute__((aligned(64)));
u32 port = 0;
u32 slot = 0;

void InitializeSystem(ManagerInfo *info)
{
    InitializeDMAChannels();

    SifInitRpc(0);

    InitDVDDrive();

    InitPad(port, slot, padBuf);

    SetupVU1INTEHandler();

    *R_EE_GIF_CTRL |= 1;

    *R_EE_VIF1_FBRST |= 1;

    *GS_REG_CSR |= 0x100;

    *R_EE_GIF_MODE |= 0x04;
    

    InitializeManager(info);
}


void CreateManagerRenderTargets(bool useZBuffer, u32 psm, u32 zsm)
{
    g_Manager.targetBack = AllocRenderTarget(useZBuffer);
    g_Manager.targetDisplay = AllocRenderTarget(false);

    if (!g_Manager.targetBack || !g_Manager.targetDisplay)
    {
        ERRORLOG("failed to allocate the rendertargets manager");
    }

    InitGS(&g_Manager, g_Manager.targetBack->render, 
            g_Manager.targetDisplay->render, 
            g_Manager.targetBack->z, psm, zsm);

    g_Manager.targetDisplay->z = g_Manager.targetBack->z;

    SetupRenderTarget(g_Manager.targetDisplay, 1, false);

    SetupRenderTarget(g_Manager.targetBack, 0, false);
}

static void SetupVU1Programs()
{

    VU1Program *prog;

    prog = CreateVU1Program(&VU1_ClipStage4_CodeStart, &VU1_ClipStage4_CodeEnd, 0); // 0

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_LightStage3_CodeStart, &VU1_LightStage3_CodeEnd, 0); // 1

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_GenericMorphTargetStage13D_CodeStart, &VU1_GenericMorphTargetStage13D_CodeEnd, 0); // 2

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_EnvMapStage2_CodeStart, &VU1_EnvMapStage2_CodeEnd, 0); // 3

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_AnimTexStage2_CodeStart, &VU1_AnimTexStage2_CodeEnd, 0); // 4

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_SpecularLightStage3_CodeStart, &VU1_SpecularLightStage3_CodeEnd, 0); // 5

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_ClippingStage_CodeStart, &VU1_ClippingStage_CodeEnd, 0); // 6

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_GenericBonesAnimStage1_CodeStart, &VU1_GenericBonesAnimStage1_CodeEnd, 0); // 7

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_ShadowExtrusion_CodeStart, &VU1_ShadowExtrusion_CodeEnd, 0); // 8

    AddProgramToManager(g_Manager.vu1Manager, prog);
}


void CreateManagerStruct(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize, bool fsaa) 
{
    g_Manager.ScreenHeight = height;
    g_Manager.ScreenWidth = width;
    g_Manager.ScreenHHalf = height >> 1;
    g_Manager.ScreenWHalf = width >> 1;
    g_Manager.gs_context = 0;
    g_Manager.fsaaEnable = fsaa;

    g_Manager.texManager = CreateTextureManager();

    g_Manager.vramManager = CreateVRAMManager();

    g_Manager.vu1DoneProcessing = true;
    g_Manager.enableDoubleBuffer = doubleBuffer;
    g_Manager.targetBack = NULL;
    g_Manager.targetDisplay = NULL;
    g_Manager.vu1Manager = NULL;

    g_Manager.drawBuffers = CreateDrawBuffers(bufferSize);
    g_Manager.dmaBuffers = CreateDMABuffers();
    g_Manager.vu1Manager = CreateVU1Manager(programSize);
    g_Manager.FPS = 0;
    g_Manager.timer = TimerZeroEnable();
    g_Manager.currentTime = g_Manager.lastTime = getTicks(g_Manager.timer);

    SetupVU1Programs();
}

void InitializeManager(ManagerInfo *info)
{
    CreateManagerStruct(info->width, info->height, info->doublebuffered, 
                        info->drawBufferSize, info->vu1programsize, info->fsaa);

    CreateManagerRenderTargets(info->zenable, info->psm, info->zsm);
}

void StartFrame()
{
    if (GetDirtyLTM(g_DrawCamera->ltm))
    {
        CreateCameraWorldMatrix(g_DrawCamera, g_DrawCamera->world);
        MatrixIdentity(g_DrawCamera->viewProj);
        MatrixMultiply(g_DrawCamera->viewProj, g_DrawCamera->viewProj, g_DrawCamera->view);
        MatrixMultiply(g_DrawCamera->viewProj, g_DrawCamera->viewProj, g_DrawCamera->proj);
        CreateCameraQuat(g_DrawCamera, g_DrawCamera->quat);
        for (int i = 0; i < 6; i++)
        {
            VECTOR normal;
            Matrix3VectorMultiply(normal, g_DrawCamera->world, g_DrawCamera->frus[0]->sides[i].planeEquation);
            MatrixVectorMultiply(g_DrawCamera->frus[1]->sides[i].pointInPlane, g_DrawCamera->world, g_DrawCamera->frus[0]->sides[i].pointInPlane);

            Normalize(normal, normal);

            ComputePlane(g_DrawCamera->frus[1]->sides[i].pointInPlane, 
                        normal, 
                        g_DrawCamera->frus[1]->sides[i].planeEquation);
        }
    }
}


void EndFrame(bool useVsync)
{

    qword_t *q = g_Manager.drawBuffers->currentvif;
    qword_t *direct = q;
    q = CreateDirectTag(q, 0, 1);
    q = draw_finish(q);
    AddSizeToDirectTag(direct, q-direct-1);
    SubmitDrawBuffersToController(q, DMA_CHANNEL_VIF1, 0, 0);
    draw_wait_finish();
    static u32 frameCounter = 0;
    static u8 init = 0;
    if (useVsync)
        graph_wait_vsync();

    if (g_Manager.enableDoubleBuffer)
        SwapManagerFrameBuffers();

    SwapManagerDrawBuffers();

    g_Manager.dmaBuffers->tospr = g_Manager.dmaBuffers->tosprtape;

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
    if (manager->targetBack)
        DestroyRenderTarget(manager->targetBack, true);
    if (manager->targetDisplay)
        DestroyRenderTarget(manager->targetDisplay, true);
    if (manager->vramManager)
        DeleteVRAMManager(manager->vramManager);
    if (manager->texManager)
        CleanTextureManager(manager->texManager);
    if (manager->drawBuffers)
        DestroyDrawBuffers(manager->drawBuffers);
    if (manager->vu1Manager)
        DestroyVU1Manager(manager->vu1Manager);
    if (manager->timer)
        TimerZeroDisable(g_Manager.timer);

    graph_vram_clear();
}

void SwapManagerDrawBuffers()
{
    SwitchDrawBuffers(g_Manager.drawBuffers);
}
