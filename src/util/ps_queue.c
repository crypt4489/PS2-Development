#include "util/ps_queue.h"

#include <stdlib.h>

#include "util/ps_linkedlist.h"
#include "log/ps_log.h"

void* PeekQueue(Queue *queue)
{
    void *ret = NULL;

    if (queue->type == FIFO)
    {
        LinkedList *head = queue->top;
        ret = head->data;
    }
    else if (queue->type == LIFO)
    {
        LinkedList *bottom;
        LinkedList *head = queue->top;
        LINKEDLIST_END(head, bottom);
        ret = bottom->data;
    }
    else
    {
        ERRORLOG("invalid queue type");
    }

    return ret;
}

void* PopQueue(Queue *queue)
{
    void *ret = NULL;
    if (queue->type == FIFO)
    {
        LinkedList *head = queue->top;
        ret = head->data;
        queue->top = RemoveNodeFromList(queue->top, head);
    }
    else if (queue->type == LIFO)
    {
        LinkedList *bottom;
        LinkedList *head = queue->top;
        LINKEDLIST_END(head, bottom);
        ret = bottom->data;
        queue->top = RemoveNodeFromList(queue->top, bottom);
    }
    else
    {
        ERRORLOG("invalid queue type");
    }

    return ret;
}

void AddQueueElement(Queue *queue, void* element)
{
    LinkedList *node = CreateLinkedListItem(element);
    queue->top = AddToLinkedList(queue->top, node);
}

Queue* CreateQueue(u32 maxCount, u32 type)
{
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->top = NULL;
    q->count = 0;
    q->maxCount = maxCount;
    q->type = type;
    return q;
}
