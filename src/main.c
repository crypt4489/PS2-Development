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

#include <draw2d.h>
#include <draw3d.h>

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
const char *worldName = "WORLD.BMP";
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

int alpha = 0x80;

// #define RESAMPLED

int FrameCounter = 0;

MeshBuffers sphereTarget;

float k = -1.0f;

Line mainLine = {{-45.0f, 0.0f, 50.0f, 1.0f}, {-45.0f, 0.0f, -50.0f, 1.0f}};

Line whatter = {{-65.0f, 0.0f, -25.0f, 1.0f}, {-35.0f, 0.0, -25.0f, 1.0f}};

int highlightIndex;

static BoundingSphere lol2Sphere = {{-15.0f, 10.0f, 50.0f, 1.0f}, 10.0f};

static BoundingSphere lolSphere = {{0.0, 10.0f, 50.0f, 1.0f}, 5.0f};


static Plane planer = {{1.0, 1.0, 0.0, 1.0f}, {1.0, 2.0, -1.0, -3.0f}};

static Plane plane2 = {{0.0, 0.0, 7.0, 1.0f}, {2.0, 3.0, -1.0, -7.0f}};


#include "math/ps_line.h"

static Color shotBoxColor[36];
static Color normal;
static Color boxhigh;
static void ShotBoxIntersectCB(VECTOR *verts, int index)
{
    for (int i = index+2; i>=index; i--)
    {
        shotBoxColor[i].r = boxhigh.r;
        shotBoxColor[i].g = boxhigh.g;
        shotBoxColor[i].b = boxhigh.b;
        shotBoxColor[i].a = boxhigh.a;
    }
}

static Ray rayray = {{0.0f, 0.0f, -10.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};

static Ray rayray2 = {{-5.0f, 0.0f, -5.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}};

static void UpdateGlossTransform()
{
    CreateRotationAndCopyMatFromObjAxes(lightTransform, *GetUpVectorLTM(direct->ltm), *GetForwardVectorLTM(direct->ltm), *GetRightVectorLTM(direct->ltm));

    MatrixTranspose(lightTransform);

    CreateNormalizedTextureCoordinateMatrix(lightTransform);
}

static void SetK()
{

    static int mag = LOD_MAG_LINEAR;

    SetupTexLODStruct(zero, k, 0, 2, 5, mag);
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

    InitCameraVBOContainer(cam, 10.0f, 10.0f, 10.0f, VBO_FIT);

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
    SetupGameObjectPrimRegs(grid, color, RENDERTEXTUREMAPPED | CLIPPING);

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

    CreateMaterial(&body->vertexBuffer, 0, body->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, GetTextureIDByName(worldName, g_Manager.texManager));

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
    SetupGameObjectPrimRegs(box, color, RENDERTEXTUREMAPPED | CLIPPING);

    u32 id = GetTextureIDByName(worldName, g_Manager.texManager);

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

static void SetupShootBoxBox()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    shotBox = InitializeGameObject();
    ReadModelFile("MODELS\\BOX.BIN", &shotBox->vertexBuffer);
    SetupGameObjectPrimRegs(shotBox, color, RENDERCOLORED);

    u32 id = GetTextureIDByName(worldName, g_Manager.texManager);

    CreateMaterial(&shotBox->vertexBuffer, 0, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, 0.0f, 0.0f, 1.0f};

    VECTOR scales = {1.f, 1.f, 1.f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, shotBox->ltm);

    shotBox->update_object = NULL;

    InitVBO(shotBox, VBO_FIT);

}

static void SetupOBBBody()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    bodyCollision = InitializeGameObject();
    ReadModelFile("MODELS\\BODY.BIN", &bodyCollision->vertexBuffer);
    SetupGameObjectPrimRegs(bodyCollision, color, RENDERTEXTUREMAPPED | CLIPPING);

    u32 id = GetTextureIDByName(worldName, g_Manager.texManager);

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
    SetupBody();
    SetupAABBBox();
    SetupOBBBody();
    SetupShootBoxBox();
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
    CreateVector(moveX * 1.25, moveY * 1.25, moveZ *1.25, 1.0f, temp);

    if (objectIndex == 0)
    {
        VectorAddXYZ(mainLine.p1, temp, mainLine.p1);
        VectorAddXYZ(mainLine.p2, temp, mainLine.p2);
        VectorAddXYZ(rayray2.origin, temp, rayray2.origin);
       // ret = LineIntersectLine(&whatter, &mainLine, temp);
       // ret = RayIntersectRay(&rayray, &rayray2);
        //DumpVector(temp);
      /* ret = RayIntersectPlane(&rayray, &plane2, temp);
       float tmep;
       DEBUGLOG("%d %d", RayIntersectSphere(&rayray, &lolSphere, temp),
        RayIntersectBox(&rayray, box->vboContainer->vbo, temp, &tmep)); */
       // VectorAddXYZ(*GetPositionVectorLTM(shotBox->ltm), temp, *GetPositionVectorLTM(shotBox->ltm));
         MATRIX m;
        CreateWorldMatrixLTM(shotBox->ltm, m);
        VECTOR v[4];
        for(int i = 0; i<3; i++)
        {
            MatrixVectorMultiply(v[i], m, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
        }

        ret = RayIntersectsTriangle(&rayray2, v[0], v[1], v[2], v[3]);

    }
    if (objectIndex == 1)
    {
        BoundingBox *boxx = (BoundingBox*)box->vboContainer->vbo;
        
        MoveBox(boxx, temp);    
        
      //  ret = LineSegmentIntersectBox(&mainLine, boxx, temp);

       // DEBUGLOG("%f", Sqrt(SqrDistFromAABB(lolSphere.center, boxx)));

        //DEBUGLOG("wowowowo %d", PlaneAABBCollision(&planer, boxx));

      MATRIX m;
        CreateWorldMatrixLTM(shotBox->ltm, m);
        VECTOR v[4];
        for(int i = 0; i<3; i++)
        {
            MatrixVectorMultiply(v[i], m, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
        }

        ret = AABBIntersectTriangle(v[0], v[1], v[2], boxx);
    } 
    else if (objectIndex == 3)
    {

        BoundingSphere *sph = &lolSphere;
        VectorAddXYZ(sph->center, temp, sph->center);

        ret = LineSegmentIntersectSphere(&mainLine, sph, temp);

        BoundingBox boxx;
        BoundingBox *boxx2 = (BoundingBox*)bodyCollision->vboContainer->vbo;
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

    for (int i = 0; i<3; i++)
    {
        CREATE_RGBAQ_STRUCT(shotBoxColor[i], 128, 255, 255, 128, 0);
    }

    for (int i = 3; i<36; i++)
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

    for (;;)
    {
        float currentTime = getTicks(g_Manager.timer);
        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;

        TestObjects();

        UpdatePad();

        if (body != NULL)
            UpdateAnimator(body->objAnimator, delta);

        float time1 = getTicks(g_Manager.timer);

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, g_Manager.bgkc.r, g_Manager.bgkc.g, g_Manager.bgkc.b, 0x80);

        //   DrawWorld(world);

        RenderLine(&mainLine, *colors[0]);

        MATRIX m;

        MatrixIdentity(m);

        RenderAABBBoxLine(box->vboContainer->vbo, *colors[1], m);

        CreateWorldMatrixLTM(bodyCollision->ltm, m);

        RenderAABBBoxLine(bodyCollision->vboContainer->vbo, *colors[2], m);

        RenderSphereLine(&lolSphere, *colors[3], 40);

        RenderSphereLine(&lol2Sphere, *colors[4], 40);

        RenderPlaneLine(&planer, *colors[4], 20);

        RenderPlaneLine(&plane2, *colors[1], 20);

        CreateWorldMatrixLTM(shotBox->ltm, m);

       // LineSegmentIntersectForAllTriangles(&mainLine,
        // shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertices,
        //  shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount,
        // m, ShotBoxIntersectCB);

        RenderGameObject(shotBox, shotBoxColor);

        RenderRay(&rayray, *colors[0], 50.0);

        RenderLine(&whatter, *colors[0]);

        RenderRay(&rayray2, *colors[1], 50.0);

        snprintf(print_out, 35, "DERRICK REGINALD %d", FrameCounter);

        PrintText(myFont, print_out, -310, -220);

        EndRendering(cam);

        EndFrame(1);

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

    Texture *tex = AddAndCreateTexture(_file, READ_BMP, 1, 0xFF, TEX_ADDRESS_CLAMP, 0);

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

    CreateLights();

    startTime = getTicks(g_Manager.timer);

    SetupGameObjects();

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("gos %f", endTime - startTime);

    SetupFont();

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

    NormalizePlane(planer.planeEquation, planer.planeEquation);

    Render();

    CleanUpGame();

    SleepThread();

    return 0;
}
#pragma GCC diagnostic pop
