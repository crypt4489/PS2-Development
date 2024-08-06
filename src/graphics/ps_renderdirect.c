#include "graphics/ps_renderdirect.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_fast_maths.h"
#include "math/ps_matrix.h"
#include "dma/ps_dma.h"
#include "pipelines/ps_vu1pipeline.h"
#include "gamemanager/ps_manager.h"
#include "math/ps_vector.h"
#include "pipelines/ps_pipelinecbs.h"
#include "graphics/ps_drawing.h"
#include "log/ps_log.h"

#include <string.h>
#include <stdlib.h>

extern VECTOR up;


void RenderRay(Ray *ray, Color color, float t)
{
    PollVU1DoneProcessing(&g_Manager);
    VECTOR v[2];

    VectorCopy(v[0], ray->origin);
    VectorScaleXYZ(v[1], ray->direction, t);
    VectorAddXYZ(v[1], ray->origin, v[1]);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCount(2, 1, true);
    DrawVectorFloat(v[0][0], v[0][1], v[0][2], 1.0f);
    DrawVectorFloat(v[1][0], v[1][1], v[1][2], 1.0f);
    StartVertexShader();
    EndCommand();
}

void RenderLine(Line *line, Color color)
{
    PollVU1DoneProcessing(&g_Manager);
    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCount(2, 1, true);
    DrawVector(line->p1);
    DrawVector(line->p2);
    StartVertexShader();
    EndCommand();
}

void RenderVertices(VECTOR *verts, u32 numVerts, Color color)
{
    PollVU1DoneProcessing(&g_Manager);
    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    DrawCount(numVerts, 1, true);
    for (int i = 0; i<numVerts; i++)
    {
        DrawVector(verts[i]);
    }
    StartVertexShader();
    EndCommand();
}

void RenderGameObject(GameObject *obj, Color *colors)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;

    MatrixIdentity(vp);

    MATRIX m;

    CreateWorldMatrixLTM(obj->ltm, m);

    MatrixMultiply(vp, vp, m);
    MatrixMultiply(vp, vp, g_DrawCamera->viewProj);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(vp, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(obj->renderState.color.r, obj->renderState.color.g, obj->renderState.color.b, obj->renderState.color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, 
                GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, 
                obj->renderState.prim.mapping, obj->renderState.prim.fogging, 
                obj->renderState.prim.blending, obj->renderState.prim.antialiasing, 
                obj->renderState.prim.mapping_type, g_Manager.gs_context, 
                obj->renderState.prim.colorfix), 0, obj->renderState.state.gs_reg_count), 
                obj->renderState.state.gs_reg_mask, 10);
    PushInteger(obj->renderState.state.render_state.state, 12, 3);

    u32 count = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount;
    DrawCount(count, 2, true);
    VECTOR *verts = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices;
    for (int i = 0; i<count; i++)
    {
        DrawVector(verts[i]);
    }

    for (int i = 0; i<count; i++)
    {
        DrawColor(colors[i]); 
    }

    StartVertexShader();

    EndCommand();
}

void RenderPlaneLine(Plane *plane, Color color, int size)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[4];
    VECTOR temp;

    CreateVector(1.0f * size, 0.0f, 1.0 * size, 1.0f, v[0]);
    CreateVector(1.0f * size, 0.0f, -1.0 * size, 1.0f, v[1]);
    CreateVector(-1.0f * size, 0.0f, 1.0 * size, 1.0f, v[2]);
    CreateVector(-1.0f * size, 0.0f, -1.0 * size, 1.0f, v[3]);

    CrossProduct(plane->planeEquation, up, temp);
    float angle = ASin(dist(temp));

    MATRIX m;

    CreateRotationMatrix(temp, angle, m);

    VectorCopy(&m[12], plane->pointInPlane);

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, m);
    MatrixMultiply(vp, vp, g_DrawCamera->viewProj);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(vp, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCount(10, 1, true);

    DrawVector(v[0]);
    DrawVector(v[1]);

    DrawVector(v[1]);
    DrawVector(v[3]);

    DrawVector(v[3]);
    DrawVector(v[2]);

    DrawVector(v[2]);
    DrawVector(v[0]);
    
    DrawVector(v[3]);
    DrawVector(v[0]);

    StartVertexShader();
    EndCommand();
}

void RenderSphereLine(BoundingSphere *sphere, Color color, int size)
{
    
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR center;
    VectorCopy(center, sphere->center);
    float r = sphere->radius;

    float step = TWOPI / size;
    float ang = 0;

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT,
     DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCount(size*2, 1, true);
    for (int i = 0; i < size - 1; i++)
    {
        DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
        ang += step;
        DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
    }
    DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
    DrawVectorFloat(r + center[0], center[1], center[2], 1.0f);
   

    StartVertexShader();
    EndCommand();
}

void RenderAABBBoxLine(BoundingBox *boxx, Color color, MATRIX world)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    VECTOR v[8];

    MatrixIdentity(vp);

    MatrixMultiply(vp, vp, world);

    MatrixMultiply(vp, vp, g_DrawCamera->viewProj);

    VectorCopy(v[0], boxx->top);
    VectorCopy(v[7], boxx->bottom);

    CreateVector(boxx->top[0], boxx->top[1], boxx->bottom[2], 1.0f, v[1]);
    CreateVector(boxx->top[0], boxx->bottom[1], boxx->bottom[2], 1.0f, v[2]);
    CreateVector(boxx->top[0], boxx->bottom[1], boxx->top[2], 1.0f, v[3]);

    CreateVector(boxx->bottom[0], boxx->top[1], boxx->bottom[2], 1.0f, v[4]);
    CreateVector(boxx->bottom[0], boxx->top[1], boxx->top[2], 1.0f, v[5]);
    CreateVector(boxx->bottom[0], boxx->bottom[1], boxx->top[2], 1.0f, v[6]);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(vp, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCount(24, 1, true);

    DrawVector(v[0]);
    DrawVector(v[1]);
    DrawVector(v[1]);
    DrawVector(v[2]);
    DrawVector(v[2]);
    DrawVector(v[3]);
    DrawVector(v[3]);
    DrawVector(v[0]);

    DrawVector(v[7]);
    DrawVector(v[4]);
    DrawVector(v[4]);
    DrawVector(v[5]);
    DrawVector(v[5]);
    DrawVector(v[6]);
    DrawVector(v[6]);
    DrawVector(v[7]);

    DrawVector(v[5]);
    DrawVector(v[0]);
    DrawVector(v[7]);
    DrawVector(v[2]);
    DrawVector(v[4]);
    DrawVector(v[1]);
    DrawVector(v[3]);
    DrawVector(v[6]);
    StartVertexShader();
    EndCommand();
}