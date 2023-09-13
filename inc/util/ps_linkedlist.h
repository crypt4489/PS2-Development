#ifndef PS_LINKEDLIST_H
#define PS_LINKEDLIST_H
#include "ps_global.h"
LinkedList* CreateLinkedListItem(void *data);
LinkedList* AddToLinkedList(LinkedList *head, LinkedList *node);
LinkedList* RemoveNodeFromList(LinkedList *head, LinkedList *node);
LinkedList* CleanLinkedListNode(LinkedList *node);
#endif