#include "ps_global.h"

#include <graph.h>
#include <audsrv.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

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
#include "physics/ps_obb.h"
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
const char *worldName = "WORLD.PNG";
const char *wallName = "WALL.PNG";
const char *alphaMap = "ALPHA_MAP.PNG";
const char *digitZero = "DIGIT.PNG";
const char *digitOne = "DIGITM1.PNG";
const char *digitTwo = "DIGITM2.PNG";

GameObject *grid = NULL;
GameObject *body = NULL;
GameObject *sphere = NULL;
GameObject *room = NULL;
GameObject *multiSphere = NULL;
GameObject *box = NULL;

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

int alpha = 0x80;

// #define RESAMPLED

int FrameCounter = 0;

MeshBuffers sphereTarget;

float k = -1.0f;

static void UpdateGlossTransform()
{
    CreateRotationAndCopyMatFromObjAxes(lightTransform, *GetUpVectorLTM(direct->ltm), *GetForwardVectorLTM(direct->ltm), *GetRightVectorLTM(direct->ltm));

    // MATRIX screen, m, camMatrix;
    // CreateWorldMatrixLTM(multiSphere->ltm, m);
    // MatrixIdentity(screen);

    // MatrixMultiply(screen, screen, m);
    // MatrixMultiply(screen, screen, cam->view);

    // MatrixInverse(lightTransform, lightTransform);

    MatrixTranspose(lightTransform);

    CreateNormalizedTextureCoordinateMatrix(lightTransform);
}

static void SetK()
{

    static int mag = LOD_MAG_LINEAR;

    SetupTexLODStruct(zero, k, 0, 2, 5, mag);
}

static void CreateSphereTarget()
{

    u32 size = 2;

    CreateMorphTargetBuffersFromFile("MODELS\\SPHEREDEFORMED.BIN", &sphereTarget);

    sphere = CreateObjectMorphBuffer(sphere, size);

    AddMeshToTargetBuffer(sphere->interpolator, &sphereTarget);

    CreateInterpolatorNodes(sphere->interpolator, size);

    for (int i = 1; i < size; i++)
    {
        AddInterpolatorNode(sphere->interpolator, 0, i, (float)((i) / 2.0f));

        AddInterpolatorNode(sphere->interpolator, i, 0, (float)((i + 1) / 2.0f));
    }

    SetInterpolatorNode(sphere->interpolator, 0);
}

static void UpdateSphereMorph()
{
    Interpolator *node = GetCurrentInterpolatorNode(sphere->interpolator);
    node->position -= 0.005f;
    SetDirtyLTM(sphere->ltm);
}

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

    cam = InitCamera(640, 480, 1.0f, 1500.0, graph_aspect_ratio(), 60.0f);

    InitCameraObb(cam, 10.0f, 10.0f, 10.0f, BBO_FIT);

    CameraLookAt(cam, camera_position, at, up);

    CreateProjectionMatrix(cam->proj, cam->width, cam->height, cam->aspect, 0.1f, 1500.0f, cam->angle);

    UpdateCameraMatrix(cam);

    CreateCameraFrustum(cam);

    SetGlobalManagerCam(cam);

    SetLastAndDirtyLTM(cam->ltm, 1.0f);

    world = CreateRenderWorld();
    world->cam = cam;

    roomWorld = CreateRenderWorld();
    roomWorld->cam = cam;

    SetGlobalDrawingCamera(cam);
}

static void CreateLights()
{

    direct = CreateLightStruct(PS_DIRECTIONAL_LIGHT);
    SetLightColor(direct, mainDirLightColor);
    PitchLTM(direct->ltm, +25.0f);
    RotateYLTM(direct->ltm, +25.0f);
    AddLightToRenderWorld(world, direct);

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

    AddLightToRenderWorld(roomWorld, spotLight);
}

static void UpdateLight()
{
    // PitchLTM(direct->ltm, 1.5f);
    // RotateYLTM(direct->ltm, 1.5f);

    // DumpMatrix(direct->ltm);

    // PitchLTM(secondLight->ltm, -1.5f);
    // RotateYLTM(secondLight->ltm, -1.0f);

    // DumpMatrix(secondLight->ltm);
}

static void SetupGrid()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    grid = InitializeGameObject();
    // ReadModelFile("MODELS\\BOX.BIN", &grid->vertexBuffer);
    SetupGameObjectPrimRegs(grid, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0));

    int dw, dl;
    float w, h;
    dw = 10;
    dl = 10;
    w = 100;
    h = 100;
    CreateGrid(dw, dl, w, h, &grid->vertexBuffer);
    u32 id = GetTextureIDByName(worldName, g_Manager.texManager);

    CreateMaterial(&grid->vertexBuffer, 0, grid->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {0.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {.5f, .5f, .5f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, grid->ltm);

    PitchLTM(grid->ltm, -45.0f);
    grid->update_object = NULL;

    // InitOBB(grid, BBO_FIXED);

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

    SetupGameObjectPrimRegs(body, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0));

    VECTOR scales = {.1f, .1f, .1f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, body->ltm);

    CreateMaterial(&body->vertexBuffer, 0, body->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, GetTextureIDByName(worldName, g_Manager.texManager));

    body->update_object = NULL;

    InitOBB(body, BBO_FIXED);

    // CreateSphereTarget();

    AnimationData *data = GetAnimationByIndex(body->vertexBuffer.meshAnimationData->animations, 2);

    body->objAnimator = CreateAnimator(data);

    CreateGraphicsPipeline(body, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, body);
}

static void RotateSphere()
{
    PitchLTM(multiSphere->ltm, 1.0f);
}

static void SetupMultiSphere()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+50.0f, 0.0f, +100.0f, 0.0f};

    multiSphere = InitializeGameObject();
    float time1 = getTicks(g_Manager.timer);
    ReadModelFile("MODELS\\TORUS1.BIN", &multiSphere->vertexBuffer);
    DEBUGLOG("TIME ON CPU FOR OBB %f", getTicks(g_Manager.timer) - time1);
    // envmap
    SetupGameObjectPrimRegs(multiSphere, color, RENDER_STATE(1, 1, 0, 0, 1, 1, 1, 3, 1, 0, 0, 1, 0, 0, 0, 0, 0));

    // alphamap
    // SetupGameObjectPrimRegs(multiSphere, color, RENDER_STATE(1, 1, 0, 0, 1, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1));
    VECTOR scales = {5.0f, 5.0f, 5.0f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, multiSphere->ltm);

    multiSphere->update_object = NULL;

    PitchLTM(multiSphere->ltm, -90.0f);

    CreateMaterial(&multiSphere->vertexBuffer, 0, multiSphere->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, GetTextureIDByName(NewYorkName, g_Manager.texManager));
    // float time1 = getTicks(g_Manager.timer);
    InitOBB(multiSphere, BBO_FIT);
    // DEBUGLOG("TIME ON CPU FOR OBB %f", getTicks(g_Manager.timer) - time1);
    multiSphere->update_object = RotateSphere;

    MatrixIdentity(lightTransform);

    CreateEnvMapPipeline(multiSphere, "ENVMAP_PIPE");

    SetEnvMapMATRIX(multiSphere->activePipeline, lightTransform);

    SetEnvMapTexture(multiSphere->activePipeline, GetTexByName(g_Manager.texManager, glossName));

    // CreateAlphaMapPipeline(multiSphere, "ALPHAMAP");

    // SetAlphaMapTexture(multiSphere->activePipeline, GetTexByName(g_Manager.texManager, alphaMap));

    AddObjectToRenderWorld(world, multiSphere);
}

static void SetupRoom()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+0.0f, +0.0f, 50.0f, 0.0f};

    room = InitializeGameObject();
    ReadModelFile("MODELS\\ROOM.BIN", &room->vertexBuffer);
    SetupGameObjectPrimRegs(room, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0));

    VECTOR scales = {25.0f, 25.0f, 25.0f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, room->ltm);

    RotateYLTM(room->ltm, -90.0f);

    room->update_object = NULL;

    InitOBB(room, BBO_FIT);

    CreateGraphicsPipeline(room, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(roomWorld, room);
}

static void SetupShadowViewer()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR pos = {+75.0f, +15.0f, -200.0f, 0.0f};

    shadowTexView = InitializeGameObject();
    // ReadModelFile("\\ROOM2.BIN;1", room);
    int w, l;
    float dw, dh;
    w = 25;
    l = 25;
    dw = 1;
    dh = 1;
    CreateGrid(w, l, dw, dh, &shadowTexView->vertexBuffer);

    SetupGameObjectPrimRegs(shadowTexView, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0));

    u32 id = 0;
#ifdef RESAMPLED
    id = resampledTexture->id;
#else
    id = shadowTexture->id;
#endif

    CreateMaterial(&shadowTexView->vertexBuffer, 0, shadowTexView->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, shadowTexView->ltm);

    // create_pipeline_obj_vu1pipeline(shadowTexView, VU1GenericTex3D, 1000);

    CreateGraphicsPipeline(shadowTexView, GEN_PIPELINE_NAME);
}

float ComputeDistanceFromFinitePlane(GameObject *obj, VECTOR pos, VECTOR topExtent, VECTOR bottomExtent)
{
    VECTOR pointInPlane = {0.0f, 0.0f, 1.0f, 1.0f};
    VECTOR planeNormal = {0.0, 1.0f, 0.0, 0.0f};
    VECTOR plane, planePoint, extentTopL, extentBottomL, temp1, temp2;
    MATRIX world;
    CreateWorldMatrixLTM(obj->ltm, world);

    MatrixVectorMultiply(planeNormal, world, planeNormal);
    MatrixVectorMultiply(pointInPlane, world, pointInPlane);
    MatrixVectorMultiply(temp1, world, topExtent);
    MatrixVectorMultiply(temp2, world, bottomExtent);

    extentTopL[0] = Max(temp1[0], temp2[0]);
    extentTopL[1] = Max(temp1[1], temp2[1]);
    extentTopL[2] = Max(temp1[2], temp2[2]);

    extentBottomL[0] = Min(temp1[0], temp2[0]);
    extentBottomL[1] = Min(temp1[1], temp2[1]);
    extentBottomL[2] = Min(temp1[2], temp2[2]);

    ComputePlane(pointInPlane, planeNormal, plane);

    PointInPlane(plane, pos, pointInPlane, planePoint);

    planePoint[0] = Min(Max(planePoint[0], extentBottomL[0]), extentTopL[0]);
    planePoint[1] = Min(Max(planePoint[1], extentBottomL[1]), extentTopL[1]);
    planePoint[2] = Min(Max(planePoint[2], extentBottomL[2]), extentTopL[2]);

    VECTOR distVec;
    VectorSubtractXYZ(pos, planePoint, distVec);

    return dist(distVec);
}

static void UpdateTessGrid(GameObject *obj)
{
    TessGrid *grid = (TessGrid *)obj->objData;
    float d = ComputeDistanceFromFinitePlane(obj, *GetPositionVectorLTM(cam->ltm), grid->extent.top, grid->extent.bottom);
    int i;
    for (i = 0; i < 4; i++)
    {
        if (d > lodGrid[i])
        {
            // INFOLOG("dist %f", d);
            break;
        }
    }

    float xDim = pow(2, (float)((i + 3)));
    float yDim = pow(2, (float)((i + 3)));

    grid->xDim = (int)xDim;
    grid->yDim = (int)yDim;

    // int totalVerts = xDim+1 * 2;

    //  INFOLOG("total verts in row %d", totalVerg_Manager.timer);
}

static void SetupTessObject()
{
    Color color;
    // CREATE_ALPHA_REGS(blender, BLEND_COLOR_DEST, BLEND_COLOR_SOURCE, BLEND_COLOR_SOURCE, BLEND_ALPHA_DEST, 0x80);
    CREATE_RGBAQ_STRUCT(color, 0xFF, 0xFF, 0xFF, 0x80, 1.0f);

    GameObject *lod_floor = InitializeGameObject();

    VECTOR tempPos2 = {0.0f, -20.0f, 0.0f, 1.0f};

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};

    SetupLTM(tempPos2, up, right, forward,
             scales,
             1.0f, lod_floor->ltm);

    SetupGameObjectPrimRegs(lod_floor, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    CreateVector(-250.0f, 0.0f, -250.0f, 1.0f, tessGrid.extent.top);

    CreateVector(250.0f, 0.0f, 250.0f, 1.0f, tessGrid.extent.bottom);

    tessGrid.xDim = 2;
    tessGrid.yDim = 2;

    lod_floor->objData = (void *)&tessGrid;

    lod_floor->update_object = UpdateTessGrid;

    create_pipeline_tess_grid_vu1pipeline(lod_floor, 2, 100, &tessGrid);

    AddObjectToRenderWorld(world, lod_floor);

    /// second wall

    GameObject *lod_wall = InitializeGameObject();

    VECTOR tempPos = {0.0f, +230.0f, -250.0f, 1.0f};

    SetupLTM(tempPos, up, right, forward,
             scales,
             1.0f, lod_wall->ltm);
    PitchLTM(lod_wall->ltm, -90.0f);

    SetupGameObjectPrimRegs(lod_wall, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    CreateVector(-250.0f, 0.0f, -250.0f, 1.0f, tessGrid2.extent.top);

    CreateVector(250.0f, 0.0f, 250.0f, 1.0f, tessGrid2.extent.bottom);

    tessGrid2.xDim = 2;
    tessGrid2.yDim = 2;

    lod_wall->objData = (void *)&tessGrid2;

    lod_wall->update_object = UpdateTessGrid;

    create_pipeline_tess_grid_vu1pipeline(lod_wall, 2, 100, &tessGrid2);

    AddObjectToRenderWorld(world, lod_wall);
}

static void SetupGameObjects()
{

    // InitSkybox();

    SetupGrid();
    SetupBody();

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
    ClearManagerTexList(&g_Manager);
    ClearManagerStruct(&g_Manager);
}

int shadowRes = 8;
static void SetupShadowRenderTarget()
{
    u32 res = 1 << shadowRes;

    shadowTarget = CreateRenderTarget(res, res, GS_PSMZ_24, ZTEST_METHOD_ALLPASS, GS_PSM_32);
    shadowTexture = CreateTextureFromRenderTarget(shadowTarget, 1, TEXTURE_FUNCTION_MODULATE);

    u32 small = res >> 1;

    resampledTarget = CreateRenderTarget(small, small, GS_PSMZ_24, ZTEST_METHOD_ALLPASS, GS_PSM_32);
    resampledTexture = CreateTextureFromRenderTarget(resampledTarget, 0, TEXTURE_FUNCTION_DECAL);

    AddToManagerTexList(&g_Manager, resampledTexture);
    AddToManagerTexList(&g_Manager, shadowTexture);

    VECTOR camPos = {0.0f, 0.0f, +10.0f, 0.0f}; //{-1.0f, 5.0f, +9.0f, 0.0f};
    VECTOR camAt = {0.0f, 0.0f, 0.0f, 0.0f};

    CameraLookAt(&shadowCam, camPos, camAt, up);

    CreateOrthoGraphicMatrix(+40.0f, -40.0f, +40.0f, -40.0f, 0.1, 100.0f, shadowCam.proj);

    UpdateCameraMatrix(&shadowCam);
}

static void RenderShadowScene()
{

    SetGlobalDrawingCamera(&shadowCam);

    SetupRenderTarget(shadowTarget, g_Manager.gs_context, 0);

    ClearScreen(shadowTarget, g_Manager.gs_context, 0xFF, 0xFF, 0xFF, alpha);

    GameObject *obj = grid;

    VU1Pipeline *pipe = GetPipelineByName("GENERIC_SHADOW_MAP", obj);

    if (pipe == NULL)
    {
        return;
    }

    RenderPipeline(obj, pipe);

#ifndef RESAMPLED
    RenderTarget *targ = shadowTarget;

    targetTex = shadowTexture;
#else
    SetupRenderTarget(resampledTarget, g_Manager.gs_context, 0);

    ClearScreen(resampledTarget, g_Manager.gs_context, 0xFF, 0xFF, 0xFF, 0x80);

    RenderTarget *targ = resampledTarget;

    targetTex = resampledTexture;

#endif
    u32 screenRes = targ->render->width >> 1;

    // INFOLOG("%d", screenRes);
#ifdef RESAMPLED
    DrawQuad(screenRes, screenRes, 0, 0, 0, shadowTexture);
#endif
    float uvStep = 1.0f / targ->render->width;

    float uvOff = 5.0f * uvStep;

    int texOFfW = (int)(uvOff * (targ->render->width >> 1));

    DrawQuad(screenRes, screenRes, texOFfW, texOFfW, 1, targetTex);

    EndRendering(&shadowCam);

    SetGlobalDrawingCamera(cam);

    SetupRenderTarget(g_Manager.targetBack, g_Manager.gs_context, 1);

    RenderPipeline(shadowTexView, shadowTexView->activePipeline);
}
static void FinishCube(void *object)
{
    GameObject *temp = box;
    InitOBB(temp, BBO_FIT);

    u32 id = GetTextureIDByName("WATER", g_Manager.texManager);

    CreateMaterial(&temp->vertexBuffer, 0, temp->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    DEBUGLOG("COUNT %d", temp->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount);

    CreateGraphicsPipeline(temp, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, temp);
}
static void LoadCube()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    box = InitializeGameObject();

    SetupGameObjectPrimRegs(box, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    VECTOR pos = {0.0f, 50.0f, 5.0f, 1.0f};

    VECTOR scales = {1.5f, 1.5f, 1.5f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, box->ltm);

    PitchLTM(box->ltm, -45.0f);
    box->update_object = NULL;

    LoadASync("MODELS\\ROOM.BIN", &box->vertexBuffer, NULL, CreateMeshBuffersFromFile, FinishCube);
}

static void FinishWater(void *object)
{
    // Material *mat = (Material*)grid->vertexBuffer.materials->data;
    Texture *tex = (Texture *)object;
    InitTextureResources(tex, TEX_ADDRESS_CLAMP);
    AddToManagerTexList(&g_Manager, tex);
    // mat->materialId = tex->id;
}

static void LoadWater()
{
    CreateTextureParams *params = (CreateTextureParams *)malloc(sizeof(CreateTextureParams));
    params->name = "WATER";
    params->readType = READ_PNG;
    params->useAlpha = 1;
    params->alpha = 0x80;
    Texture *tex = (Texture *)malloc(sizeof(Texture));
    LoadASync("TEXTURES\\MONET.PNG", tex, params, CreateTextureFromFile, FinishWater);
}

int Render()
{
    float lastTime = getTicks(g_Manager.timer);

    for (;;)
    {
        float currentTime = getTicks(g_Manager.timer);
        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;

        //  SetK();

        UpdatePad();
        if (body != NULL)
            UpdateAnimator(body->objAnimator, delta);

        // UpdateGlossTransform();

        if (FrameCounter == 250)
        {
            //  LoadWater();
            //  LoadCube();
        }

        float time1 = getTicks(g_Manager.timer);

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, g_Manager.bgkc.r, g_Manager.bgkc.g, g_Manager.bgkc.b, 0x80);

        DrawWorld(world);

       // DEBUGLOG("%f\n", getTicks(g_Manager.timer) - time1);

        // DrawWorld(roomWorld);

        // RenderShadowScene();

        // FindOBBMaxAndMinVerticesVU0(multiSphere);

        snprintf(print_out, 35, "DERRICK REGINALD %d", FrameCounter);

        PrintText(myFont, print_out, -310, -220);

        snprintf(print_out, 20, "K-VALUE %f", k);

        // PrintText(myFont, print_out, -310, -200);

        EndRendering(cam);

        EndFrame(1);

        HandleASyncIO();

        WakeupIOThread();

        UpdateLight();

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
}

static void LoadInTextures()
{
    char _file[MAX_FILE_NAME];

    char _folder[9] = "TEXTURES\\";

    /*  AppendString(_folder, face1Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, face2Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, face3Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, face4Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, face5Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, face6Name, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, NewYorkName, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP, 0);

      AppendString(_folder, glossName, _file, MAX_FILE_NAME);

      AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP, 1); */

    AppendString(_folder, worldName, _file, MAX_FILE_NAME);

    Texture *tex = AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP, 0);

    SetFilters(tex, PS_FILTER_BILINEAR);
}

int main(int argc, char **argv)
{
    InitializeSystem(1, 640, 480, GS_PSM_32);

    InitASyncIO(25, 5.0f);

    SetupWorldObjects();

    float totalTime;

    float startTime = totalTime = getTicks(g_Manager.timer);

    SetupVU1Programs();

    float endTime = getTicks(g_Manager.timer);

    DEBUGLOG("VU1 programs %f", endTime - startTime);

    startTime = getTicks(g_Manager.timer);

    LoadInTextures();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("texes %f", endTime - startTime);

    SetupShadowRenderTarget();

    CreateLights();

    startTime = getTicks(g_Manager.timer);

    SetupGameObjects();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("gos %f", endTime - startTime);

    SetupFont();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("total %f", endTime - totalTime);

    audsrv_adpcm_t sample;

    VagFile *vag = LoadVagFile("SOUNDS\\COME.VAG");

    SifInitRpc(0);
    int ret;
    printf("sample: kicking IRXs\n");
    ret = SifLoadModule("cdrom0:\\LIBSD.IRX", 0, NULL);
    printf("libsd loadmodule %d\n", ret);

    printf("sample: loading audsrv\n");
    ret = SifLoadModule("cdrom0:\\AUDSRV.IRX", 0, NULL);
    printf("audsrv loadmodule %d\n", ret);

    ret = audsrv_init();

    audsrv_adpcm_init();

    audsrv_load_adpcm(&sample, vag->samples, vag->header.dataLength+16);
    DEBUGLOG("%d %d %d %d %d", sample.pitch, sample.loop, sample.channels, sample.size, vag->header.sampleRate);
    int channel = audsrv_ch_play_adpcm(-1, &sample);
    audsrv_adpcm_set_volume(channel, MAX_VOLUME);

    Render();

    CleanUpGame();

    SleepThread();

    return 0;
}
#pragma GCC diagnostic pop
