#include "ps_global.h"
#include "ps_gs.h"
#include "ps_vif.h"
#include "ps_misc.h"
#include "ps_timer.h"
#include "ps_texture.h"
#include "ps_pad.h"
#include "ps_texture_io.h"
#include "ps_vumanager.h"
#include "ps_font.h"
#include "ps_manager.h"
#include "ps_camera.h"
#include "ps_file_io.h"
#include "ps_gameobject.h"
#include "ps_obb.h"
#include "ps_movement.h"
#include "ps_lights.h"
#include "ps_renderworld.h"
#include "ps_vu1pipeline.h"
#include "ps_quat.h"
#include "body.h"
#include "pad.h"
#include "skybox.h"
#include "shadows.h"
#include "ps_dma.h"
#include "ps_morphtarget.h"
#include "ps_pipelines.h"
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <graph.h>
#include "ps_fast_maths.h"
#include <stdlib.h>
#include "ps_log.h"
#include "ps_animation.h"

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

char print_out[20] = "DREW FLETCHER";

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

const char *waterName = "WATER.BMP";
const char *NewYorkName = "NEWYORK.PNG";
const char *face1Name = "FACE1.PNG";
const char *face2Name = "FACE2.PNG";
const char *face3Name = "FACE3.PNG";
const char *face4Name = "FACE4.PNG";
const char *face5Name = "FACE5.PNG";
const char *face6Name = "FACE6.PNG";
const char *glossName = "GLOSS.PNG";
const char *worldName = "WORLD.PNG";
const char *wallName = "WALL.PNG";

GameObject *box;
GameObject *sphere;
GameObject *room;
GameObject *body;
GameObject *multiSphere;

static float lodGrid[4] = {150.0f, 125.0f, 75.0f, 50.0f};

RenderWorld *world;
RenderWorld *roomWorld;

Camera *cam;

LightStruct *ambient;
LightStruct *direct;
LightStruct *secondLight;
LightStruct *point;
LightStruct *ambientRoom;
LightStruct *spotLight;

float rad = 5.0f;
float highAngle = 90.0f;
float lowAngle = 0.0f;

RenderTarget *shadowTarget, *resampledTarget;
Texture *shadowTexture, *resampledTexture;
Camera shadowCam;

GameObject *shadowTexView;
Texture *targetTex;

int alpha = 0x80;

// #define RESAMPLED

int no_plugin = 0;

MeshBuffers sphereTarget;

static void UpdateGlossTransform()
{
    CreateRotationAndCopyMatFromObjAxes(lightTransform, *GetUpVectorLTM(direct->ltm), *GetForwardVectorLTM(direct->ltm), *GetRightVectorLTM(direct->ltm));

    MatrixTranspose(lightTransform);

    CreateNormalizedTextureCoordinateMatrix(lightTransform);
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
    myFont = CreateFontStruct("FONTS\\TIMES.BMP", "FONTS\\TIMESNR.DAT", READ_BMP);
}

static void SetupWorldObjects()
{
    VECTOR camera_position = {150.0f, 300.0f, +200.00f, 1.00f};

    VECTOR at = {-50.0f, 0.0f, 0.0f, 0.0f};

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
    AddLightToRenderWorld(world, secondLight);

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

static void SetupCube()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    box = InitializeGameObject();
    // ReadModelFile("MODELS\\BOX.BIN", &box->vertexBuffer);
    SetupGameObjectPrimRegs(box, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 0, 0, 1, 0, 0, 0, 0, 0));

    DEBUGLOG("Here is the render state 0x%x", box->renderState.state.render_state.state);

    int w, l;
    float dw, dh;
    w = 25;
    l = 25;
    dw = 100;
    dh = 100;
    CreateGrid(w, l, dw, dh, &box->vertexBuffer);
    u32 id = GetTextureIDByName(NewYorkName, g_Manager.texManager);

    CreateMaterial(&box->vertexBuffer, 0, box->vertexBuffer.vertexCount - 1, id);

    VECTOR pos = {0.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {.5f, .5f, .5f, 1.0f};

    SetupLTM(pos, forward, right, up,
             scales,
             1.0f, box->ltm);
    box->update_object = update_cube;

    InitOBB(box, BBO_FIXED);

    CreateGraphicsPipeline(box, "Clipper");

  //  CreateShadowMapVU1Pipeline(box, 0, DEFAULT_PIPELINE_SIZE);

    AddObjectToRenderWorld(world, box);
}

static void SetupSphere()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {0.0f, 0.0f, 0.0f, 0.0f};

    sphere = InitializeGameObject();

    DEBUGLOG("WE ARE HERE!");

    ReadModelFile("MODELS\\BODY.CBIN", &sphere->vertexBuffer);

    SetupGameObjectPrimRegs(sphere, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 1));

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, sphere->ltm);

    CreateMaterial(&sphere->vertexBuffer, 0, sphere->vertexBuffer.vertexCount - 1, 10);

    sphere->update_object = NULL;

    InitOBB(sphere, BBO_FIT);

   // CreateSphereTarget();

    AnimationData *data = GetAnimationByIndex(sphere->vertexBuffer.meshAnimationData->animations, 2);

    Animator *animator = CreateAnimator(data);

    sphere->objAnimator = animator;

    CreateGraphicsPipeline(sphere, "DOES THIS WORK?");

    AddObjectToRenderWorld(world, sphere);
}

static void SetupMultiSphere()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+50.0f, 0.0f, 0.0f, 0.0f};

    multiSphere = InitializeGameObject();
    ReadModelFile("MODELS\\MULTISPHERE.BIN", &multiSphere->vertexBuffer);
    SetupGameObjectPrimRegs(multiSphere, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0));

    VECTOR scales = {5.0f, 5.0f, 5.0f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, multiSphere->ltm);

    multiSphere->update_object = NULL;

    InitOBB(multiSphere, BBO_FIT);

    matrix_unit(lightTransform);

    CreateEnvMapPipeline(multiSphere, "ENVMAP_PIPE", VU1Stage4 | VU1Stage3, DRAW_VERTICES | DRAW_TEXTURE | DRAW_NORMAL, GetTexByName(g_Manager.texManager, glossName), lightTransform);

    AddObjectToRenderWorld(world, multiSphere);
}

static void SetupRoom()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+0.0f, +0.0f, 0.0f, 0.0f};

    room = InitializeGameObject();
    ReadModelFile("MODELS\\ROOM.BIN", &room->vertexBuffer);
    SetupGameObjectPrimRegs(room,  color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0));

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
    color_t color;

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

    SetupGameObjectPrimRegs(shadowTexView, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0));

    u32 id = 0;
#ifdef RESAMPLED

    id = resampledTexture->id;
#else
    id = shadowTexture->id;
#endif

    CreateMaterial(&shadowTexView->vertexBuffer, 0, shadowTexView->vertexBuffer.vertexCount - 1, id);

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

    //  INFOLOG("total verts in row %d", totalVerts);
}

static void SetupTessObject()
{
    color_t color;
    // CREATE_ALPHA_REGS(blender, BLEND_COLOR_DEST, BLEND_COLOR_SOURCE, BLEND_COLOR_SOURCE, BLEND_ALPHA_DEST, 0x80);
    CREATE_RGBAQ_STRUCT(color, 0xFF, 0xFF, 0xFF, 0x80, 1.0f);

    GameObject *lod_floor = InitializeGameObject();

    VECTOR tempPos2 = {0.0f, -20.0f, 0.0f, 1.0f};

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};


    SetupLTM(tempPos2, up, right, forward,
             scales,
             1.0f, lod_floor->ltm);

    SetupGameObjectPrimRegs(lod_floor, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0));

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

    SetupGameObjectPrimRegs(lod_wall, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0));

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
    InitSkybox();

  //  SetupCube();

    SetupSphere();

   //  SetupMultiSphere();

    // SetupShadowViewer();

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

    shadowTarget = CreateRenderTarget(res, res);
    shadowTexture = CreateTextureFromRenderTarget(shadowTarget, 1, TEXTURE_FUNCTION_MODULATE);

    u32 small = res >> 1;

    resampledTarget = CreateRenderTarget(small, small);
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

    GameObject *obj = box;

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

int Render()
{
    float lastTime = 0.0f;
    float lastTimeFrame = getTicks(ts);
    u32 frameCounter = 0;
    for (;;)
    {
        float currentTime = getTicks(ts);

        if (currentTime > ( lastTimeFrame + 1000.0f ))
        {
            DEBUGLOG("frame per second %d", frameCounter);
            lastTimeFrame = currentTime;
            frameCounter = 0;
        }

        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;
       // DEBUGLOG("%f %f %f", currentTime, delta, totalWhat);

        UpdatePad();

        UpdateAnimator(sphere->objAnimator, delta);


        UpdateGlossTransform();

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, g_Manager.bgkc.r, g_Manager.bgkc.g, g_Manager.bgkc.b, 0x80);

        DrawWorld(world);



        // DrawWorld(roomWorld);

        // RenderShadowScene();

        //while(PollVU1DoneProcessing(&g_Manager) < 0);

     // ReadFromVU1(vu1_data_address + (*vif1_top * 0), 16 * 4, 1);

//while(1);

        PrintText(myFont, print_out, -310, -220);

        EndRendering(cam);

        EndFrame();

        //sphere->objAnimator->currentTime += 0.1f;

        //if (sphere->objAnimator->currentTime >= 1.0f)
        //{
         //   sphere->objAnimator->currentTime = 0.0f;
        //}

        // while(1);

        UpdateLight();



       // UpdateSphereMorph();

        snprintf(print_out, 20, "DREW FLETCHER %d", no_plugin);

        no_plugin++;

        frameCounter++;
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

    prog = CreateVU1Program(&VU1_GenericBonesAnimStage1_CodeStart, &VU1_GenericBonesAnimStage1_CodeEnd, 0); // 6

    AddProgramToManager(g_Manager.vu1Manager, prog);
}

static void LoadInTextures()
{
    char _file[MAX_FILE_NAME];

    char _folder[9] = "TEXTURES\\";

    AppendString(_folder, waterName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_BMP, 0x80, 1, TEX_ADDRESS_WRAP);

    AppendString(_folder, face1Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face2Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face3Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face4Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face5Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face6Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, NewYorkName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0x80, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, glossName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0xFF, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, worldName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0xFF, 1, TEX_ADDRESS_CLAMP);

    AppendString(_folder, wallName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 0xFF, 1, TEX_ADDRESS_CLAMP);
}

int main(int argc, char **argv)
{
    InitializeSystem();

    SetupWorldObjects();

    ts = TimerZeroEnable();

    float totalTime;

    float startTime = totalTime = getTicks(ts);

    SetupVU1Programs();

    float endTime = getTicks(ts);

    DEBUGLOG("VU1 programs %f", endTime - startTime);



    startTime = getTicks(ts);

    LoadInTextures();

    endTime = getTicks(ts);

    DEBUGLOG("texes %f", endTime - startTime);

    SetupShadowRenderTarget();

    CreateLights();

    startTime = getTicks(ts);

    SetupGameObjects();

    endTime = getTicks(ts);

    DEBUGLOG("gos %f", endTime - startTime);

    SetupFont();

    endTime = getTicks(ts);

    DEBUGLOG("total %f", endTime - totalTime);

    Render();

    CleanUpGame();

    TimerZeroDisable(ts);

    SleepThread();

    return 0;
}
