#include "pipelines/ps_vu1pipeline.h"

#include <draw.h>

#include <string.h>
#include <stdlib.h>

#include "graphics/ps_drawing.h"
#include "camera/ps_camera.h"
#include "dma/ps_dma.h"
#include "textures/ps_texture.h"
#include "log/ps_log.h"

#include "textures/ps_texturemanager.h"

void ExecutePipelineCBs(GameObject *obj, VU1Pipeline *pipe)
{
    PipelineCallback **link = pipe->cbs;
    for (int i = 0; i < pipe->numberCBS; i++)
    {
        link[i]->callback(pipe, obj, link[i]->args, link[i]->offset);
    }
}

void RenderPipeline(GameObject *obj, VU1Pipeline *pipe)
{
    ExecutePipelineCBs(obj, pipe);
    u32 matCount = obj->vertexBuffer.matCount;
    LinkedList *mats = obj->vertexBuffer.materials;
    while(matCount)
    {
        Material *mat = (Material*)mats->data;
        UploadTextureDrawing(GetTextureByID(g_Manager.texManager, mat->materialId));
        matCount--;
        mats = mats->next;
    }
    CallCommand(pipe->q, true);
}

void SetActivePipelineByName(GameObject *obj, const char *name)
{
    VU1Pipeline *iter = obj->pipelines;
    int len = strnlen(name, MAX_CHAR_PIPELINE_NAME);
    while (strncmp(name, iter->name, len) != 0)
    {
        iter = iter->next;
        if (!iter)
        {
            ERRORLOG("pipeline not found");
            return;
        }
    }

    obj->activePipeline = iter;
}

VU1Pipeline *GetPipelineByName(const char *name, GameObject *obj)
{
    VU1Pipeline *iter = obj->pipelines;
    int len = strnlen(name, MAX_CHAR_PIPELINE_NAME);
    while (strncmp(name, iter->name, len) != 0)
    {
        iter = iter->next;
        if (!iter)
        {
            ERRORLOG("pipeline not found");
            return NULL;
        }
    }

    return iter;
}

void SetActivePipeline(GameObject *obj, VU1Pipeline *pipe)
{
    obj->activePipeline = pipe;
}

void DeletePipeline(VU1Pipeline *pipe)
{
    int size = pipe->numberCBS;
    for (int i = 0; i < size; i++)
    {
        if (pipe->cbs[i]->args)
            free(pipe->cbs[i]->args);
        free(pipe->cbs[i]);
    }
    size = pipe->renderPasses;
    for (int i = 0; i < size; i++)
    {
        free(pipe->passes[i]);
    }
    free(pipe->cbs);
    free(pipe->q);
    free(pipe);
}
void AddVU1Pipeline(GameObject *obj, VU1Pipeline *pipeline)
{
    VU1Pipeline *iter = obj->pipelines;

    if (!iter)
    {
        obj->pipelines = pipeline;
        return;
    }

    while (iter->next)
    {
        iter = iter->next;
    }

    iter->next = pipeline;
    return;
}

PipelineCallback *CreatePipelineCBNode(pipeline_callback cb, u32 offset, void *argument, u32 id)
{
    PipelineCallback *node = (PipelineCallback *)malloc(sizeof(PipelineCallback));
    node->args = argument;
    node->offset = offset;
    node->callback = cb;
    node->id = id;
    return node;
}

PipelineCallback *CreateVU1WriteCBNode( pipeline_callback cb, u32 pipeline_loc, 
                                        u32 count, 
                                        u32 splitloc, 
                                        u32 id)
{
    VU1HeaderArgs *args = (VU1HeaderArgs*)malloc(sizeof(VU1HeaderArgs));
    args->count = count;
    args->loc = splitloc;
    return CreatePipelineCBNode(cb, pipeline_loc, args, id);

}

VU1Pipeline *CreateVU1Pipeline(const char *name, int sizeOfCBS, u32 renderPasses)
{
    VU1Pipeline *node = (VU1Pipeline *)malloc(sizeof(VU1Pipeline));

    node->cbs = malloc(sizeof(PipelineCallback *) * sizeOfCBS);
    for (int i = 0; i < sizeOfCBS; i++)
    {
        node->cbs[i] = NULL;
    }

    node->passes = malloc(sizeof(VU1PipelineRenderPass *) * renderPasses);
    for (int i = 0; i < renderPasses; i++)
    {
        node->passes[i] = (VU1PipelineRenderPass *)malloc(sizeof(VU1PipelineRenderPass));
    }

    node->next = NULL;
    node->q = NULL;
    node->qwSize = 0;
    node->numberCBS = 0;
    node->callBackSize = sizeOfCBS;
    node->currentRenderPass = 0;
    node->renderPasses = renderPasses;

    memcpy(node->name, name, strnlen(name, MAX_CHAR_PIPELINE_NAME));

    return node;
}


VU1Pipeline* AddPipelineCallbackNode(VU1Pipeline *pipeline, PipelineCallback *node)
{
    pipeline->cbs[pipeline->numberCBS++] = node;
    return pipeline;
}
