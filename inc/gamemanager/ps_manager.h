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
void* PeekQueue(Queue *queue);
void* PopQueue(Queue *queue);
void AddQueueElement(Queue *queue);
Queue* CreateQueue(u32 maxCount, u32 type);
void SwapManagerDMABuffers();
LinkedList* CleanLinkedListNode(LinkedList *node);
void UpdateCurrentTexNameInGS(GameManager *manager, const char *name);
void EndFrame();

inline void SetGlobalManagerCam(Camera *cam)
{
    g_Manager.mainCam = cam;
};

#endif
