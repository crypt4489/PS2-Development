#ifndef PS_LINKEDLIST_H
#define PS_LINKEDLIST_H
#include "ps_global.h"

#define LINKEDLIST_END(head, ret) do {\
    ret = head;\
    head = head->next;\
    if (head == NULL)\
        break;\
} while(1)

LinkedList* CreateLinkedListItem(void *data);
LinkedList* AddToLinkedList(LinkedList *head, LinkedList *node);
LinkedList* RemoveNodeFromList(LinkedList *head, LinkedList *node);
LinkedList* CleanLinkedListNode(LinkedList *node);
#endif