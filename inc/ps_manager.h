#ifndef PS_MANAGER_H
#define PS_MANAGER_H
#include "ps_global.h"

void InitializeSystem();
void CreateManagerRenderTargets();
void SetupManagerTexture();
void InitializeManager(u32 width, u32 height, u32 doubleBuffer, u32 bufferSize, u32 programSize);
void AddToManagerTexList(GameManager *manager, Texture *tex);
void ClearManagerTexList(GameManager *manager);
Texture* GetTexObjFromTexList(GameManager *manager, int index);
int PollVU1DoneProcessing(GameManager *manager);
Texture *GetTexByName(TexManager *manager, const char *name);
void ClearManagerStruct(GameManager *manager);
LinkedList* CreateLinkedListItem(void *data);
LinkedList* AddToLinkedList(LinkedList *head, LinkedList *node);
LinkedList* RemoveNodeFromList(LinkedList *head, LinkedList *node);
void SwapManagerDMABuffers();
LinkedList* CleanLinkedListNode(LinkedList *node);;


void EndFrame();

inline void SetGlobalManagerCam(Camera *cam)
{
    g_Manager.mainCam = cam;
};

inline void UpdateCurrentTexNameInGS(GameManager *manager, const char *name)
{
    strncpy(manager->textureInVram->name, name, MAX_CHAR_TEXTURE_NAME);
};

#endif
