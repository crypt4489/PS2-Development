#ifndef PS_MANAGER_H
#define PS_MANAGER_H
#include "ps_global.h"

void InitializeSystem(bool useZBuffer, u32 width, u32 height, u32 psm);
void CreateManagerRenderTargets(bool useZBuffer, u32 psm);
void SetupManagerTexture();
void InitializeManager(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize, u32 useZBuffer, u32 psm);
void AddToManagerTexList(GameManager *manager, Texture *tex);
Texture* GetTexObjFromTexList(GameManager *manager, int index);
int PollVU1DoneProcessing(GameManager *manager);
void ClearManagerStruct(GameManager *manager);
void SwapManagerDMABuffers();
void EndFrame(bool useVsync);
void CreateManagerStruct(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize) ;

inline void SetGlobalManagerCam(Camera *cam)
{
    g_Manager.mainCam = cam;
};

#endif
