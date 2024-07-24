#include "gs/ps_vrammanager.h"

#include <stdlib.h>

#include "gs/ps_gs.h"
#include "util/ps_linkedlist.h"

VRAMManager *CreateVRAMManager()
{
    VRAMManager *manager = (VRAMManager*)malloc(sizeof(VRAMManager));
    manager->vramSize = GRAPH_VRAM_MAX_WORDS;
    manager->currentTextureBasePtr = 0;
    manager->systemVRAMUsed = 0;
    manager->userVRAMUsed = 0;
    manager->renderTargets = NULL;
    return manager;
}

int AllocateVRAM(VRAMManager *manager, int width, int height, int bpp, bool systemMemory)
{
    int size = graph_vram_size(width, height, bpp, GRAPH_ALIGN_PAGE);
    int allocated = 0;
    
    if (systemMemory)
    {
        allocated = graph_vram_allocate(width, height, bpp, GRAPH_ALIGN_PAGE);
        manager->systemVRAMUsed += size;
    } 
    else
    {
        allocated = manager->systemVRAMUsed + manager->userVRAMUsed;
        manager->userVRAMUsed += size;
    }

    return allocated;

}

void CalculateTextureBasePointer(VRAMManager *manager, RenderTarget *target)
{
	int systemvram = manager->systemVRAMUsed;
	if (systemvram <= target->render->address)
	{
		systemvram = target->render->address;
		systemvram += graph_vram_size(target->render->width, target->render->height, target->render->psm, GRAPH_ALIGN_PAGE);
		if (target->z)
		{
			systemvram += graph_vram_size(target->render->width, target->render->height, target->z->zsm, GRAPH_ALIGN_PAGE);
		}
	}
	manager->currentTextureBasePtr = systemvram;
}


static void MoveAddresses(LinkedList *current, RenderTarget *base)
{
    u32 baseAddress = base->render->address;
    while(current)
    {
        RenderTarget *updated = (RenderTarget*)current->data;
        updated->render->address = baseAddress;
        baseAddress += graph_vram_size(updated->render->width, updated->render->height, updated->render->psm, GRAPH_ALIGN_PAGE);
        if (updated->z)
        {
            updated->z->address = baseAddress;
            baseAddress += graph_vram_size(updated->render->width, updated->render->height, 
                                           updated->z->zsm, GRAPH_ALIGN_PAGE); 
        }
        current = current->next;
    }
}

void AddRenderTargetVRAMManager(VRAMManager *manager, RenderTarget *target)
{
    LinkedList *node = CreateLinkedListItem(target);
	manager->renderTargets = AddToLinkedList(manager->renderTargets, node);
}

void RemoveRenderTargetVRAMManager(VRAMManager *manager, RenderTarget *target)
{
    LinkedList *head = manager->renderTargets;
	LinkedList *list = head;
	while(list)
	{
		if (target == (RenderTarget*)list->data)
		{
            MoveAddresses(list->next, target);
			manager->renderTargets = RemoveNodeFromList(head, list);
            manager->userVRAMUsed -= target->memInVRAM;
			return;
		}
		list = list->next;
	}
}