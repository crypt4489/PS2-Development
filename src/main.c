#include "ps_global.h"

#include <graph.h>
#include <audsrv.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "math/ps_plane.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_misc.h"
#include "system/ps_timer.h"
#include "textures/ps_texture.h"
#include "pad/ps_pad.h"
#include "io/ps_texture_io.h"
#include "system/ps_vumanager.h"
#include "textures/ps_font.h"
#include "gamemanager/ps_manager.h"
#include "camera/ps_camera.h"
#include "io/ps_file_io.h"
#include "gameobject/ps_gameobject.h"
#include "physics/ps_vbo.h"
#include "physics/ps_movement.h"
#include "world/ps_lights.h"
#include "world/ps_renderworld.h"
#include "pipelines/ps_vu1pipeline.h"
#include "math/ps_quat.h"
#include "body.h"
#include "pad.h"
#include "skybox.h"
#include "graphics/shadows.h"
#include "dma/ps_dma.h"
#include "animation/ps_morphtarget.h"
#include "pipelines/ps_pipelines.h"
#include "math/ps_fast_maths.h"
#include "log/ps_log.h"
#include "animation/ps_animation.h"
#include "audio/ps_sound.h"
#include "io/ps_async.h"
#include "pipelines/ps_pipelinecbs.h"
#include "physics/ps_primtest.h"
#include "graphics/ps_renderdirect.h"
#include "geometry/ps_adjacency.h"
#include "graphics/ps_drawing.h"
#include "textures/ps_texturemanager.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

extern u32 VU1_LightStage3_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_LightStage3_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_GenericMorphTargetStage13D_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_GenericMorphTargetStage13D_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_EnvMapStage2_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_EnvMapStage2_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_AnimTexStage2_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_AnimTexStage2_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_SpecularLightStage3_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_SpecularLightStage3_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ClippingStage_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ClippingStage_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ClipStage4_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ClipStage4_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_GenericBonesAnimStage1_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_GenericBonesAnimStage1_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1_ShadowExtrusion_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_ShadowExtrusion_CodeEnd __attribute__((section(".vudata")));

TimerStruct *ts;

char print_out[50];

MATRIX animTransform, squareTransform, lightTransform, cameraTransform;

TessGrid tessGrid, tessGrid2;

Font *myFont;

float startTime;

VECTOR lightPos = {0.0f, 80.0f, +25.0f, 1.0f};
VECTOR mainDirLightDirection = {+1.0f, +2.0f, -0.5f, 0.0f};
VECTOR mainDirLightColor = {0.0f, 0.51f, 0.52f, 1.0f};

VECTOR secDirLightColor = {0.75f, 0.25f, 0.13f, 1.0f};
VECTOR ambientColor = {0.25f, 0.25f, 0.30f, 1.0f};

VECTOR roomAmbientColor = {0.10f, 0.10f, 0.10f, 1.0f};

const char *NewYorkName = "NEWYORK.PNG";
const char *face1Name = "FACE1.PNG";
const char *face2Name = "FACE2.PNG";
const char *face3Name = "FACE3.PNG";
const char *face4Name = "FACE4.PNG";
const char *face5Name = "FACE5.PNG";
const char *face6Name = "FACE6.PNG";
const char *glossName = "SPHERE.PNG";
const char *worldName = "WORLD.BMP";
const char *wallName = "WALL.PNG";
const char *alphaMap = "ALPHA_MAP.PNG";
const char *digitZero = "DIGIT.PNG";
const char *digitOne = "DIGITM1.PNG";
const char *digitTwo = "DIGITM2.PNG";
const char *wowwer = "WOW.PNG";

GameObject *grid = NULL;
GameObject *body = NULL;
GameObject *sphere = NULL;
GameObject *room = NULL;
GameObject *multiSphere = NULL;
GameObject *box = NULL;
GameObject *bodyCollision = NULL;
GameObject *shotBox = NULL;

static float lodGrid[4] = {150.0f, 125.0f, 75.0f, 50.0f};

RenderWorld *world = NULL;
RenderWorld *roomWorld = NULL;

Camera *cam = NULL;

LightStruct *ambient = NULL;
LightStruct *direct = NULL;
LightStruct *secondLight = NULL;
LightStruct *point = NULL;
LightStruct *ambientRoom = NULL;
LightStruct *spotLight = NULL;

float rad = 5.0f;
float highAngle = 90.0f;
float lowAngle = 0.0f;

RenderTarget *shadowTarget = NULL, *resampledTarget = NULL;
Texture *shadowTexture = NULL, *resampledTexture = NULL;
Camera shadowCam;

GameObject *shadowTexView = NULL;
Texture *targetTex = NULL;

Texture *zero = NULL;

FaceVertexTable table = NULL;

int alpha = 0x80;

// #define RESAMPLED

int FrameCounter = 0;

MeshBuffers sphereTarget;

float k = -1.0f;

Line mainLine = {{-45.0f, 0.0f, 50.0f, 1.0f}, {-45.0f, 0.0f, -50.0f, 1.0f}};

Line whatter = {{-65.0f, 0.0f, -25.0f, 1.0f}, {-35.0f, 0.0, -25.0f, 1.0f}};

int highlightIndex;

static BoundingSphere lol2Sphere = {{-15.0f, 10.0f, 50.0f, 1.0f}, 10.0f};

static BoundingSphere lolSphere = {{-20.0f, 15.0f, -20.0f, 1.0f}, 5.0f};

static Plane planer = {{1.0, 1.0, 0.0, 1.0f}, {1.0, 2.0, -1.0, -3.0f}};

static Plane plane2 = {{0.0, 0.0, 7.0, 1.0f}, {2.0, 3.0, -1.0, -7.0f}};

#include "math/ps_line.h"

static Color shotBoxColor[36];
static Color normal;
static Color boxhigh;
static void ShotBoxIntersectCB(VECTOR *verts, int index)
{
    for (int i = index + 2; i >= index; i--)
    {
        shotBoxColor[i].r = boxhigh.r;
        shotBoxColor[i].g = boxhigh.g;
        shotBoxColor[i].b = boxhigh.b;
        shotBoxColor[i].a = boxhigh.a;
    }
}

static Ray rayray = {{0.0f, 0.0f, -10.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};

static Ray rayray2 = {{-5.0f, 0.0f, -5.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}};

static VECTOR volLightPos = {-20.0f, 15.0f, -20.0f, 1.0f};


#if 0
VECTOR drawVECS[250];
u32 drawCnt;

static void WriteOut(VECTOR a, VECTOR b)
{
    VECTOR v3 = {0.0f, 0.0f, 0.0f, 1.0f};
    VECTOR v4 = {0.0f, 0.0f, 0.0f, 1.0f};
    VectorSubtractXYZ(a, volLightPos, v3);
    VectorScaleXYZ(v3, v3, 2.0f);
    VectorSubtractXYZ(b, volLightPos, v4);
    VectorScaleXYZ(v4, v4, 2.0f);
    VectorCopy(drawVECS[drawCnt], a);
    VectorAddXYZ(v3, a, drawVECS[drawCnt + 1]);
    VectorCopy(drawVECS[drawCnt + 2], b);
    VectorAddXYZ(v3, a, drawVECS[drawCnt + 3]);
    VectorCopy(drawVECS[drawCnt + 4], b);
    VectorAddXYZ(v4, b, drawVECS[drawCnt + 5]);
    for (int v = drawCnt; v < drawCnt + 6; v++)
    {
        drawVECS[v][3] = 1.0f;
    }
    drawCnt += 6;
}

static void DrawShadowExtrusion2(VECTOR *vertices, u32 numVerts, MATRIX m)
{
    u32 count = 0;
    u32 face = 0;
    for (int i = 0; i < numVerts; i += count + 1, face++)
    {

        count = *((u32 *)&vertices[i][3]);
        VECTOR v1, v2, v3, v7, v8, v9, cross, dists;
        ZeroVector(dists);
        int ind = 1;
        MatrixVectorMultiply(v1, m, vertices[i + ind++]);
        MatrixVectorMultiply(v2, m, vertices[i + ind++]);
        MatrixVectorMultiply(v3, m, vertices[i + ind++]);
        VectorSubtractXYZ(v2, v1, v7);
        VectorSubtractXYZ(v3, v2, v8);
        CrossProduct(v7, v8, cross);
        VectorSubtractXYZ(volLightPos, v1, v9);
        float d = DotProduct(cross, v9);
        if (d <= 0.0f)
            continue;
        if (vertices[i][0] < 0.0f)
        {
            WriteOut(v1, v2);
        }
        else
        {
            MatrixVectorMultiply(v9, m, vertices[i + ind++]);
            VectorSubtractXYZ(v9, v2, v9);
            CrossProduct(v9, v7, cross);
            VectorSubtractXYZ(volLightPos, v1, v9);
            dists[0] = DotProduct(cross, v9);
        }

        if (vertices[i][1] < 0.0f)
        {
            WriteOut(v2, v3);
        }
        else
        {
            MatrixVectorMultiply(v9, m, vertices[i + ind++]);
            VectorSubtractXYZ(v9, v3, v9);
            CrossProduct(v9, v8, cross);
            VectorSubtractXYZ(volLightPos, v2, v9);
            dists[1] = DotProduct(cross, v9);
        }

        if (vertices[i][2] < 0.0f)
        {
            WriteOut(v3, v1);
        }
        else
        {
            MatrixVectorMultiply(v9, m, vertices[i + ind++]);
            VectorSubtractXYZ(v1, v3, v8);
            VectorSubtractXYZ(v9, v3, v9);
            CrossProduct(v9, v8, cross);
            VectorSubtractXYZ(volLightPos, v3, v9);
            dists[2] = DotProduct(cross, v9);
        }

        if (dists[0] <= 0.0f)
        {
            WriteOut(v1, v2);
        }

        if (dists[1] <= 0.0f)
        {
            WriteOut(v2, v3);
        }

        if (dists[2] <= 0.0f)
        {
            WriteOut(v3, v1);
        }
    }
}

#endif



static void update_cube(GameObject *cube)
{
    static float angle = 1.0f;

    RotateYLTM(cube->ltm, angle);

    SetDirtyLTM(cube->ltm);
}

static void SetupFont()
{
    myFont = CreateFontStruct("FONTS\\LAUNCHERFONT.BMP", "FONTS\\LAUNCHERDATA.DAT", READ_BMP);
}

static void SetupWorldObjects()
{
    VECTOR camera_position = {55.0f, 0.0f, +120.00f, 1.00f};

    VECTOR at = {+50.0f, 0.0f, +100.0f, 0.0f};

    cam = InitCamera(g_Manager.ScreenWidth, g_Manager.ScreenHeight, 1.0f, 1500.0, graph_aspect_ratio(), 60.0f);

    InitCameraVBOContainer(cam, 10.0f, 10.0f, 10.0f, VBO_FIT);

    CameraLookAt(cam, camera_position, at, up);

    CreateProjectionMatrix(cam->proj, cam->width, cam->height, cam->aspect, 0.1f, 1500.0f, cam->angle);

    UpdateCameraMatrix(cam);

    CreateCameraFrustum(cam);

    SetLastAndDirtyLTM(cam->ltm, 1.0f);

    world = CreateRenderWorld();
    world->cam = cam;

    SetGlobalDrawingCamera(cam);
}

static void CreateLights()
{

    direct = CreateLightStruct(PS_DIRECTIONAL_LIGHT);
    SetLightColor(direct, mainDirLightColor);
    PitchLTM(direct->ltm, +25.0f);
    RotateYLTM(direct->ltm, +25.0f);
    //AddLightToRenderWorld(world, direct);

    secondLight = CreateLightStruct(PS_DIRECTIONAL_LIGHT);
    SetLightColor(secondLight, secDirLightColor);
    PitchLTM(secondLight->ltm, -30.0f);
    RotateYLTM(secondLight->ltm, -25.0f);
    // AddLightToRenderWorld(world, secondLight);

    ambient = CreateLightStruct(PS_AMBIENT_LIGHT);
    SetLightColor(ambient, ambientColor);
    AddLightToRenderWorld(world, ambient);

    ambientRoom = CreateLightStruct(PS_AMBIENT_LIGHT);
    SetLightColor(ambientRoom, roomAmbientColor);

    VECTOR pos = {+0.0f, +0.0f, 0.0f, 0.0f};

    VECTOR pointColor = {1.0f, 1.0f, 1.00f, 0.0f};

    point = CreateLightStruct(PS_POINT_LIGHT);
    SetLightColor(point, pointColor);
    SetPositionVectorLTM(point->ltm, pos);
    SetLightRadius(point, 5.0f);

    //  AddLightToRenderWorld(roomWorld, point);

    spotLight = CreateLightStruct(PS_SPOT_LIGHT);

    VECTOR spotpos = {+0.0f, +0.0f, 15.0f, 0.0f};

    VECTOR spotColor = {1.0f, 1.0f, 1.00f, 0.0f};
    SetLightColor(spotLight, spotColor);
    SetPositionVectorLTM(spotLight->ltm, spotpos);
    SetLightRadius(spotLight, 10.0f);
    SetLightTheta(spotLight, 90.0f);

    RotateYLTM(spotLight->ltm, 180.0f);

    //AddLightToRenderWorld(roomWorld, spotLight);
}

static void SetupGrid()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0xFF, 0xFF, 0xFF, 0x80, 1.0f);

    grid = InitializeGameObject();
    SetupGameObjectPrimRegs(grid, color, RENDERNORMAL | CLIPPING);

    int dw, dl;
    float w, h;
    dw = 10;
    dl = 10;
    w = 1000;
    h = 1000;
    CreateGrid(dw, dl, w, h, &grid->vertexBuffer);
    u64 id = GetTextureIDByName(g_Manager.texManager, worldName);

    CreateMaterial(&grid->vertexBuffer, 0, grid->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, -15.0f, 0.0f, 1.0f};

    VECTOR scales = {.5f, .5f, .5f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, grid->ltm);

    PitchLTM(grid->ltm, 0.0f);
    grid->update_object = NULL;

    // InitOBB(grid, VBO_FIXED);

    CreateGraphicsPipeline(grid, "Clipper");

    //  CreateShadowMapVU1Pipeline(box, 0, DEFAULT_PIPELINE_SIZE);

    AddObjectToRenderWorld(world, grid);
}

static void SetupBody()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {-50.0f, 0.0f, 0.0f, 0.0f};

    body = InitializeGameObject();

    ReadModelFile("MODELS\\BODY.BIN", &body->vertexBuffer);

    SetupGameObjectPrimRegs(body, color, RENDERTEXTUREMAPPED | CLIPPING | SKELETAL_ANIMATION);

    VECTOR scales = {.1f, .1f, .1f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, body->ltm);

    CreateMaterial(&body->vertexBuffer, 0, body->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, GetTextureIDByName(g_Manager.texManager, worldName));

    body->update_object = NULL;

    InitVBO(body, VBO_FIXED);

    // CreateSphereTarget();

    AnimationData *data = GetAnimationByIndex(body->vertexBuffer.meshAnimationData->animations, 2);

    body->objAnimator = CreateAnimator(data);

    CreateGraphicsPipeline(body, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, body);
}

static void SetupAABBBox()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    box = InitializeGameObject();
    ReadModelFile("MODELS\\BOX.BIN", &box->vertexBuffer);
    SetupGameObjectPrimRegs(box, color, RENDERTEXTUREMAPPED | CULLING_OPTION);

    u64 id = GetTextureIDByName(g_Manager.texManager, worldName);

    CreateMaterial(&box->vertexBuffer, 0, box->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {50.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {1.f, 1.f, 1.f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, box->ltm);

    box->update_object = NULL;

    InitVBO(box, VBO_FIT);

    CreateGraphicsPipeline(box, "Clipper");

    AddObjectToRenderWorld(world, box);
}

u32 count;

VECTOR *adjs;

MATRIX m;

static void SetupShootBoxBox()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    shotBox = InitializeGameObject();
    ReadModelFile("MODELS\\BOX.BIN", &shotBox->vertexBuffer);
    SetupGameObjectPrimRegs(shotBox, color, DRAWING_OPTION | COLOR_ENABLE | ZSTATE(1));

    u64 id = GetTextureIDByName(g_Manager.texManager, worldName);

    CreateMaterial(&shotBox->vertexBuffer, 0, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {.25f, .25f, .25f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, shotBox->ltm);

    shotBox->update_object = NULL;

    InitVBO(shotBox, VBO_FIT);

    table = ComputeFaceToVertexTable(
        shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices,
        shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount);

    CreateWorldMatrixLTM(shotBox->ltm, m);

    adjs = CreateAdjacencyVertices(table, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices,
                                   shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount, &count);
}

GameObject *shotBoxBig = NULL;

static void SetupShootBigBoxBox()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    shotBoxBig = InitializeGameObject();
    ReadModelFile("MODELS\\BOX.BIN", &shotBoxBig->vertexBuffer);
    SetupGameObjectPrimRegs(shotBoxBig, color, (DRAWING_OPTION | ZSTATE(1)));

    VECTOR pos = {-50.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {.75f, .75f, .75f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, shotBoxBig->ltm);

    shotBoxBig->update_object = NULL;
}

static void SetupOBBBody()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    bodyCollision = InitializeGameObject();
    ReadModelFile("MODELS\\BODY.BIN", &bodyCollision->vertexBuffer);
    SetupGameObjectPrimRegs(bodyCollision, color, RENDERTEXTUREMAPPED | CLIPPING);

    u64 id = GetTextureIDByName(g_Manager.texManager, worldName);

    CreateMaterial(&bodyCollision->vertexBuffer, 0, bodyCollision->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, 100.0f, 0.0f, 1.0f};

    VECTOR scales = {10.0f, 10.0f, 10.0f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, bodyCollision->ltm);

    bodyCollision->update_object = NULL;

    PitchLTM(bodyCollision->ltm, 90.0f);

    InitVBO(bodyCollision, VBO_FIXED);

    CreateGraphicsPipeline(bodyCollision, "Clipper");

    AddObjectToRenderWorld(world, bodyCollision);
}

static void SetupGameObjects()
{

    // InitSkybox();

    SetupGrid();
    // SetupBody();
    SetupAABBBox();
    // SetupOBBBody();
    SetupShootBoxBox();
    //SetupShootBigBoxBox();
    // SetupMultiSphere();
    //  SetupShadowViewer();

    // SetupRoom();

    //  SetupTessObject();
}

static void CleanUpGame()
{
    CleanCameraObject(cam);
    DestoryRenderWorld(world);
    DestoryRenderWorld(roomWorld);
    ClearManagerStruct(&g_Manager);
}



void DrawTexturedObject(GameObject *obj)
{
    PollVU1DoneProcessing(&g_Manager);
    MATRIX vp;
    MATRIX world = {1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                -15.0f, 50.0f, 0.0f, 1.0f};
    BeginCommand();
    BindTexture(GetTexByName(g_Manager.texManager, wowwer), false);
   
    MatrixIdentity(vp);
    MatrixMultiply(vp, vp, world);
    MatrixMultiply(vp, vp, g_DrawCamera->viewProj);
    
    ShaderHeaderLocation(16);
    ShaderProgram(0);
    DepthTest(true, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
     
   
    AllocateShaderSpace(16, 0);
    
   PushMatrix(vp, 0, sizeof(MATRIX));
    PushScaleVector();
    PushColor(obj->renderState.color.r, obj->renderState.color.g, obj->renderState.color.b, obj->renderState.color.a, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, 
                GS_SET_PRIM(obj->renderState.prim.type, obj->renderState.prim.shading, 
                true, obj->renderState.prim.fogging, 
                obj->renderState.prim.blending, obj->renderState.prim.antialiasing, 
                PRIM_MAP_ST, g_Manager.gs_context, PRIM_UNFIXED), 0, 3), DRAW_STQ2_REGLIST, 10);
    PushInteger(RENDERTEXTUREMAPPED, 12, 3);

    DrawCount(18, 2, true);
    for (int i = 0; i<18; i++)
    {
        DrawVector(obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
    }

    for (int i = 0; i<18; i++)
    {
        DrawVector(obj->vertexBuffer.meshData[MESHTRIANGLES]->texCoords[i]);
    }

    StartVertexShader();
    
    BindTexture(GetTexByName(g_Manager.texManager, worldName), true);
    
    DrawCount(obj->vertexBuffer.meshData[1]->vertexCount/2, 2, true);
    for (int i = obj->vertexBuffer.meshData[1]->vertexCount/2; i<obj->vertexBuffer.meshData[1]->vertexCount; i++)
    {
       DrawVector(obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
    }

    for (int i = obj->vertexBuffer.meshData[1]->vertexCount/2; i<obj->vertexBuffer.meshData[1]->vertexCount; i++)
    {
       DrawVector(obj->vertexBuffer.meshData[MESHTRIANGLES]->texCoords[i]);
    }

    StartVertexShader();
    EndCommand(); 
}


void DrawShadowQuad(int height, int width, int xOffset, int yOffset, u32 destTest, u32 setFrameMask, u8 alpha, u8 red, u8 green, u8 blue)
{
    PollVU1DoneProcessing(&g_Manager);
    bool destTestEnable = destTest;
    BeginCommand();
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    DestinationAlphaTest(destTestEnable, destTest);
    FrameBufferMaskWord(setFrameMask);
    DepthBufferMask(true);
    SetRegSizeAndType(2, DRAW_RGBAQ_REGLIST);
    PrimitiveType(GS_SET_PRIM(PRIM_TRIANGLE_STRIP, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED));
    DrawCount(4, 1, false);
    WritePairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, -), 0xFFFFFF));
    WritePairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    WritePairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, -), 0xFFFFFF));
    WritePairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    FrameBufferMask(0, 0, 0, 0);
    DepthBufferMask(false);
    EndCommand();            
}

static void RenderShadowVertices(VECTOR *verts, u32 numVerts, MATRIX m)
{
    PollVU1DoneProcessing(&g_Manager);

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(8);
    DepthTest(true, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    FrameBufferMask(0xFF, 0xFF, 0xFF, 0x00);
    DepthBufferMask(true);
    
    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(0, 0, 0, 0x80, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_FLAT,
     DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushMatrix(volLightPos, 11, 12);
    PushInteger(0x3, 11, 3);
    PushMatrix(*GetPositionVectorLTM(cam->ltm), 15, sizeof(VECTOR));
    DrawCount(count, 1, true);
    for (int i = 0; i < count; i++)
    {
        DrawVector(verts[i]);
    }
    StartVertexShader();
    AllocateShaderSpace(3, 9);
    PushColor(0, 0, 0, 0, 0);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_FLAT,
     DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 1);
    PushMatrix(volLightPos, 2, 12);
    PushInteger(0x0, 2, 3);
    DrawCount(count, 1, true);
    for (int i = 0; i < count; i++)
    {
        DrawVector(verts[i]);
    }
    StartVertexShader();
    FrameBufferMask(0x0, 0x0, 0x0, 0x0);
    DepthBufferMask(false);
    EndCommand();
}

Color *colors[5];
static Color lcolors[5];
Color highlight;
Color *held = NULL;
int objectIndex;
int moveX, moveY, moveZ;
#include "math/ps_ray.h"
void TestObjects()
{
    VECTOR temp;
    int ret = 1;
    CreateVector(moveX * 1.25, moveY * 1.25, moveZ * 1.25, 1.0f, temp);

    if (objectIndex == 0)
    {
        VectorAddXYZ(mainLine.p1, temp, mainLine.p1);
        VectorAddXYZ(mainLine.p2, temp, mainLine.p2);
        VectorAddXYZ(rayray2.origin, temp, rayray2.origin);
        // ret = LineIntersectLine(&whatter, &mainLine, temp);
        // ret = RayIntersectRay(&rayray, &rayray2);
        // DumpVector(temp);
        /* ret = RayIntersectPlane(&rayray, &plane2, temp);
         float tmep;
         DEBUGLOG("%d %d", RayIntersectSphere(&rayray, &lolSphere, temp),
          RayIntersectBox(&rayray, box->vboContainer->vbo, temp, &tmep)); */
        // VectorAddXYZ(*GetPositionVectorLTM(shotBox->ltm), temp, *GetPositionVectorLTM(shotBox->ltm));
        MATRIX m;
        CreateWorldMatrixLTM(shotBox->ltm, m);
        VECTOR v[4];
        for (int i = 0; i < 3; i++)
        {
            MatrixVectorMultiply(v[i], m, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
        }

        ret = RayIntersectsTriangle(&rayray2, v[0], v[1], v[2], v[3]);
    }
    if (objectIndex == 1)
    {
        BoundingBox *boxx = (BoundingBox *)box->vboContainer->vbo;

        MoveBox(boxx, temp);

        //  ret = LineSegmentIntersectBox(&mainLine, boxx, temp);

        // DEBUGLOG("%f", Sqrt(SqrDistFromAABB(lolSphere.center, boxx)));

        // DEBUGLOG("wowowowo %d", PlaneAABBCollision(&planer, boxx));

        MATRIX m;
        CreateWorldMatrixLTM(shotBox->ltm, m);
        VECTOR v[4];
        for (int i = 0; i < 3; i++)
        {
            MatrixVectorMultiply(v[i], m, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
        }
        float t = 0.f;
        // ret = AABBIntersectTriangle(v[0], v[1], v[2], boxx);

        ret = RayIntersectBox(&rayray2, boxx, v[3], &t);
    }
    else if (objectIndex == 3)
    {

        BoundingSphere *sph = &lolSphere;
        VectorAddXYZ(sph->center, temp, sph->center);
        VectorAddXYZ(volLightPos, temp, volLightPos);

        ret = LineSegmentIntersectSphere(&mainLine, sph, temp);

        BoundingBox boxx;
        BoundingBox *boxx2 = (BoundingBox *)bodyCollision->vboContainer->vbo;
        MATRIX world;
        VECTOR center, half;
        CreateWorldMatrixLTM(bodyCollision->ltm, world);

        MatrixVectorMultiply(boxx.top, world, boxx2->top);
        MatrixVectorMultiply(boxx.bottom, world, boxx2->bottom);
        FindCenterAndHalfAABB(&boxx, center, half);

        DumpVector(lolSphere.center);

        DEBUGLOG("dist from line %f", DistanceFromLineSegment(&mainLine, lolSphere.center, temp));
    }
    else if (objectIndex == 4)
    {
        ret = LineSegmentIntersectPlane(&mainLine, planer.planeEquation, planer.pointInPlane);
    }

    if (!ret)
        DEBUGLOG("%d hits the line", objectIndex);
}

static u8 glowR = 255;
static u8 glowG = 128;
static u8 glowB = 128;


int Render()
{
    CREATE_RGBAQ_STRUCT(highlight, 255, 0, 0, 128, 0);
    CREATE_RGBAQ_STRUCT(lcolors[0], 0, 0, 255, 128, 0);
    CREATE_RGBAQ_STRUCT(lcolors[1], 0, 255, 0, 128, 0);
    CREATE_RGBAQ_STRUCT(lcolors[2], 255, 0, 255, 128, 0);
    CREATE_RGBAQ_STRUCT(lcolors[3], 255, 255, 0, 128, 0);
    CREATE_RGBAQ_STRUCT(lcolors[4], 128, 128, 128, 128, 0);

    CREATE_RGBAQ_STRUCT(boxhigh, 255, 128, 128, 128, 0);
    CREATE_RGBAQ_STRUCT(normal, 128, 255, 128, 128, 0);

    for (int i = 0; i < 3; i++)
    {
        CREATE_RGBAQ_STRUCT(shotBoxColor[i], 128, 255, 255, 128, 0);
    }

    for (int i = 3; i < 36; i++)
    {
        CREATE_RGBAQ_STRUCT(shotBoxColor[i], 128, 255, 128, 128, 0);
    }

    colors[0] = &highlight;
    held = &lcolors[0];
    colors[1] = &lcolors[1];
    colors[2] = &lcolors[2];
    colors[3] = &lcolors[3];
    colors[4] = &lcolors[4];
    float lastTime = getTicks(g_Manager.timer);

    *R_EE_GIF_MODE |= 0x04;
    for (;;)
    {
        float currentTime = getTicks(g_Manager.timer);
        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;

        TestObjects();

        UpdatePad();

        if (body)
            UpdateAnimator(body->objAnimator, delta);

        float time1 = getTicks(g_Manager.timer);

        StartFrame();

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, 0xFF, 0xFF, 0xFF, 0x80);

         qword_t what[2];
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
        CreateDMATag(what, DMA_END, 0, VIF_CODE(0x8000, 0, VIF_CMD_MSKPATH3, 0), 0, 0);
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
       dma_channel_send_chain(DMA_CHANNEL_VIF1, what, 1, 1, 0);
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);

        //DrawWorld(world);
        MATRIX ident;
        MatrixIdentity(ident);

        

        RenderSphereLine(&lolSphere, *colors[3], 40);
        RenderGameObject(shotBox, shotBoxColor);
        RenderPlaneLine(&plane2, *colors[1], 20);

        
        
        RenderRay(&rayray2, *colors[2], 25);
        RenderLine(&mainLine, *colors[3]);
       RenderAABBBoxLine(shotBox->vboContainer->vbo, *colors[2], ident);

       

        DrawShadowQuad(g_Manager.ScreenHeight, g_Manager.ScreenWidth, 0, 0, 0, 0x00FFFFFF, 0, 0, 0, 0);

       RenderShadowVertices(adjs, count, m);

        //ReadFromVU(vu1_data_address + (*vif1_top * 4), 256*4, 0);

     DrawShadowQuad(g_Manager.ScreenHeight, g_Manager.ScreenWidth, 0, 0, 1, 0xFF000000, 0, 0, 0, 0);


        DrawTexturedObject(shotBox);

        snprintf(print_out, 35, "DERRICK REGINALD %d", FrameCounter);

       PrintText(myFont, print_out, -310, -220, LEFT);


        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
        CreateDMATag(what, DMA_END, 0, VIF_CODE(0x0000, 0, VIF_CMD_MSKPATH3, 0), 0, 0);
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);
        dma_channel_send_chain(DMA_CHANNEL_VIF1, what, 1, 1, 0);
        dma_channel_wait(DMA_CHANNEL_VIF1, -1);


        EndRendering(cam);

        EndFrame(true);
       // while(true);
        FrameCounter++;
    }

    return 0;
}

static void SetupVU1Programs()
{

    VU1Program *prog;

    prog = CreateVU1Program(&VU1_ClipStage4_CodeStart, &VU1_ClipStage4_CodeEnd, 0); // 0

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_LightStage3_CodeStart, &VU1_LightStage3_CodeEnd, 0); // 1

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_GenericMorphTargetStage13D_CodeStart, &VU1_GenericMorphTargetStage13D_CodeEnd, 0); // 2

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_EnvMapStage2_CodeStart, &VU1_EnvMapStage2_CodeEnd, 0); // 3

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_AnimTexStage2_CodeStart, &VU1_AnimTexStage2_CodeEnd, 0); // 4

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_SpecularLightStage3_CodeStart, &VU1_SpecularLightStage3_CodeEnd, 0); // 5

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_ClippingStage_CodeStart, &VU1_ClippingStage_CodeEnd, 0); // 6

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_GenericBonesAnimStage1_CodeStart, &VU1_GenericBonesAnimStage1_CodeEnd, 0); // 7

    AddProgramToManager(g_Manager.vu1Manager, prog);

    prog = CreateVU1Program(&VU1_ShadowExtrusion_CodeStart, &VU1_ShadowExtrusion_CodeEnd, 0); // 8

    AddProgramToManager(g_Manager.vu1Manager, prog);
}

static void LoadInTextures()
{
    char _file[MAX_FILE_NAME];

    char _folder[9] = "TEXTURES\\";

    AppendString(_folder, worldName, _file, MAX_FILE_NAME);

    Texture *tex = AddAndCreateTexture(_file, READ_BMP, 1, 0xFF, TEX_ADDRESS_CLAMP);

    SetFilters(tex, PS_FILTER_BILINEAR);

    AppendString(_folder, wowwer, _file, MAX_FILE_NAME);

    tex = AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP);

    SetFilters(tex, PS_FILTER_BILINEAR);
}

void StartUpSystem()
{
    ManagerInfo info;
    info.doublebuffered = true;
    info.drawBufferSize = 2000;
    info.fsaa = true;
    info.zenable = true;
    info.height = 448;
    info.width = 640;
    info.psm = GS_PSM_32;
    info.zsm = GS_PSMZ_24;
    info.vu1programsize = 10;
    InitializeSystem(&info);

    SetupWorldObjects();
}

int main(int argc, char **argv)
{
    StartUpSystem();

    

    float totalTime;

    float startTime = totalTime = getTicks(g_Manager.timer);

    SetupVU1Programs();

    float endTime = getTicks(g_Manager.timer);

    DEBUGLOG("VU1 programs %f", endTime - startTime);

    startTime = getTicks(g_Manager.timer);

    LoadInTextures();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("texes %f", endTime - startTime);

    CreateLights();

    SetupFont();

    startTime = getTicks(g_Manager.timer);

    SetupGameObjects();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("gos %f", endTime - startTime);

    

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("total %f", endTime - totalTime);

    audsrv_adpcm_t sample;

    VagFile *vag = LoadVagFile("SOUNDS\\HOW.VAG");

    int ret;
    printf("sample: kicking IRXs\n");
    ret = SifLoadModule("cdrom0:\\LIBSD.IRX", 0, NULL);
    printf("libsd loadmodule %d\n", ret);

    printf("sample: loading audsrv\n");
    ret = SifLoadModule("cdrom0:\\AUDSRV.IRX", 0, NULL);
    printf("audsrv loadmodule %d\n", ret);

    ret = audsrv_init();

    audsrv_adpcm_init();

    audsrv_load_adpcm(&sample, vag->samples, vag->header.dataLength + 16);
    DEBUGLOG("%d %d %d %d %d", sample.pitch, sample.loop, sample.channels, sample.size, vag->header.sampleRate);
    int channel = audsrv_ch_play_adpcm(-1, &sample);
    audsrv_adpcm_set_volume(channel, MAX_VOLUME);

    Render();

    DestroyVAGFile(vag);

    CleanUpGame();

    SleepThread();

    return 0;
}
#pragma GCC diagnostic pop
