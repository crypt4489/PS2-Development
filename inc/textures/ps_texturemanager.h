#ifndef PS_TEXTUREMANAGER_H
#define PS_TEXTUREMANAGER_H
#include "ps_global.h"
void CleanTextureManager(TextureManager *manager);
Texture *GetTexByName(TextureManager *manager, const char *name);
TextureManager *CreateTextureManager();
void AddToTextureManager(TextureManager *manager, Texture *tex);
u64 GetTextureIDByName(TextureManager *manager, const char *name);
Texture *GetTextureByID(TextureManager *manager, u64 id);
#endif