#include "pipelines/ps_pipelinecbs.h"

#include <stdlib.h>
#include <string.h>
#include <draw2d.h>
#include <draw3d.h>
#include <draw_tests.h>

#include "util/ps_linkedlist.h"
#include "pipelines/ps_pipelineinternal.h"
#include "dma/ps_dma.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "pipelines/ps_vu1pipeline.h"
#include "world/ps_lights.h"
#include "physics/ps_vbo.h"
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
#include "graphics/ps_drawing.h"

typedef struct interpolator_callback_data_t
{
  u32 currIndex;
  u32 matCount;
} InterpolatorCallbackData;

void SetupAlphaMapPass1(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  // draw the object and only affect the alpha channel so it is all zero.
  // don't affect zbuffer or framebuffer rgb
  qword_t *q = pipe->q + pipeline_loc;
  Color color;
  CREATE_RGBAQ_STRUCT(color, 0, 0, 0, 0, 1.0f);
  InitializeDMATag(q, true);
  DepthTest(true, 1);
  SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0x00);
  PrimitiveColor(color);
  FrameBufferMaskWord(0x00ffffff);
  DepthBufferMask(true);
  ResetDMAState();
}

void SetupAlphaMapPass3(VU1Pipeline *pipe, GameObject *obj, void *mat, u32 pipeline_loc)
{
  // draw object normally, not affecting alpha channel of framebuffer
  qword_t *q = pipe->q + pipeline_loc;
  InitializeDMATag(q, true);
  DepthTest(true, 2);
  SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0x00);
  DestinationAlphaTest(true, DTEST_METHOD_PASS_ONE);
  FrameBufferMaskWord(0x00ffffff);
  DepthBufferMask(false);
  ResetDMAState();
  InitializeVIFHeaderUpload(pipe->q + pipeline_loc + 6, NULL, 1);
  PushInteger(obj->renderState.properties.props, 12, 3);
  ResetVIFDrawingState();
}

void SetupAlphaMapPass2(VU1Pipeline *pipe, GameObject *obj, void *mat, u32 pipeline_loc)
{
  // draw object with normal rgbaq and update alpha to one
  // where the texture alpha is one
  qword_t *q = pipe->q + pipeline_loc;
  InitializeDMATag(q, true);
  DepthTest(true, 1);
  SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0x00);
  PrimitiveColor(obj->renderState.color);
  ResetDMAState();
  
}

void SetupAlphaMapFinish(VU1Pipeline *pipe, GameObject *obj, void *mat, u32 pipeline_loc)
{
  qword_t *q = pipe->q + pipeline_loc;
  InitializeDMATag(q, true);
  FrameBufferMaskWord(0x0);
  ResetDMAState();
}

void SetupStage2MATVU1(VU1Pipeline *pipe, GameObject *obj, void *mat, u32 pipeline_loc)
{
  // printf("here!");
 // qword_t *q = pipeline_loc + 16;

  if (!mat)
  {
    ERRORLOG("MATRIX for Stage 2 is NULL");
    return;
  }

  //memcpy(q, mat, sizeof(MATRIX));
}

void SetupMapTexture(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  Texture *tex = (Texture*)arg;
 // CreateLoadByIdDCODETag(pipeline_loc, tex->id);
}

void SetupBlendingCXT(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  qword_t *q = pipe->q + pipeline_loc;

  blend_t blendee;

  blendee.color1 = BLEND_COLOR_SOURCE;
  blendee.color2 = BLEND_COLOR_ZERO;
  blendee.color3 = BLEND_COLOR_DEST;
  blendee.alpha = BLEND_ALPHA_FIXED;
  blendee.fixed_alpha = 0x80;

  InitializeDMATag(q, true);
  DepthTest(true, 2);
  SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0);
  BlendingEquation(&blendee);
  ResetDMAState();
  InitializeVIFHeaderUpload(q + 5, NULL, 1);
  PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(obj->renderState.gsstate.prim.type, 
              obj->renderState.gsstate.prim.shading, obj->renderState.gsstate.prim.mapping, 
              obj->renderState.gsstate.prim.fogging, 1, obj->renderState.gsstate.prim.antialiasing, 
              obj->renderState.gsstate.prim.mapping_type, g_Manager.gs_context, 
              obj->renderState.gsstate.prim.colorfix), 0, 
              obj->renderState.gsstate.gs_reg_count), obj->renderState.gsstate.gs_reg_mask, 10);
  ResetVIFDrawingState();
}

void SetupPerBoneAnimationVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  u32 *headerLocation = (u32 *)arg;
  pipeline_loc += VU1_LOCATION_ANIMATION_VECTOR;
  qword_t *q = pipe->q + pipeline_loc;
  q->sw[0] = *headerLocation;
}

void SetupPerMorphDrawVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  u32 index = obj->interpolator->currInterpNode;
  pipeline_loc += VU1_LOCATION_ANIMATION_VECTOR;
  qword_t *q = pipe->q + pipeline_loc;
  ((float *)q->sw)[0] = obj->interpolator->interpolators[index]->position;
  ((float *)q->sw)[1] = (1.0f - obj->interpolator->interpolators[index]->position);
}

void UpdateInterpolatorDrawVU1(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  InterpolatorCallbackData *data = (InterpolatorCallbackData *)arg;

  if (data->currIndex != obj->interpolator->currInterpNode)
  {
    // DEBUGLOG("Curr Index %d %d %d", pipe->currentRenderPass, data->matCount, count);
  //  CreateMeshDMAUpload(pipeline_loc, obj, 27, DRAW_NORMAL | DRAW_TEXTURE | DRAW_MORPH | DRAW_VERTICES, data->matCount, &pipe->passes[pipe->currentRenderPass]->programs);
    data->currIndex = obj->interpolator->currInterpNode;
  }
}

qword_t *CreateMorphInterpolatorDMAUpload(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 _index, u32 _matCount)
{
  InterpolatorCallbackData *data = (InterpolatorCallbackData *)malloc(sizeof(InterpolatorCallbackData));
  if (!data)
  {
    ERRORLOG("cannot create morph pipeline callbacks");
    return tag;
  }
  data->currIndex = _index;
  data->matCount = _matCount;

  //PipelineCallback *setupInterpolatorNode = CreatePipelineCBNode(UpdateInterpolatorDrawVU1, q, data, MORPHTARGET_INTERPOLATOR_PCB);

 // tag = AddPipelineCallbackNodeQword(pipeline, setupInterpolatorNode, tag, q);

  return tag;
}

qword_t *CreateMorphPipelineCallbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline)
{
  //PipelineCallback *setupVU1Morph = CreatePipelineCBNode(SetupPerMorphDrawVU1Header, q, NULL, MORPHTARGET_VU1_HEADER_PCB);

 // tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Morph, tag, q);

  return tag;
}

qword_t *CreateSkinnedAnimationCallbacks(qword_t *tag, qword_t *q, VU1Pipeline *pipeline, u32 headerLocation)
{
  u32 *val = (u32 *)malloc(sizeof(u32));

  *val = headerLocation;

 // PipelineCallback *setupVU1Bones = CreatePipelineCBNode(SetupPerBoneAnimationVU1Header, q, val, SKINNED_VU1_HEADER_PCB);

 // tag = AddPipelineCallbackNodeQword(pipeline, setupVU1Bones, tag, q);

  return tag;
}

void UpdateBoneVectorsDrawVU1(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{

}

qword_t *CreateBonesVectorsDMAUpload(qword_t *tag, qword_t *q, VU1Pipeline *pipeline)
{
 // PipelineCallback *setupVU1BonesDMA = CreatePipelineCBNode(UpdateBoneVectorsDrawVU1, q, NULL, SKINNED_DMA_UPLOAD_PCB);

  //tag = AddPipelineCallbackNodeQword(pipeline, setupVU1BonesDMA, tag, q);

  return tag;
}

void SetupPerObjDrawRegisters(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  qword_t *q = pipe->q + pipeline_loc;
  InitializeDMATag(q, true);
  DepthTest(obj->renderState.properties.Z_ENABLE, obj->renderState.properties.Z_TYPE);
  SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0x80);
  ResetDMAState();
}

void SetupPerObjDrawVU1HeaderAlphaMap(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  qword_t *pipeline_temp = pipe->q + pipeline_loc;
  VU1HeaderArgs *args = (VU1HeaderArgs*)arg;

  MATRIX screen, m;
  CreateWorldMatrixLTM(obj->ltm, m);

  MatrixIdentity(screen);
  MatrixMultiply(screen, screen, m);

  Camera *cam = NULL;
  if (g_DrawWorld)
  {
    cam = g_DrawWorld->cam;
  }
  else
  {
    cam = g_DrawCamera;
  }

  MatrixMultiply(screen, screen, cam->viewProj);

  InitializeVIFHeaderUpload(pipeline_temp, pipe->q + args->loc, args->count);

  PushMatrix(screen, 0, sizeof(MATRIX));
  PushMatrix(m, 4, sizeof(MATRIX));
  PushScaleVector();
  PushColor(obj->renderState.color.r, obj->renderState.color.g, obj->renderState.color.b, obj->renderState.color.a, 9);
  PushPairU64(GIF_SET_TAG(0, 1, 1, 
                GS_SET_PRIM(obj->renderState.gsstate.prim.type, obj->renderState.gsstate.prim.shading, 
                obj->renderState.gsstate.prim.mapping, obj->renderState.gsstate.prim.fogging, 
                obj->renderState.gsstate.prim.blending, obj->renderState.gsstate.prim.antialiasing, 
                obj->renderState.gsstate.prim.mapping_type, g_Manager.gs_context, 
                obj->renderState.gsstate.prim.colorfix), 0, obj->renderState.gsstate.gs_reg_count), 
                obj->renderState.gsstate.gs_reg_mask, 10);
  PushInteger((obj->renderState.properties.props & 0xffffff7f), 12, 3);
  VECTOR camProps;
  camProps[0] = cam->near;
  camProps[1] = cam->frus[0]->nwidth;
  camProps[2] = cam->frus[0]->nheight;
  PushFloats(camProps, 13, sizeof(float) * 3);
  if (GetDirtyLTM(cam->ltm))
  {
    PushMatrix(cam->quat, 14, sizeof(VECTOR));
    PushFloats(*GetPositionVectorLTM(cam->ltm), 15, sizeof(float) * 3);
  }
  ResetVIFDrawingState();
}

void SetupPerObjDrawVU1Header(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  qword_t *pipeline_temp = pipe->q + pipeline_loc;
  VU1HeaderArgs *args = (VU1HeaderArgs*)arg;


  Camera *cam = NULL;
  if (g_DrawWorld)
  {
    cam = g_DrawWorld->cam;
  }
  else
  {
    cam = g_DrawCamera;
  }
 

  InitializeVIFHeaderUpload(pipeline_temp, pipe->q + args->loc, args->count);


  PushScaleVector();
  PushColor(obj->renderState.color.r, obj->renderState.color.g, obj->renderState.color.b, obj->renderState.color.a, 9);
  PushPairU64(GIF_SET_TAG(0, 1, 1, 
                GS_SET_PRIM(obj->renderState.gsstate.prim.type, obj->renderState.gsstate.prim.shading, 
                obj->renderState.gsstate.prim.mapping, obj->renderState.gsstate.prim.fogging, 
                obj->renderState.gsstate.prim.blending, obj->renderState.gsstate.prim.antialiasing, 
                obj->renderState.gsstate.prim.mapping_type, g_Manager.gs_context, 
                obj->renderState.gsstate.prim.colorfix), 0, obj->renderState.gsstate.gs_reg_count), 
                obj->renderState.gsstate.gs_reg_mask, 10);
  PushInteger(obj->renderState.properties.props, 12, 3);
  VECTOR camProps;
  camProps[0] = cam->near;
  camProps[1] = cam->frus[0]->nwidth;
  camProps[2] = cam->frus[0]->nheight;
  PushFloats(camProps, 13, sizeof(float) * 3);
  if (GetDirtyLTM(cam->ltm))
  {
    
    PushMatrix(cam->quat, 14, sizeof(VECTOR));
    PushFloats(*GetPositionVectorLTM(cam->ltm), 15, sizeof(float) * 3);
  }
  ResetVIFDrawingState();
}

void SetupPerObjLightBuffer(VU1Pipeline *pipe, GameObject *obj, void *arg, u32 pipeline_loc)
{
  qword_t *q = pipe->q + pipeline_loc + VU1_LOCATION_LIGHTS_BUFFER;

  if (g_DrawWorld)
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
