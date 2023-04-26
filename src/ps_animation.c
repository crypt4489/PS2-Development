#include "ps_animation.h"

AnimationData * GetAnimationByIndex(LinkedList *animations, u32 index)
{
    u32 indexZero = index - 1;
    LinkedList *ret = animations;
    while(indexZero > 0)
    {
        ret = ret->next;
        indexZero--;
    }
    return (AnimationData*)ret->data;
}