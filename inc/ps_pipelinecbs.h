#ifndef PS_VU1CALLBACKS
#define PS_VU1CALLBACKS

#include "ps_global.h"
void SetupPerObjDrawRegisters(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);
void SetupPerObjDrawVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);
void SetupPerObjDrawWireframeVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);
void SetupPerObjDrawTessVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);
void SetupPerObjMVPMatrix(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);
void SetupPerObjLightBuffer(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);

void SetupPerMorphDrawVU1Header(VU1Pipeline *pipe, GameObject* obj, void *arg, qword_t *pipeline_loc);
void UpdateInterpolatorDrawVU1(VU1Pipeline *pipe, GameObject* obj, void *arg, qword_t *pipeline_loc);
qword_t * CreateMorphInterpolatorDMAUpload(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 _index, u32 _matCount);
qword_t *CreateMorphPipelineCallbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline);

void SetupStage2MATVU1(VU1Pipeline *pipe, GameObject *obj, void *mat, qword_t *pipeline_loc);
void SetupBlendingCXT(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc);

#endif