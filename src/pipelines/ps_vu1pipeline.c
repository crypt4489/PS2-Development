#include "pipelines/ps_vu1pipeline.h"

#include <draw.h>


#include <string.h>
#include <stdlib.h>

#include "camera/ps_camera.h"
#include "dma/ps_dma.h"
#include "textures/ps_texture.h"
#include "log/ps_log.h"

#include "textures/ps_texturemanager.h"

u32 GetDoubleBufferOffset(u32 base)
{
    u32 half = (1024 - base) >> 1;
    return half;
}

void CreatePipelineSizes(u32 pCode, u32 *numberOfCbs, u32 *vu1_header_size)
{
    u32 size = 3;
    u32 header = 16;

    if ((pCode & VU1Stage1) != 0)
    {
        size += 1;
    }

    if ((pCode & VU1Stage2) != 0)
    {
        header += 4;
        size++;
    }

    if ((pCode & VU1Stage3) != 0)
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
   // DEBUGLOG("Here!");
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
            Material *mat = (Material *)q->sw[1];
            Texture *tex = GetTextureByID(g_Manager.texManager, mat->materialId);
            q++;
            if (tex)
            {
                UploadTextureViaManagerToVRAM(tex);
            }
            else
            {
                ERRORLOG("texture is null");
                while(1);
            }
        }

        else if (decode.code == DMA_DCODE_LOAD_ID_TEXTURE)
        {

            u64 id = q->dw[1];
            // INFOLOG("texhere! %d", id);
            Texture *tex = GetTextureByID(g_Manager.texManager, id);
            q++;
            if (tex)
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
             //printf("here!");
            int index = ((int)q->sw[1]) - 1;
            //ERRORLOG("%d", index);
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
           // DEBUGLOG("Here!!");
            SubmitToDMAController(q, channel, type, qwc, tte);
            q += qwc;
            pipe->currentRenderPass += 1;
        }
        else if (decode.code == DMA_DCODE_DRAW_FINISH)
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
            draw_wait_finish();
        }

        else
        {
            channel = decode.chann;
            qwc = decode.qwc;
            tte = decode.tte;
            type = decode.type;
            // q++;
            //    ERRORLOG("%x %x %x", q->sw[0], q->sw[1], q->sw[2]);
            //  dump_packet(pipe->q);
            // while(1);

            q++;

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

PipelineCallback *CreatePipelineCBNode(pipeline_callback cb, qword_t *pipeline_loc, void *argument, u32 id)
{
    PipelineCallback *node = (PipelineCallback *)malloc(sizeof(PipelineCallback));
    node->args = argument;
    node->q = pipeline_loc;
    node->callback = cb;
    node->id = id;
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

    node->passes = malloc(sizeof(VU1PipelineRenderPass *) * renderPasses);
    for (int i = 0; i < renderPasses; i++)
    {
        node->passes[i] = (VU1PipelineRenderPass *)malloc(sizeof(VU1PipelineRenderPass));
    }

    node->next = NULL;
    node->q = NULL;
    node->numberCBS = 0;
    node->callBackSize = sizeOfCBS;
    node->currentRenderPass = 0;
    node->renderPasses = renderPasses;

    memcpy(node->name, name, strnlen(name, MAX_CHAR_PIPELINE_NAME));

    return node;
}

qword_t *AddPipelineCallbackNodeQword(VU1Pipeline *pipeline, PipelineCallback *node, qword_t *q, qword_t *targ)
{
    int callNumber = 1;
    int size = pipeline->numberCBS;


    for (int i = 0; i <= size; i++)
    {
        if (!pipeline->cbs[i])
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

    qword_t *b = q;
    b->sw[0] = DMA_DCODE_CALLBACK_FUNC;
    b->sw[1] = callNumber;
    b->sw[2] = DMA_DCODE_CALLBACK_FUNC;
    b->sw[3] = DMA_DCODE_CALLBACK_FUNC;
    b++;
    return b;
}
