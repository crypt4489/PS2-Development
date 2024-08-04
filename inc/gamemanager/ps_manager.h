#ifndef PS_MANAGER_H
#define PS_MANAGER_H
#include "ps_global.h"

void InitializeSystem(ManagerInfo *info);
void InitializeManager(ManagerInfo *info);
void CreateManagerRenderTargets(bool useZBuffer, u32 psm, u32 zsm);
void AddToManagerTexList(GameManager *manager, Texture *tex);
Texture* GetTexObjFromTexList(GameManager *manager, int index);
int PollVU1DoneProcessing(GameManager *manager);
void ClearManagerStruct(GameManager *manager);
void SwapManagerDrawBuffers();
void EndFrame(bool useVsync);
void CreateManagerStruct(u32 width, u32 height, bool doubleBuffer, u32 bufferSize, u32 programSize, bool fsaa);
void StartFrame();


#endif
