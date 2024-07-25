#include "textures/ps_texturemanager.h"
#include "textures/ps_texture.h"
#include "util/ps_hashmap.h"
#include "log/ps_log.h"

#include <string.h>
#include <stdlib.h>

u64 TextureManagerHashFunction(const char* key, int size)
{
    u64 hashval = 0;
    for(int i = 0; i<size; i++)
    {
        hashval = (hashval << 4) + key[i];
        long g = hashval & 0xF0000000L;
        if (g != 0) hashval *= (g >> 24);
        hashval &= ~g; 
    }
    return hashval;
}

void ClearManagerTexList(TextureManager *manager)
{
    ClearHashMap(manager->textureMap->trees, true, manager->textureMap->cap);
    manager->count = 0;
    manager->currIndex = -1;
}

Texture *GetTexByName(TextureManager *manager, const char *name)
{
    int strLength = strnlen(name, MAX_CHAR_TEXTURE_NAME);
    HashEntry *entry = GetFromHashMap(manager->textureMap, name, strLength);
    if (entry)
        return entry->data;
    return NULL;
}

TextureManager *CreateTextureManager()
{
    TextureManager *texManager = (TextureManager *)malloc(sizeof(TextureManager));
    texManager->count = 0;
    texManager->currIndex = -1;
    texManager->textureMap = CreateHashMap(1024, TextureManagerHashFunction);
    return texManager;
}

void AddToTextureManager(TextureManager *manager, Texture *tex)
{
    u32 nameLength = strnlen(tex->name, MAX_CHAR_TEXTURE_NAME);
    manager->count++;
    tex->id = TextureManagerHashFunction(tex->name, nameLength);
    InsertHashMap(manager->textureMap, tex->name, nameLength, tex);
}

u32 GetTextureIDByName(TextureManager *manager, const char *name)
{
    Texture *tex = GetTexByName(manager, name);
    if (tex == NULL)
    {
        ERRORLOG("Missed Texture for Texture ID by Name");
        return 0;
    }
    return tex->id;
}

Texture *GetTextureByID(TextureManager *manager, u32 id)
{
    HashEntry *entry = GetFromHashMapByCode(manager->textureMap, id);
    if (entry)
        return entry->data;
    
    ERRORLOG("Missed Texture for Texture By ID");
    return NULL;
}

