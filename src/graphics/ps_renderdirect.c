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
#include "gameobject/ps_gameobject.h"
#include "textures/ps_texturemanager.h"
#include "animation/ps_animation.h"
#include "system/ps_vumanager.h"
#include "camera/ps_camera.h"
#include <string.h>
#include <stdlib.h>

extern VECTOR up;

void DetermineVU1Programs(ObjectProperties *state, qword_t *programs)
{
    
    bool stage1 = state->SKELETAL_ANIMATION || state->MORPH_TARGET;
    bool stage2 = state->ANIMATION_TEXUTRE || state->ENVIRONMENTMAP;
    bool stage3 = state->LIGHTING_ENABLE || state->SPECULAR;

    u32 addr2 = 0, addr3 = 0, addr4 = 0;
    

    if (stage3)
    {
        if (state->SPECULAR)
        {
            addr4 = VU1GenericSpecular;
        }
        else
        {
            addr4 = VU1GenericLight3D;
        }
    }

    if (stage2)
    {
        addr3 = addr4;
        addr4 = (state->ENVIRONMENTMAP * VU1GenericEnvMap) | (state->ANIMATION_TEXUTRE * VU1GenericAnimTex);
    }

    if (stage1)
    {
        addr2 = addr4;
        addr4 = (state->MORPH_TARGET * VU1GenericMorph) | (state->SKELETAL_ANIMATION * VU1GenericSkinned);
    }

    if (state->CLIPPING)
    {
        programs->sw[stage1] = VU1GenericClipper;
        programs->sw[(!stage1)&1] = addr4;
    } else {
        programs->sw[0] = addr4;
    }
    
    programs->sw[2] = addr2;
    programs->sw[3] = addr3;
}


void RenderRay(Ray *ray, Color color, float t)
{
    MATRIX m;
    MatrixIdentity(m);
    VECTOR v[2];

    VectorCopy(v[0], ray->origin);
    VectorScaleXYZ(v[1], ray->direction, t);
    VectorAddXYZ(v[1], ray->origin, v[1]);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCountWrite(2, 1);
    DrawVectorFloat(v[0][0], v[0][1], v[0][2], 1.0f);
    DrawVectorFloat(v[1][0], v[1][1], v[1][2], 1.0f);
    StartVertexShader();
    SubmitCommand(false);
}

void RenderLine(Line *line, Color color)
{
    MATRIX m;
    MatrixIdentity(m);
    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCountWrite(2, 1);
    DrawVector(line->p1);
    DrawVector(line->p2);
    StartVertexShader();
    SubmitCommand(false);
}

void RenderVertices(VECTOR *verts, u32 numVerts, Color color)
{
    MATRIX m;
    MatrixIdentity(m);
    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    DrawCountWrite(numVerts, 1);
    for (int i = 0; i<numVerts; i++)
    {
        DrawVector(verts[i]);
    }
    StartVertexShader();
    SubmitCommand(false);
}

int DrawHeaderSize(GameObject *obj, int *baseHeader)
{
    int header = 16;
    *baseHeader = 16;
    if (obj->renderState.properties.ANIMATION_TEXUTRE || obj->renderState.properties.ENVIRONMENTMAP) { header = 20; }
    if (obj->renderState.properties.LIGHTING_ENABLE) { header = 36; *baseHeader = 36; }
    if (obj->renderState.properties.SKELETAL_ANIMATION) header += (obj->vertexBuffer.meshAnimationData->jointsCount * 3);
    return header;
}

int MaxUploadSize(VertexType type, u32 headerEnd, u32 regCount, bool clipping)
{
    int size = GetDoubleBufferOffset(headerEnd);
    u32 uploadCount = 1; 
    uploadCount += ((type & V_TEXTURE) != 0) * 1;
    uploadCount += ((type & V_NORMAL) != 0) * 1;
    uploadCount += ((type & V_COLOR) != 0) * 1;
    uploadCount += ((type & V_SKINNED) != 0) * 2;
    u32 precnt = size / (uploadCount + regCount);
    precnt -= (precnt/2) * clipping;
    return precnt - (precnt%3);
}

LinkedList *LoadMaterial(LinkedList *list, bool immediate, u32 *start, u32 *end)
{
    Material *mat = (Material*)list->data;
    *start =  mat->start; 
    *end = mat->end;
    Texture *tex = GetTextureByID(g_Manager.texManager, mat->materialId);
    BindTexture(tex, immediate);
    return list->next;
}

void RenderGameObject(GameObject *obj)
{
    u32 matCount = obj->vertexBuffer.matCount;
    MeshVectors *buffer = obj->vertexBuffer.meshData[MESHTRIANGLES];
    u32 count = buffer->vertexCount;
    VertexType type = GetVertexType(&obj->renderState.properties);
    u32 start = 0, end = count-1;
    LinkedList *matIter = obj->vertexBuffer.materials;


    qword_t programs;

    DetermineVU1Programs(&obj->renderState.properties, &programs);

    int base = 0;
    
    int headerSize = DrawHeaderSize(obj, &base);


   

    BeginCommand();
    if (matCount) { 
        matIter = LoadMaterial(matIter, true, &start, &end);
    }
    ShaderHeaderLocation(headerSize);
    for(int i = 0; i<4; i++)
        ShaderProgram(programs.sw[i], i);
    DepthTest(obj->renderState.properties.Z_ENABLE, obj->renderState.properties.Z_TYPE);;
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    AllocateShaderSpace(base, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(obj->world, 4, sizeof(MATRIX));
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

    Camera *cam = NULL;
    if (g_DrawWorld)
    {
        cam = g_DrawWorld->cam;
    }
    else
    {
     cam = g_DrawCamera;
    }
    VECTOR camProps;
    camProps[0] = cam->near;
    camProps[1] = cam->frus[0]->nwidth;
    camProps[2] = cam->frus[0]->nheight;
    PushFloats(camProps, 13, sizeof(float) * 3);
    PushMatrix(cam->quat, 14, sizeof(VECTOR));
    PushFloats(*GetPositionVectorLTM(cam->ltm), 15, sizeof(float) * 3);
  
    if (V_SKINNED & type)
    {
        PushInteger(base, 11, 0);
        UpdateVU1BoneMatrices(obj->vertexBuffer.meshAnimationData->finalBones, 
        obj->vertexBuffer.meshAnimationData->root,
        obj->objAnimator, 
        obj->vertexBuffer.meshAnimationData->joints, 
        obj->vertexBuffer.meshAnimationData->jointsCount);
        BindVectors(obj->vertexBuffer.meshAnimationData->finalBones, obj->vertexBuffer.meshAnimationData->jointsCount * 3, 0, base);
    }

    int max = MaxUploadSize(type, headerSize, obj->renderState.gsstate.gs_reg_count, obj->renderState.properties.CLIPPING);

    UploadBuffers(start, end, max, buffer, type);
    
    for (int i = 1; i<matCount; i++)
    {
        matIter = LoadMaterial(matIter, true, &start, &end);
        UploadBuffers(start, end, max, buffer, type);
    } 
    
    
    SubmitCommand(false);
}

void UploadBuffers(u32 start, u32 end, u32 maxCount, MeshVectors *buffer, VertexType type)
{
    
    u32 total = (end - start) + 1;
    u32 iterations = total / maxCount;
    if (total % maxCount) iterations++;

    for (int i = 0; i<iterations; i++, total-=maxCount)
    {
        u32 top = 1;

        if (maxCount > total)
            maxCount = total;

        DrawUpload(maxCount);
        BindVectors(&(buffer->vertices[start]), maxCount, true, top);
        top += maxCount;

        if (type & V_TEXTURE)
        {
            BindVectors(&(buffer->texCoords[start]), maxCount, true, top);
            top += maxCount;
        }

        if (type & V_NORMAL)
        {
            BindVectors(&(buffer->normals[start]), maxCount, true, top);
            top += maxCount;
        }

        if (type & V_COLOR)
        {
            BindVectors(&(buffer->colors[start]), maxCount, true, top);
            top += maxCount;
        }

        if (type & V_SKINNED)
        {
            BindVectorInts(&(buffer->bones[start]), maxCount, true, top);
            top += maxCount;
            BindVectors(&(buffer->weights[start]), maxCount, true, top);
            top += maxCount;
        }
        start += maxCount;
        StartVertexShader();
    }
}

void RenderPlaneLine(Plane *plane, Color color, int size)
{
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


    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCountWrite(10, 1);

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
    SubmitCommand(false);
}

void RenderSphereLine(BoundingSphere *sphere, Color color, int size)
{
    MATRIX m;
    MatrixIdentity(m);
    VECTOR center;
    VectorCopy(center, sphere->center);
    float r = sphere->radius;

    float step = TWOPI / size;
    float ang = 0;

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT,
     DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCountWrite(size*2, 1);
    for (int i = 0; i < size - 1; i++)
    {
        DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
        ang += step;
        DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
    }
    DrawVectorFloat(r * Cos(ang) + center[0], r * Sin(ang) + center[1], center[2], 1.0f);
    DrawVectorFloat(r + center[0], center[1], center[2], 1.0f);
   

    StartVertexShader();
    SubmitCommand(false);
}

void RenderAABBBoxLine(BoundingBox *boxx, Color color, MATRIX world)
{
   
    VECTOR v[8];


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
    ShaderProgram(0, 0);
    DepthTest(1, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_NOTEQUAL, 0xFF);
    AllocateShaderSpace(16, 0);
     PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(world, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(color.r, color.g, color.b, color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_LINE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushInteger(0, 12, 3);
    DrawCountWrite(24, 1);

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
    SubmitCommand(false);
}