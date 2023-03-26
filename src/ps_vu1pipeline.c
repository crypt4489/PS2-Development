#include "ps_vu1pipeline.h"

#include <string.h>
#include <stdlib.h>

#include "ps_camera.h"
#include "ps_dma.h"
#include "ps_texture.h"
#include "ps_misc.h"
#include "ps_log.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

u32 GetDoubleBufferOffset(u32 base)
{
    u32 half = (1024 - base) / 2;
    return half + base;
}

void CreatePipelineSizes(u32 code, u32 *numberOfCbs, u32 *vu1_header_size)
{
    u32 size = 3;
    u32 header = 16;

    if ((code & VU1Stage1) != 0)
    {
        size+=1;
    }

    if ((code & VU1Stage2)  != 0)
    {
        header += 4;
        size++;
    }

    if ((code & VU1Stage3)  != 0)
    {
        header = 36;
        size++;
    }

    *numberOfCbs = size;
    *vu1_header_size = header;
}




void ParsePipeline(GameObject *obj, VU1Pipeline *pipe)
{
    qword_t *q = pipe->q;
    DMA_DCODE_STRUCT decode;
    int loop = 1;
    int channel, qwc, tte, type;
    while (loop)
    {
        decode.code = q->sw[0];
        if (decode.code == DMA_DCODE_END)
        {
            loop = 0;
            break;
        }
         else if (decode.code == DMA_DCODE_LOAD_MATERIAL)
        {
            Material *mat = (Material*)q->sw[1];
            u32 id = mat->materialId;
            //INFOLOG("texhere! %d", id);
            Texture *tex = GetTextureByID(id, g_Manager.texManager);
            q++;
            if (tex != NULL)
            {
                UploadTextureViaManagerToVRAM(tex);
            }
            else
            {
                ERRORLOG("texture is null");
            }
        }

         else if (decode.code == DMA_DCODE_LOAD_ID_TEXTURE)
        {

            u32 id = q->sw[1];
            //INFOLOG("texhere! %d", id);
            Texture *tex = GetTextureByID(id, g_Manager.texManager);
            q++;
            if (tex != NULL)
            {
                UploadTextureViaManagerToVRAM(tex);
            }
            else
            {
                ERRORLOG("texture is null");
            }
        }

        else if (decode.code == DMA_DCODE_CALLBACK_FUNC)
        {
            // printf("here!");
            int index = ((int)q->sw[1]) - 1;
            pipe->cbs[index]->callback(pipe, obj, pipe->cbs[index]->args, pipe->cbs[index]->q);
            q++;
        }
        else if (decode.code == DMA_DCODE_UPLOAD_MESH)
        {
            DMA_DCODE_STRUCT temp;
            temp.code = q->sw[1];
            channel = temp.chann;
            tte = temp.tte;
            type = temp.type;
            qwc = temp.qwc;
            q++;
            SubmitToDMAController(q, channel, type, qwc, tte);
            q += qwc;
            pipe->currentRenderPass+=1;
        }
        else
        {
            channel = decode.chann;
            qwc = decode.qwc;
            tte = decode.tte;
            type = decode.type;
            q++;
            //  ERRORLOG("%d %d %d", channel, qwc, tte);
            //  dump_packet(pipe->q);
            // while(1);

            SubmitToDMAController(q, channel, type, qwc, tte);
            q += qwc;
        }
    }

    pipe->currentRenderPass = 0;
}

void ExecutePipelineCBs(GameObject *obj, VU1Pipeline *pipe)
{
    PipelineCallback **link = pipe->cbs;

    // INFOLOG("%s", pipe->name);
    for (int i = 0; i < pipe->numberCBS; i++)
    {
        link[i]->callback(pipe, obj, link[i]->args, link[i]->q);
    }
}

void RenderPipeline(GameObject *obj, VU1Pipeline *active_pipe)
{
   // dump_packet(active_pipe->q);
    ParsePipeline(obj, active_pipe);
}

void SetActivePipelineByName(GameObject *obj, const char *name)
{
    VU1Pipeline *iter = obj->pipelines;
    int len = strlen(name);
    while (strncmp(name, iter->name, len) != 0)
    {
        iter = iter->next;
        if (iter == NULL)
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
    int len = strlen(name);
    while (strncmp(name, iter->name, len) != 0)
    {
        iter = iter->next;
        if (iter == NULL)
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
        free(pipe->cbs[i]);
    }
    free(pipe->cbs);
    free(pipe->q);
    free(pipe);
}
void AddVU1Pipeline(GameObject *obj, VU1Pipeline *pipeline)
{
    VU1Pipeline *iter = obj->pipelines;

    if (iter == NULL)
    {
        obj->pipelines = pipeline;
        return;
    }
    while (iter->next != NULL)
    {
        iter = iter->next;
    }

    iter->next = pipeline;
    return;
}

PipelineCallback *CreatePipelineCBNode(pipeline_callback cb, qword_t *pipeline_loc, void *argument)
{
    PipelineCallback *node = (PipelineCallback *)malloc(sizeof(PipelineCallback));
    node->args = argument;
    node->q = pipeline_loc;
    node->callback = cb;
    return node;
}

VU1Pipeline *CreateVU1Pipeline(const char *name, int sizeOfCBS, u32 renderPasses)
{
    VU1Pipeline *node = (VU1Pipeline *)malloc(sizeof(VU1Pipeline));
    node->cbs = malloc(sizeof(PipelineCallback *) * sizeOfCBS);
    for (int i = 0; i < sizeOfCBS; i++)
    {
        node->cbs[i] = NULL;
    }
    node->programs = malloc(sizeof(qword_t*) * renderPasses);
    for (int i = 0; i<renderPasses; i++)
    {
        node->programs[i] = (qword_t*)malloc(sizeof(qword_t));
    }
    node->next = NULL;
    node->q = NULL;
    node->numberCBS = 0;
    node->callBackSize = sizeOfCBS;
    node->currentRenderPass = 0;
    node->renderPasses = renderPasses;
    strncpy(node->name, name, MAX_CHAR_PIPELINE_NAME);
    return node;
}

qword_t *AddPipelineCallbackNodeQword(VU1Pipeline *pipeline, PipelineCallback *node, qword_t *q, qword_t *targ)
{
    int callNumber = 1;
    int size = pipeline->numberCBS;

    if (size == 0)
    {
        pipeline->numberCBS++;
        pipeline->cbs[0] = node;
    }
    else
    {
        for (int i = 0; i <= size; i++)
        {
            if (pipeline->cbs[i] == NULL)
            {
                pipeline->numberCBS++;
                pipeline->cbs[i] = node;
                break;
            }
            else if (pipeline->cbs[i]->callback == node->callback && pipeline->cbs[i]->q == targ)
            {
                break;
            }
            callNumber++;
        }
    }

    qword_t *b = q;
    b->sw[0] = DMA_DCODE_CALLBACK_FUNC;
    b->sw[1] = callNumber;
    b->sw[2] = DMA_DCODE_CALLBACK_FUNC;
    b->sw[3] = DMA_DCODE_CALLBACK_FUNC;
    b++;
    return b;
}

#pragma GCC diagnostic pop
