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
    if (!queue->top)
        return ret;
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
        return ret;
    }
    queue->count--;
    return ret;
}

void AddQueueElement(Queue *queue, void* element)
{
    if (!element)
    {
        ERRORLOG("PAssed a null element to AddQueueElement");
        return;
    }


    if (queue->maxCount >= queue->count + 1) {
        LinkedList *node = CreateLinkedListItem(element);
        queue->top = AddToLinkedList(queue->top, node);
        queue->count++;
    }

}

Queue* CreateQueue(u32 maxCount, u32 type)
{
    Queue *q = (Queue*)malloc(sizeof(Queue));
    if (!q)
    {
        ERRORLOG("Cannot create queue!");
        return NULL;
    }
    q->top = NULL;
    q->count = 0;
    q->maxCount = maxCount;
    q->type = type;
    return q;
}

void DeleteQueue(Queue *queue)
{
    LinkedList *iter = queue->top;
    while(iter)
    {
        iter = CleanLinkedListNode(iter);
    }
    free(queue);
}
