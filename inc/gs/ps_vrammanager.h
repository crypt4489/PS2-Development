#ifndef PS_VRAMMANAGER_H
#define PS_VRAMMANAGER_H
#include "ps_global.h"

VRAMManager *CreateVRAMManager();
int AllocateVRAM(VRAMManager *manager, int width, int height, int bpp, bool systemMemory);
void CalculateTextureBasePointer(VRAMManager *manager, RenderTarget *target);
void AddRenderTargetVRAMManager(VRAMManager *manager, RenderTarget *target);
void RemoveRenderTargetVRAMManager(VRAMManager *manager, RenderTarget *target);
#endif