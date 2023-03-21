#ifndef PS_PIPELINE_H
#define PS_PIPELINE_H
#include "ps_global.h"

void CreateGraphicsPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2);
qword_t *CreateVU1Callbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerSize, u32 pCode, u32 dCode, ...);
void CreateVU1ProgramsList(qword_t *q, u32 pipeCode, u16 drawCode, u16 stg2PrgNum, u16 stg1PrgNum);
void create_pipeline_obj_wireframe_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize);
void create_pipeline_tess_grid_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize, TessGrid *grid);
void CreateEnvMapPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2, Texture *envMap, MATRIX envMatrix);
void CreateSpecularPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2);
void CreateClipperGraphicsPipeline(GameObject *obj, const char* name, u32 pipeCode, u16 drawCode, u16 stg1, u16 stg2);
#endif