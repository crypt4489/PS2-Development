#include "pipelines/ps_pipelinecbs.h"

#include <stdlib.h>
#include <string.h>

#include "pipelines/ps_pipelineinternal.h"
#include "dma/ps_dma.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "pipelines/ps_vu1pipeline.h"
#include "world/ps_lights.h"
#include "physics/ps_obb.h"
#include "gs/ps_gs.h"
#include "math/ps_fast_maths.h"
#include "animation/ps_morphtarget.h"
#include "system/ps_vif.h"
#include "gameobject/ps_gameobject.h"
#include "math/ps_quat.h"
#include "camera/ps_camera.h"
#include "log/ps_log.h"
#include "animation/ps_animation.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

typedef struct interpolator_callback_data_t
{
  u32 currIndex;
  u32 matCount;
} InterpolatorCallbackData;

void SetupAlphaMapPass1(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *q = pipeline_loc;
  color_t color;
  CREATE_RGBAQ_STRUCT(color, 0, 0, 0, 0, 1.0f);
  q = SetupZTestGS(q, obj->renderState.state.render_state.Z_TYPE, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_ALLPASS, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);
  q = SetupRGBAQGS(q, color);
  q = SetFrameBufferMask(q, g_Manager.targetBack->render, 0x00ffffff, g_Manager.gs_context);
  q = SetZBufferMask(q, g_Manager.targetBack->z, 1, g_Manager.gs_context);
}

void SetupAlphaMapPass3(VU1Pipeline *pipe, GameObject *obj, void *mat, qword_t *pipeline_loc)
{
  qword_t *q = pipeline_loc;
  q = SetupZTestGS(q, 2, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 1, DTEST_METHOD_PASS_ONE, g_Manager.gs_context);
  q = SetFrameBufferMask(q, g_Manager.targetBack->render, 0xff000000, g_Manager.gs_context);

  q++;

  q->sw[3] = obj->renderState.state.render_state.state;
}

void SetupAlphaMapPass2(VU1Pipeline *pipe, GameObject *obj, void *mat, qword_t *pipeline_loc)
{
  qword_t *q = pipeline_loc;
  q = SetupRGBAQGS(q, obj->renderState.color);
  q = SetZBufferMask(q, g_Manager.targetBack->z, 0, g_Manager.gs_context);
  q = SetupZTestGS(q, 2, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);
}

void SetupAlphaMapFinish(VU1Pipeline *pipe, GameObject *obj, void *mat, qword_t *pipeline_loc)
{
  qword_t *q = pipeline_loc;
  q = SetFrameBufferMask(q, g_Manager.targetBack->render, 0x00000000, g_Manager.gs_context);
}

void SetupStage2MATVU1(VU1Pipeline *pipe, GameObject *obj, void *mat, qword_t *pipeline_loc)
{
  // printf("here!");
  qword_t *q = pipeline_loc + 16;

  if (mat == NULL)
  {
    ERRORLOG("MATRIX for Stage 2 is NULL");
    return;
  }

  memcpy(q, mat, sizeof(MATRIX));
}

void SetupMapTexture(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  Texture *tex = (Texture*)arg;
  CreateLoadByIdDCODETag(pipeline_loc, tex->id);
}

void SetupBlendingCXT(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  // printf("here!");
  qword_t *q = pipeline_loc;

  blend_t blendee;

  blendee.color1 = BLEND_COLOR_SOURCE;
  blendee.color2 = BLEND_COLOR_ZERO;
  blendee.color3 = BLEND_COLOR_DEST;
  blendee.alpha = BLEND_ALPHA_FIXED;
  blendee.fixed_alpha = 0x80;

  q = SetupZTestGS(q, 2, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);

  q = SetupAlphaGS(q, &blendee, g_Manager.gs_context);

  q++;

  q->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, obj->renderState.prim.mapping, obj->renderState.prim.fogging, 1, obj->renderState.prim.antialiasing, obj->renderState.prim.mapping_type, g_Manager.gs_context, obj->renderState.prim.colorfix), 0, obj->renderState.state.gs_reg_count);
  q->dw[1] = obj->renderState.state.gs_reg_mask;
}

void SetupPerBoneAnimationVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  u32 *headerLocation = (u32 *)arg;
  pipeline_loc += VU1_LOCATION_ANIMATION_VECTOR;
  qword_t *q = pipeline_loc;
  q->sw[0] = *headerLocation;
}

void SetupPerMorphDrawVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  u32 index = obj->interpolator->currInterpNode;
  pipeline_loc += VU1_LOCATION_ANIMATION_VECTOR;
  qword_t *q = pipeline_loc;
  ((float *)q->sw)[0] = obj->interpolator->interpolators[index]->position;
  ((float *)q->sw)[1] = (1.0f - obj->interpolator->interpolators[index]->position);
}

void UpdateInterpolatorDrawVU1(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  InterpolatorCallbackData *data = (InterpolatorCallbackData *)arg;

  if (data->currIndex != obj->interpolator->currInterpNode)
  {
    // DEBUGLOG("Curr Index %d %d %d", pipe->currentRenderPass, data->matCount, count);
    CreateMeshDMAUpload(pipeline_loc, obj, 27, DRAW_NORMAL | DRAW_TEXTURE | DRAW_MORPH | DRAW_VERTICES, data->matCount, &pipe->passes[pipe->currentRenderPass]->programs);
    data->currIndex = obj->interpolator->currInterpNode;
  }
}

qword_t *CreateMorphInterpolatorDMAUpload(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 _index, u32 _matCount)
{
  InterpolatorCallbackData *data = (InterpolatorCallbackData *)malloc(sizeof(InterpolatorCallbackData));
  if (data == NULL)
  {
    ERRORLOG("cannot create morph pipeline callbacks");
    return tag;
  }
  data->currIndex = _index;
  data->matCount = _matCount;

  PipelineCallback *setupInterpolatorNode = CreatePipelineCBNode(UpdateInterpolatorDrawVU1, q, data, MORPHTARGET_INTERPOLATOR_PCB);

  tag = AddPipelineCallbackNodeQword(pipeline, setupInterpolatorNode, tag, q);

  return tag;
}

qword_t *CreateMorphPipelineCallbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline)
{
  PipelineCallback *setupVU1Morph = CreatePipelineCBNode(SetupPerMorphDrawVU1Header, q, NULL, MORPHTARGET_VU1_HEADER_PCB);

  tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Morph, tag, q);

  return tag;
}

qword_t *CreateSkinnedAnimationCallbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerLocation)
{
  u32 *val = (u32 *)malloc(sizeof(u32));

  *val = headerLocation;

  PipelineCallback *setupVU1Bones = CreatePipelineCBNode(SetupPerBoneAnimationVU1Header, q, val, SKINNED_VU1_HEADER_PCB);

  tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Bones, tag, q);

  return tag;
}

void UpdateBoneVectorsDrawVU1(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{

  Animator *anim = obj->objAnimator;
  UpdateVU1BoneMatrices(pipeline_loc, anim, obj->vertexBuffer.meshAnimationData->joints, obj->vertexBuffer.meshAnimationData->jointsCount);
}

qword_t *CreateBonesVectorsDMAUpload(qword_t *tag, qword_t *q, VU1Pipeline *pipeline)
{
  PipelineCallback *setupVU1BonesDMA = CreatePipelineCBNode(UpdateBoneVectorsDrawVU1, q, NULL, SKINNED_DMA_UPLOAD_PCB);

  tag = AddPipelineCallbackNodeQword(pipeline, setupVU1BonesDMA, tag, q);

  return tag;
}

void SetupPerObjDrawTessVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *pipeline_temp = pipeline_loc; // obj->wvp_matrix;//

  pipeline_temp += 4;

  TessGrid *grid = (TessGrid *)arg;

  float lengthX = Abs(grid->extent.top[0] - grid->extent.bottom[0]);

  float lengthY = Abs(grid->extent.top[2] - grid->extent.bottom[2]);

  float stepVertX = lengthX / (float)grid->xDim;

  float stepVertY = lengthY / (float)grid->yDim;

  VECTOR firstSet;
  firstSet[0] = stepVertX;
  firstSet[1] = stepVertY;

  pipeline_temp = VectorToQWord(pipeline_temp, firstSet);

  // pack dim

  pipeline_temp->sw[0] = grid->xDim + 1;
  pipeline_temp->sw[1] = grid->yDim;
  pipeline_temp->sw[2] = 0;
  pipeline_temp->sw[3] = 0;
  pipeline_temp++;

  VECTOR extent;
  VectorCopy(extent, grid->extent.top);
  extent[3] = 0.0f;

  pipeline_temp = VectorToQWord(pipeline_temp, extent);

  //  printf("here in tess grid callback");
}

void SetupPerObjDrawRegisters(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *q = pipeline_loc;

  q = SetupZTestGS(q, obj->renderState.state.render_state.Z_TYPE, obj->renderState.state.render_state.Z_ENABLE, 0x00, ATEST_METHOD_NOTEQUAL, ATEST_KEEP_FRAMEBUFFER, 0, 0, g_Manager.gs_context);
  q = SetupRGBAQGS(q, obj->renderState.color);
}

void SetupPerObjDrawWireframeVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *pipeline_temp = pipeline_loc;

  pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

  pipeline_temp = VIFSetupScaleVector(pipeline_temp);

  pipeline_temp->sw[0] = (int)255;
  pipeline_temp->sw[1] = (int)0;
  pipeline_temp->sw[2] = (int)0;
  pipeline_temp->sw[3] = (int)128;
  pipeline_temp++;

  pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, obj->renderState.prim.shading, DRAW_DISABLE, obj->renderState.prim.fogging, obj->renderState.prim.blending, obj->renderState.prim.antialiasing, obj->renderState.prim.mapping_type, g_Manager.gs_context, obj->renderState.prim.colorfix), 0, 2);
  pipeline_temp->dw[1] = DRAW_RGBAQ_REGLIST;
  pipeline_temp += 2;
  pipeline_temp->sw[3] = obj->renderState.state.render_state.state;
}
void SetupPerObjDrawVU1HeaderAlphaMap(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *pipeline_temp = pipeline_loc;

  pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

  pipeline_temp = VIFSetupScaleVector(pipeline_temp);

  pipeline_temp->sw[0] = (int)obj->renderState.color.r;
  pipeline_temp->sw[1] = (int)obj->renderState.color.g;
  pipeline_temp->sw[2] = (int)obj->renderState.color.b;
  pipeline_temp->sw[3] = (int)obj->renderState.color.a;
  pipeline_temp++;

  pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, obj->renderState.prim.mapping, obj->renderState.prim.fogging, obj->renderState.prim.blending, obj->renderState.prim.antialiasing, obj->renderState.prim.mapping_type, g_Manager.gs_context, obj->renderState.prim.colorfix), 0, obj->renderState.state.gs_reg_count);
  pipeline_temp->dw[1] = obj->renderState.state.gs_reg_mask;
  pipeline_temp += 2;
  pipeline_temp->sw[3] = (obj->renderState.state.render_state.state & 0xffffff7f);
}

void SetupPerObjDrawVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  qword_t *pipeline_temp = pipeline_loc;

  pipeline_temp += VU1_LOCATION_SCALE_VECTOR;

  pipeline_temp = VIFSetupScaleVector(pipeline_temp);

  pipeline_temp->sw[0] = (int)obj->renderState.color.r;
  pipeline_temp->sw[1] = (int)obj->renderState.color.g;
  pipeline_temp->sw[2] = (int)obj->renderState.color.b;
  pipeline_temp->sw[3] = (int)obj->renderState.color.a;
  pipeline_temp++;

  pipeline_temp->dw[0] = GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, obj->renderState.prim.mapping, obj->renderState.prim.fogging, obj->renderState.prim.blending, obj->renderState.prim.antialiasing, obj->renderState.prim.mapping_type, g_Manager.gs_context, obj->renderState.prim.colorfix), 0, obj->renderState.state.gs_reg_count);
  pipeline_temp->dw[1] = obj->renderState.state.gs_reg_mask;
  pipeline_temp += 2;
  pipeline_temp->sw[3] = obj->renderState.state.render_state.state;
}

void SetupPerObjMVPMatrix(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  MATRIX screen, m;
  CreateWorldMatrixLTM(obj->ltm, m);

  MatrixIdentity(screen);
  MatrixMultiply(screen, screen, m);

  VECTOR camProps;

  Camera *cam = NULL;
  if (g_DrawWorld != NULL)
  {
    cam = g_DrawWorld->cam;
  }
  else
  {
    cam = g_DrawCamera;
  }

  if (cam == NULL)
  {
    ERRORLOG("something went wrong with camera");
  }

  qword_t *pipeline_temp = pipeline_loc;

  memcpy(pipeline_temp + 4, m, 4 * sizeof(qword_t));

  MatrixMultiply(screen, screen, cam->view);
  MatrixMultiply(screen, screen, cam->proj);
  memcpy(pipeline_temp, screen, 4 * sizeof(qword_t));

  camProps[0] = cam->near;
  camProps[1] = cam->frus->nwidth;
  camProps[2] = cam->frus->nheight;
  memcpy(pipeline_temp + 13, camProps, sizeof(float) * 4);
  if (GetDirtyLTM(cam->ltm))
  {
    VECTOR quat;
    CreateCameraQuat(cam, quat);
    memcpy(pipeline_temp + 14, quat, sizeof(float) * 4);
    memcpy(pipeline_temp + 15, GetPositionVectorLTM(cam->ltm), sizeof(float) * 3);
  }
}

void SetupPerObjLightBuffer(VU1Pipeline *pipe, GameObject *obj, void *arg, qword_t *pipeline_loc)
{
  MATRIX m;
  if (GetDirtyLTM(obj->ltm))
  {
    CreateWorldMatrixLTM(obj->ltm, m);
    memcpy(pipeline_loc + VU1_LOCATION_GLOBAL_MATRIX, m, sizeof(MATRIX));
  }

  qword_t *q = pipeline_loc + VU1_LOCATION_LIGHTS_BUFFER;

  if (g_DrawWorld != NULL)
  {
    int count = g_DrawWorld->lightCount;
    q = InitVU1LightHeader(q, count);
    LinkedList *node = g_DrawWorld->lights;
    for (int i = 0; i < count; i++)
    {
      LightStruct *light = (LightStruct *)node->data;
      q = PackLightIntoQWord(q, light);
      node = node->next;
    }
  }
}