#ifndef PS_MANAGER_H
#define PS_MANAGER_H
#include "ps_global.h"

void InitializeSystem(ManagerInfo *info);
void CreateManagerRenderTargets(bool useZBuffer, u32 psm, u32 zsm);
void SetupManagerTexture();
void InitializeManager(u32 width, u32 height, 
                       bool doubleBuffer, u32 bufferSize, 
                       u32 programSize, bool useZBuffer, u32 psm, u32 zsm);
void AddToManagerTexList(GameManager *manager, Texture *tex);
Texture* GetTexObjFromTexList(GameManager *manager, int index);
int PollVU1DoneProcessing(GameManager *manager);
void ClearManagerStruct(GameManager *manager);
void SwapManagerDMABuffers();
void EndFrame(bool useVsync);
void CreateManagerStruct(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize) ;
void StartFrame();


#endif
