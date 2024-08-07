#include "util/ps_linkedlist.h"

#include <stdlib.h>
#include "log/ps_log.h"

LinkedList *CreateLinkedListItem(void *data)
{
    LinkedList *node = (LinkedList *)malloc(sizeof(LinkedList));
    node->next = NULL;
    node->data = data;
    return node;
}

LinkedList *AddToLinkedList(LinkedList *head, LinkedList *node)
{
    if (!head)
    {
        head = node;
        return head;
    }

    LinkedList *iter = head;
    while (iter->next)
    {
        iter = iter->next;
    }

    iter->next = node;

    return head;
}

LinkedList *RemoveNodeFromList(LinkedList *head, LinkedList *node)
{
    LinkedList **iter = &head;

    while (*iter != node)
    {
        iter = &(*iter)->next;
        if (!(*iter))
        {
            ERRORLOG("Cannot find node in list to remove");
            return head;
        }
    }

    *iter = CleanLinkedListNode(*iter);

    return head;
}

LinkedList *CleanLinkedListNode(LinkedList *node)
{
    LinkedList *ret = NULL;
    if (node)
    {
        ret = node->next;
        node->data = NULL;
        free(node);
    }
    else
    {
        ERRORLOG("Cannot remove NULL pointer from LinkedList");
    }

    return ret;
}
