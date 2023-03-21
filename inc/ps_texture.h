#ifndef PSTEXTURE_H
#define PSTEXTURE_H

#include "ps_global.h"


#define PS_FILTER_NNEIGHBOR 0
#define PS_FILTER_BILINEAR 1


void CleanTextureStruct(Texture *tex);
void addStringNameToTexture(Texture *tex, const char *buffer);
Texture* AddAndCreateTexture(const char *filePath, u32 readType, u8 useProgrammedAlpha, u8 alphaVal, u32 mode);
u32 compareTextureNames(Texture *tex1, Texture *tex2);
qword_t* CreateTexChain(qword_t *input, Texture *tex);
qword_t *CreateTexChainWOTAGS(qword_t *input, Texture *tex);
void UploadTextureViaManagerToVRAM(Texture *tex);
//void UploadTextureViaManagerToVRAM(Texture *tex);
Texture *SetTextureFilter(Texture *tex, u8 filter);
u32 GetTextureIDByName(const char *name, TexManager *TexManager);
Texture *GetTextureByID(u32 id, TexManager *texManager);
#endif
