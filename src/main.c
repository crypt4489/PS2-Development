#include "ps_global.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "math/ps_misc.h"
#include "system/ps_timer.h"
#include "textures/ps_texture.h"
#include "pad/ps_pad.h"
#include "IO/ps_texture_io.h"
#include "system/ps_vumanager.h"
#include "textures/ps_font.h"
#include "gamemanager/ps_manager.h"
#include "camera/ps_camera.h"
#include "IO/ps_file_io.h"
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
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <graph.h>
#include <stdlib.h>
#include <audsrv.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>

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

extern u32 VU1_SphereMappingStage2_CodeStart __attribute__((section(".vudata")));
extern u32 VU1_SphereMappingStage2_CodeEnd __attribute__((section(".vudata")));

TimerStruct *ts;

char print_out[35] = "DERRICK REGINALD";

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
const char *glossName = "SPHERE.PNG";
const char *worldName = "WORLD.PNG";
const char *wallName = "WALL.PNG";
const char *alphaMap = "ALPHA_MAP.PNG";

GameObject *grid = NULL;
GameObject *body = NULL;
GameObject *sphere = NULL;
GameObject *room = NULL;
GameObject *multiSphere = NULL;

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

int alpha = 0x80;

// #define RESAMPLED

int FrameCounter = 0;

MeshBuffers sphereTarget;

static void doTheThing()
{
    MATRIX screen, m, camMatrix;
    CreateWorldMatrixLTM(multiSphere->ltm, m);
    matrix_unit(screen);

    matrix_multiply(screen, screen, m);
    matrix_multiply(screen, screen, cam->view);

    matrix_copy(m, screen);

    MatrixInverse(screen, m);

    // DumpMatrix(m);
    //  CreateNormalizedTextureCoordinateMatrix(m);
    m[8] *= -1.0f;
    m[9] *= -1.0f;
    m[10] *= -1.0f;
    // m[3] = m[7] = m[11] = 0.0f;
    // DumpMatrix(m);
    MatrixTranspose(m);
    for (int i = 0; i < multiSphere->vertexBuffer.vertexCount; i++)
    {
        VECTOR incident, incidentNormal;
        MatrixVectorMultiply(incident, screen, multiSphere->vertexBuffer.vertices[i]);
        normalize(incident, incidentNormal);
        // DumpVector(incidentNormal);
        VECTOR outNormal, reflect;
        Matrix3VectorMultiply(outNormal, m, multiSphere->vertexBuffer.normals[i]);
        normalize(outNormal, outNormal);
        // DumpVector(outNormal);

        reflect[0] = outNormal[0] * incidentNormal[0];
        reflect[1] = outNormal[1] * incidentNormal[1];
        reflect[2] = outNormal[2] * incidentNormal[2];

        float dot = reflect[0] + reflect[1] + reflect[2];

        if (dot < 0.1f)
            dot = 0.0f;

        VECTOR output;
        ScaleVectorXYZ(output, outNormal, dot * 2.0f);
        VectorSubtractXYZ(incidentNormal, output, reflect);

        reflect[2] = reflect[2] + 1.0f;

        float toSqr;

        VECTOR temp;

        temp[0] = reflect[0] * reflect[0];
        temp[1] = reflect[1] * reflect[1];
        temp[2] = reflect[2] * reflect[2];

        toSqr = temp[0] + temp[1] + temp[2];

        float sqr = Sqrt(toSqr);

        float sqrB = sqr;

        sqr *= 2.0f;

        // multiSphere->vertexBuffer.texCoords[i][0] = (outNormal[0] / 2.0f) + 0.5;
        // multiSphere->vertexBuffer.texCoords[i][1] = (outNormal[1] / 2.0f) + 0.5;

        multiSphere->vertexBuffer.texCoords[i][0] = (reflect[0] / sqr) + 0.5;
        multiSphere->vertexBuffer.texCoords[i][1] = (reflect[1] / sqr) + 0.5;

        if (i > multiSphere->vertexBuffer.vertexCount - 30)
        {
            // DEBUGLOG("%f", sqr);
            //
            /*   DumpVector(multiSphere->vertexBuffer.texCoords[i]);
                DumpVector(multiSphere->vertexBuffer.normals[i]);
                DumpVector(outNormal);
                DEBUGLOG(""); */
        }
    }
}

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

static void SetupGrid()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    grid = InitializeGameObject();
    // ReadModelFile("MODELS\\BOX.BIN", &grid->vertexBuffer);
    SetupGameObjectPrimRegs(grid, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0));

    int w, l;
    float dw, dh;
    w = 25;
    l = 25;
    dw = 100;
    dh = 100;
    CreateGrid(w, l, dw, dh, &grid->vertexBuffer);
    u32 id = GetTextureIDByName(NewYorkName, g_Manager.texManager);

    CreateMaterial(&grid->vertexBuffer, 0, grid->vertexBuffer.vertexCount - 1, id);

    VECTOR pos = {0.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {.5f, .5f, .5f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, grid->ltm);

    PitchLTM(grid->ltm, -45.0f);
    grid->update_object = NULL;

    InitOBB(grid, BBO_FIXED);

    CreateGraphicsPipeline(grid, "Clipper");

    //  CreateShadowMapVU1Pipeline(box, 0, DEFAULT_PIPELINE_SIZE);

    AddObjectToRenderWorld(world, grid);
}

static void SetupBody()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {0.0f, 0.0f, 0.0f, 0.0f};

    body = InitializeGameObject();

    ReadModelFile("MODELS\\BODY.CBIN", &body->vertexBuffer);

    SetupGameObjectPrimRegs(body, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0));

    VECTOR scales = {.1f, .1f, .1f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, body->ltm);

    CreateMaterial(&body->vertexBuffer, 0, body->vertexBuffer.vertexCount - 1, 10);

    body->update_object = NULL;

    InitOBB(body, BBO_FIXED);

    // CreateSphereTarget();

    AnimationData *data = GetAnimationByIndex(body->vertexBuffer.meshAnimationData->animations, 2);

    body->objAnimator = CreateAnimator(data);

    CreateGraphicsPipeline(body, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, body);
}

static void SetupMultiSphere()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+50.0f, 0.0f, +100.0f, 0.0f};

    multiSphere = InitializeGameObject();
    ReadModelFile("MODELS\\SPHERE.BIN", &multiSphere->vertexBuffer);
    SetupGameObjectPrimRegs(multiSphere, color, RENDER_STATE(1, 1, 0, 0, 1, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    VECTOR scales = {5.0f, 5.0f, 5.0f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, multiSphere->ltm);

    multiSphere->update_object = NULL;

    PitchLTM(multiSphere->ltm, -90.0f);
    // multiSphere->vertexBuffer.vertexCount -= 240;

    CreateMaterial(&multiSphere->vertexBuffer, 0, multiSphere->vertexBuffer.vertexCount - 1, GetTextureIDByName(glossName, g_Manager.texManager));

    InitOBB(multiSphere, BBO_FIXED);

    matrix_unit(lightTransform);

    // CreateEnvMapPipeline(multiSphere, "ENVMAP_PIPE", VU1Stage4 | VU1Stage3, DRAW_VERTICES | DRAW_TEXTURE | DRAW_NORMAL, GetTexByName(g_Manager.texManager, alphaMap), lightTransform);

    CreateGraphicsPipeline(multiSphere, GEN_PIPELINE_NAME);

    // CreateAlphaMapPipeline(multiSphere, "ALPHAMAP", GetTexByName(g_Manager.texManager, alphaMap));

    AddObjectToRenderWorld(world, multiSphere);
}

static void SetupRoom()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {+0.0f, +0.0f, 50.0f, 0.0f};

    room = InitializeGameObject();
    ReadModelFile("MODELS\\ROOM.BIN", &room->vertexBuffer);
    SetupGameObjectPrimRegs(room, color, RENDER_STATE(1, 1, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));

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

    SetupGameObjectPrimRegs(shadowTexView, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));

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

    //  INFOLOG("total verts in row %d", totalVerg_Manager.timer);
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

    SetupGameObjectPrimRegs(lod_floor, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

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

    SetupGameObjectPrimRegs(lod_wall, color, RENDER_STATE(1, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

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

    SetupGrid();
    SetupBody();

    SetupMultiSphere();
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

int Render()
{
    float lastTime = getTicks(g_Manager.timer);

    for (;;)
    {
        float currentTime = getTicks(g_Manager.timer);
        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;

        UpdatePad();
        if (body)
            UpdateAnimator(body->objAnimator, delta);

        UpdateGlossTransform();

        // doTheThing();

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, g_Manager.bgkc.r, g_Manager.bgkc.g, g_Manager.bgkc.b, 0x00);

        DrawWorld(world);

        // DrawWorld(roomWorld);

        // RenderShadowScene();

        // while(PollVU1DoneProcessing(&g_Manager) < 0);

        // DumpCameraFrustum(cam);

        //  ReadFromVU1(vu1_data_address + (*vif1_tops * 0), 16 * 4, 0);

        PrintText(myFont, print_out, -310, -220);

        EndRendering(cam);

        EndFrame();

        UpdateLight();

        snprintf(print_out, 20, "DERRICK REGINALD %d", FrameCounter);

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

    prog = CreateVU1Program(&VU1_SphereMappingStage2_CodeStart, &VU1_SphereMappingStage2_CodeEnd, 0); // 8

    AddProgramToManager(g_Manager.vu1Manager, prog);
}

static void LoadInTextures()
{
    char _file[MAX_FILE_NAME];

    char _folder[9] = "TEXTURES\\";

    AppendString(_folder, waterName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_BMP, 1, 0x80, TEX_ADDRESS_WRAP);

    AppendString(_folder, face1Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face2Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face3Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face4Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face5Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, face6Name, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, NewYorkName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0x80, TEX_ADDRESS_CLAMP);

    AppendString(_folder, glossName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_WRAP);

    AppendString(_folder, worldName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP);

    AppendString(_folder, wallName, _file, MAX_FILE_NAME);

    AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP);

    AppendString(_folder, alphaMap, _file, MAX_FILE_NAME);

    AddAndCreateAlphaMap(_file, READ_PNG, TEX_ADDRESS_CLAMP);
}

int main(int argc, char **argv)
{
    InitializeSystem();

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

    VagFile *vag = LoadVagFile("SOUNDS\\MUSIC.VAG");

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

    audsrv_load_adpcm(&sample, vag->samples, vag->header.dataLength);
    DEBUGLOG("%d %d %d %d", sample.pitch, sample.loop, sample.channels, sample.size);
    // int channel = audsrv_ch_play_adpcm(-1, &sample);
    // audsrv_adpcm_set_volume(channel, MAX_VOLUME);

    Render();

    CleanUpGame();

    SleepThread();

    return 0;
}
#pragma GCC diagnostic pop