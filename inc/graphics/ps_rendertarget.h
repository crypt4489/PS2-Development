#ifndef PS_RENDERTARGET_H
#define PS_RENDERTARGET_H
#include "ps_global.h"
void SetupRenderTarget(RenderTarget *target, int context, bool wait);
RenderTarget *CreateRenderTarget(int height, int width, int zsm, int zmethod, int psm);
Texture *CreateTextureFromRenderTarget(RenderTarget *target, u32 filter, u32 function);
RenderTarget *AllocRenderTarget(bool useZBuffer);
void DestroyRenderTarget(RenderTarget *target, bool system);

#endif
