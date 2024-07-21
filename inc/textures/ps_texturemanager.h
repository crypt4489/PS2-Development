#ifndef PS_TEXTUREMANAGER_H
#define PS_TEXTUREMANAGER_H
#include "ps_global.h"
void ClearTextureManagerList(TextureManager *manager);
Texture *GetTexByName(TextureManager *manager, const char *name);
TextureManager *CreateTextureManager();
void AddToTextureManager(TextureManager *manager, Texture *tex);
u32 GetTextureIDByName(TextureManager *manager, const char *name);
Texture *GetTextureByID(TextureManager *manager, u32 id);
#endif