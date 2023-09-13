#ifndef PS_QUEUE_H
#define PS_QUEUE_H
#include "ps_global.h"
void* PeekQueue(Queue *queue);
void* PopQueue(Queue *queue);
void AddQueueElement(Queue *queue);
Queue* CreateQueue(u32 maxCount, u32 type);
#endif