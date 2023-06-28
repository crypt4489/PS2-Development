#ifndef PS_PIPELINE_H
#define PS_PIPELINE_H
#include "ps_global.h"

void CreateGraphicsPipeline(GameObject *obj, const char *name);
qword_t *CreateVU1Callbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerSize, u32 pCode, u32 dCode, ...);
void CreateVU1ProgramsList(qword_t *q, u32 pipeCode, u16 drawCode);
void create_pipeline_obj_wireframe_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize);
void create_pipeline_tess_grid_vu1pipeline(GameObject *obj, u32 programNumber, u32 qwSize, TessGrid *grid);
void CreateEnvMapPipeline(GameObject *obj, const char *name, u32 pipeCode, u16 drawCode, Texture *envMap, MATRIX envMatrix);
void CreateSpecularPipeline(GameObject *obj, const char *name);
void CreateClipperGraphicsPipeline(GameObject *obj, const char* name);
void CreateAlphaMapPipeline(GameObject *obj, const char *name, Texture *alphaMap);
#endif
