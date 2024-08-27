#ifndef PS_VU1PIPELINE_H
#define PS_VU1PIPELINE_H

#include "ps_global.h"

typedef struct vu1_header_args
{
    u32 count;
    u32 loc;
} VU1HeaderArgs;

u32 GetDoubleBufferOffset(u32 base);

VU1Pipeline* GetPipelineByName(const char *name, GameObject *obj);


PipelineCallback *CreatePipelineCBNode(pipeline_callback cb, u32 offset, void *argument, u32 id);
VU1Pipeline *CreateVU1Pipeline(const char *name, int sizeOfCBS, u32 renderPasses);
VU1Pipeline* AddPipelineCallbackNode(VU1Pipeline *pipeline, PipelineCallback *node);
void AddVU1Pipeline(GameObject *obj, VU1Pipeline *pipeline);

void SetActivePipelineByName(GameObject *obj, const char *name);
void SetActivePipeline(GameObject *obj, VU1Pipeline *pipe);
void ExecutePipelineCBs(GameObject *obj, VU1Pipeline *pipe);
void DeletePipeline(VU1Pipeline *pipe);
void RenderPipeline(GameObject *obj, VU1Pipeline *pipe);
PipelineCallback *CreateVU1WriteCBNode( pipeline_callback cb, u32 pipeline_loc, 
                                        u32 count, 
                                        u32 splitloc, 
                                        u32 id);
#endif