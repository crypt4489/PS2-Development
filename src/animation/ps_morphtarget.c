#include "animation/ps_morphtarget.h"

#include <stdlib.h>

#include "gameobject/ps_gameobject.h"

#include "system/ps_vumanager.h"
#include "io/ps_file_io.h"
#include "world/ps_lights.h"
#include "gs/ps_gs.h"
#include "dma/ps_dma.h"
#include "pipelines/ps_vu1pipeline.h"
#include "log/ps_log.h"

void CreateMorphTargetBuffersFromFile(const char *targetFile, MeshBuffers *buffer)
{
    ReadModelFile(targetFile, buffer);
}

MorphTargetBuffer *CreateMorphTargetBuffer(u32 bufferSize)
{
    MorphTargetBuffer *buffer = (MorphTargetBuffer *)malloc(sizeof(MorphTargetBuffer));
    if (buffer == NULL)
    {
        ERRORLOG("failed to allocate target buffer");
        return NULL;
    }

    buffer->morph_targets = malloc(sizeof(MeshBuffers *) * bufferSize);
    if (buffer->morph_targets == NULL)
    {
        ERRORLOG("failed to allocate target buffer");
        free(buffer);
        return NULL;
    }

    buffer->meshCap = bufferSize;
    buffer->meshCount = buffer->interpCount = buffer->interpCap = 0;
    buffer->intCb = GenericUpdateInterpolatorNode;
    buffer->mtCb = GenericUpdateMorphBuffer;
    return buffer;
}

GameObject *CreateObjectMorphBuffer(GameObject *obj, u32 bufferSize)
{
    obj->interpolator = CreateMorphTargetBuffer(bufferSize);
    if (obj->interpolator == NULL)
    {
        ERRORLOG("object interpolator failed to allocate");
        return obj;
    }

    if (AddMeshToTargetBuffer(obj->interpolator, &obj->vertexBuffer) == 0)
    {
        ERRORLOG("object interpolator failed to add base mesh");
    }

    return obj;
}

MorphTargetBuffer *CreateInterpolatorNodes(MorphTargetBuffer *buffer, u32 interpolatorSize)
{
    buffer->interpolators = malloc(sizeof(Interpolator *) * interpolatorSize);
    if (buffer->interpolators == NULL)
    {
        ERRORLOG("failed to create interpolators array");
        return buffer;
    }

    buffer->interpCap = interpolatorSize;

    return buffer;
}

u32 AddMeshToTargetBuffer(MorphTargetBuffer *buffer, MeshBuffers *mesh)
{
    u32 count = GetMorphMeshCount(buffer);
    if (count + 1 > GetMorphMeshCapacity(buffer))
    {
        ERRORLOG("cannot add mesh to morph buffer");
        return 0;
    }

    buffer->morph_targets[count] = mesh;
    buffer->meshCount++;

    return 1;
}

void AddInterpolatorNode(MorphTargetBuffer *buffer, u16 _start, u16 _end, float _scale)
{

    u32 count = GetMorphInterpolatorNodesCount(buffer);
    if (count + 1 > GetMorphInterpolatorNodesCapacity(buffer))
    {
        ERRORLOG("Morph Buffer interpolator overflow");
        return;
    }

    Interpolator *node = (Interpolator *)malloc(sizeof(Interpolator));
    if (node == NULL)
    {
        ERRORLOG("cannot create interpolator node");
        return;
    }

    node->begin = _start;
    node->end = _end;
    node->scale = _scale;
    node->status = ANIM_PAUSE;
    node->position = 1.0f;

    buffer->interpolators[count] = node;
    buffer->interpCount++;
}

MorphTargetBuffer *SetInterpolatorNode(MorphTargetBuffer *buffer, u32 num)
{
    if (num > GetMorphInterpolatorNodesCount(buffer))
    {
        ERRORLOG("Num is greater than interpolator count");
        return buffer;
    }

    buffer->currInterpNode = num;
    buffer->interpolators[num]->status = ANIM_RUNNING;

    return buffer;
}

Interpolator *GetInterpolatorNode(MorphTargetBuffer *buffer, u32 num)
{
    if (num > GetMorphInterpolatorNodesCount(buffer))
    {
        ERRORLOG("num is greater than interpolator count");
        return NULL;
    }

    return buffer->interpolators[num];
}

MeshBuffers *GetMorphMeshBuffer(MorphTargetBuffer *buffer, u32 num)
{
    if (num > GetMorphMeshCount(buffer))
    {
        ERRORLOG("num is greater than mesh interpolator count");
        return NULL;
    }

    return buffer->morph_targets[num];
}

Interpolator *GetCurrentInterpolatorNode(MorphTargetBuffer *buffer)
{
    return buffer->interpolators[buffer->currInterpNode];
}

void SetInterpolatorCallback(MorphTargetBuffer *buffer, interpolator_callback cb)
{
    buffer->intCb = cb;
}

void SetMorphTargetCallback(MorphTargetBuffer *buffer, morph_target_callback cb)
{
    buffer->mtCb = cb;
}

void ExecuteMorphTargetCBFuncs(MorphTargetBuffer *buffer)
{
    if (buffer == NULL)
        return;

    Interpolator *node = GetCurrentInterpolatorNode(buffer);

    if (buffer->intCb(node))
    {
        buffer->mtCb(buffer);
    }
}

u32 GenericUpdateInterpolatorNode(Interpolator *node)
{
    if (node->status == ANIM_RUNNING)
    {
        if (node->position < 0.0f)
        {
            node->status = ANIM_STOPPED;
            node->position = 1.0f;
            return 1;
        }
    }
    else if (node->status == ANIM_STOPPED)
    {
        // node->position = 1.0f;
    }

    return 0;
}

void GenericUpdateMorphBuffer(MorphTargetBuffer *buffer)
{
    u32 numInterps = GetMorphInterpolatorNodesCount(buffer);
    u32 newIndex, currIndex;
    currIndex = GetMorphCurrentInterpolatorNode(buffer);
    newIndex = (currIndex + 1) % numInterps;
    SetInterpolatorNode(buffer, newIndex);
}
