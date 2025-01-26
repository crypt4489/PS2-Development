#include "ps_global.h"

#include <audsrv.h>


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "math/ps_plane.h"
#include "gs/ps_gs.h"
#include "system/ps_vif.h"
#include "util/ps_misc.h"
#include "system/ps_timer.h"
#include "textures/ps_texture.h"
#include "pad/ps_pad.h"
#include "io/ps_texture_io.h"
#include "system/ps_vumanager.h"
#include "textures/ps_font.h"
#include "gamemanager/ps_manager.h"
#include "camera/ps_camera.h"
#include "io/ps_file_io.h"
#include "io/ps_model_io.h"
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

#include <loadfile.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

Controller mainController;

char print_out[150];

MATRIX animTransform, squareTransform, lightTransform, cameraTransform;

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
const char *worldName = "WORLD2.BMP";
const char *wallName = "WALL.PNG";
const char *alphaMap = "ALPHA_MAP.PNG";
const char *digitZero = "DIGIT.PNG";
const char *digitOne = "DIGITM1.PNG";
const char *dudeer = "WORLD4BMP.BMP";
const char *wowwer = "WOW.PNG";

GameObject *grid = NULL;
GameObject *body = NULL;
GameObject *sphere = NULL;
GameObject *room = NULL;
GameObject *multiSphere = NULL;
GameObject *box = NULL;
GameObject *bodyCollision = NULL;
GameObject *shotBox = NULL;

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

float k = -1.0f;

Line mainLine = {{-45.0f, 0.0f, 50.0f, 1.0f}, {-45.0f, 0.0f, -50.0f, 1.0f}};

Line whatter = {{-65.0f, 0.0f, -25.0f, 1.0f}, {-35.0f, 0.0, -25.0f, 1.0f}};

int highlightIndex;

static BoundingSphere lol2Sphere = {{-15.0f, 10.0f, 50.0f, 1.0f}, 10.0f};

static BoundingSphere lolSphere = {{-20.0f, 15.0f, -20.0f, 1.0f}, 5.0f};

static Plane planer = {{1.0, 1.0, 0.0, 1.0f}, {1.0, 2.0, -1.0, -3.0f}};

static Plane plane2 = {{0.0, 0.0, 7.0, 1.0f}, {2.0, 3.0, -1.0, -7.0f}};

#include "math/ps_line.h"

static Color normal;
static Color boxhigh;

static Ray rayray = {{0.0f, 0.0f, -10.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};

static Ray rayray2 = {{-5.0f, 0.0f, -5.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}};

static VECTOR volLightPos = {-20.0f, 15.0f, -20.0f, 1.0f};


static void WriteOut(VECTOR v)
{
    
}
u32 clipping, outclip;
static void ClippVerts(GameObject *obj)
{
    MATRIX m;

    VECTOR scale = {2048.0f, 2048.0f, ((float)0xFFFFFF) / 32.0f, 0.0f};
    
    u32 vertCount = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount;
    VECTOR *verts = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices;
    VECTOR *texes = obj->vertexBuffer.meshData[MESHTRIANGLES]->texCoords;
    Color *color = &obj->renderState.color;
    MatrixIdentity(m);
    CreateWorldMatrixLTM(obj->ltm, m);
    MatrixMultiply(m, m, cam->viewProj);
    BeginCommand();
    u32 start = 0;
    u32 end = 35;
    LoadMaterial(obj->vertexBuffer.materials, true, &start, &end);
    DepthTest(true, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    SetRegSizeAndType(obj->renderState.gsstate.gs_reg_count, obj->renderState.gsstate.gs_reg_mask);
    PrimitiveType(GS_SET_PRIM(obj->renderState.gsstate.prim.type, obj->renderState.gsstate.prim.shading, 
                obj->renderState.gsstate.prim.mapping, obj->renderState.gsstate.prim.fogging, 
                obj->renderState.gsstate.prim.blending, obj->renderState.gsstate.prim.antialiasing, 
                obj->renderState.gsstate.prim.mapping_type, g_Manager.gs_context, 
                obj->renderState.gsstate.prim.colorfix));
   // DrawCountDirectRegList(vertCount);
    DrawCountDirectPacked(0);

    VECTOR zero;
    u32 clipping1;
    ZeroVector(zero);
    for (int i = 0; i<4; i++)
    {
         asm __volatile(
            "lqc2 $vf1, 0x00(%1)\n"
            "vclipw.xyz $vf1, $vf1\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "cfc2 %0, $vi18\n"
            : "=r"(clipping1)
            : "r"(zero)
            : "memory"
        );
    }

    //DEBUGLOG("%d", clipping1);
    int count = 0;
    for(int i = 0; i<3; i+=3)
    {
        clipping = 0;
        VECTOR v, v1, v2, st, st1, st2;
        MatrixVectorMultiply(v, m, verts[i]);
        MatrixVectorMultiply(v1, m, verts[i+1]);
        MatrixVectorMultiply(v2, m, verts[i+2]);
        //DumpVector(v);  
        asm __volatile(
            "lqc2 $vf1, 0x00(%1)\n"
            "vclipw.xyz $vf1, $vf1\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "lqc2 $vf1, 0x00(%2)\n"
            "vclipw.xyz $vf1, $vf1\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "lqc2 $vf1, 0x00(%3)\n"
            "vclipw.xyz $vf1, $vf1\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "vnop\n"
            "cfc2 %0, $vi18\n"
            : "=r"(clipping)
            : "r"(v), "r"(v1), "r"(v2)
            : "memory"
        );

        clipping &= 0x3FFFF;
        u32 judgement = 0;
        u32 comp = 0x01041;
        DEBUGLOG("%x", clipping);

         //x right plane
        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREx+");  continue;} 

        comp += comp; 
        //x left plane
        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREx-");  continue;} 

        comp += comp;

        //y bottom plane
        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREy+");  continue;}  
        //y top plane
        comp += comp;

        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREy-");  continue;} 

        comp += comp;
        //positive z near plane
        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREnear");  continue;}  
        //negative z far plane
        comp += comp;

        judgement = clipping & comp;
        if (judgement == comp) {DEBUGLOG("HEREfar");  continue;} 

        
        //judge Edge AB
        //first for +x, check if AB is on side
        comp = 0x01040;
        judgement = clipping & comp;
        if (judgement-comp == 0)
        {

        }

        // negative -x
        comp += comp;
        judgement = clipping & comp;
        if (judgement-comp == 0)
        {

        }
        //negative z
        comp += comp;
        judgement = clipping & comp;
        if (judgement-comp == 0)
        {

        }
       
        
        outclip = clipping;
        clipping += 0x7FFF;
        if (clipping >= 0x8000) clipping = 0x8000;
        //clipping = 0x7fff;
        //DEBUGLOG("%x", clipping);
        float q = 1.0f/v[3];
        VectorScaleXYZ(v, v, q);
        VectorMultiplyXYZ(v, scale, v);
        VectorAddXYZ(v, scale, v);

        float q1 = 1.0f/v1[3];
        VectorScaleXYZ(v1, v1, q1);
        VectorMultiplyXYZ(v1, scale, v1);
        VectorAddXYZ(v1, scale, v1);

        float q2 = 1.0f/v2[3];
        VectorScaleXYZ(v2, v2, q2);
        VectorMultiplyXYZ(v2, scale, v2);
        VectorAddXYZ(v2, scale, v2);

        asm __volatile(
            "lqc2 $vf1, 0x00(%0)\n"
            "lqc2 $vf2, 0x00(%1)\n"
            "lqc2 $vf3, 0x00(%2)\n"
            "vftoi4.xyz $vf1, $vf1\n"
            "vftoi4.xyz $vf2, $vf2\n"
            "vftoi4.xyz $vf3, $vf3\n"
            "sqc2 $vf1, 0x00(%0)\n"
            "sqc2 $vf2, 0x00(%1)\n"
            "sqc2 $vf3, 0x00(%2)\n"
            :
            : "r"(v), "r"(v1), "r"(v2)
            : "memory"
        );

        VectorScaleXYZ(st, texes[i], q);
         VectorScaleXYZ(st1, texes[i+1], q1);
         VectorScaleXYZ(st2, texes[i+2], q2);
       
        
        Bin2Float x1;
        x1.int_x = clipping;
    
        v[3] = x1.float_x;
        v1[3] = x1.float_x;
        v2[3] = x1.float_x;


        DrawVector(st);
        DrawColor(*color);
        DrawVector(v);
        DrawVector(st1);
        DrawColor(*color);
        DrawVector(v1);
        DrawVector(st2);
        DrawColor(*color);
        DrawVector(v2);
        //printf("%f %f %f", v[0], v[1], v[2]);
        //printf("%d %x %x %x %x\n", i, x1.int_x, x2.int_x, x3.int_x, clipping);
        count += 3;
    }
    DEBUGLOG("%d", count);
   SubmitCommand(false);
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
    // AddLightToRenderWorld(world, direct);

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

    // AddLightToRenderWorld(roomWorld, spotLight);
}

static void SetupGrid()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x10, 0x80, 0xFF, 0x80, 1.0f);

    grid = InitializeGameObject();
    SetupGameObjectPrimRegs(grid, color, RENDERNORMAL | CLIPPING);

    int dw, dl;
    float w, h;
    dw = 2;
    dl = 2;
    w = 1000;
    h = 1000;
    CreateGrid(dw, dl, w, h, &grid->vertexBuffer);
    u64 id = GetTextureIDByName(g_Manager.texManager, wowwer);

   // CreateMaterial(&grid->vertexBuffer, 0, grid->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, -15.0f, 0.0f, 1.0f};

    VECTOR scales = {.5f, .5f, .5f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, grid->ltm);

    PitchLTM(grid->ltm, 0.0f);
    grid->update_object = NULL;

    // InitOBB(grid, VBO_FIXED);

    CreateGraphicsPipeline(grid, "Clipper");

    AddObjectToRenderWorld(world, grid);
}

static void SetupBody()
{
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    VECTOR object_position = {-50.0f, 0.0f, 0.0f, 0.0f};

    body = InitializeGameObject();

    ReadModelFile("MODELS\\BODY.BIN", &body->vertexBuffer);

    SetupGameObjectPrimRegs(body, color, RENDERTEXTUREMAPPED | SKELETAL_ANIMATION);

    //VECTOR scales = {10.f, 10.f, 10.f, 1.0f};

    VECTOR scales = {.1f, .1f, .1f, 1.0f};

    SetupLTM(object_position, up, right, forward,
             scales,
             1.0f, body->ltm);

    CreateMaterial(&body->vertexBuffer, 0, body->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, GetTextureIDByName(g_Manager.texManager, wowwer));

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

    u64 id = GetTextureIDByName(g_Manager.texManager, wowwer);
   // CreateMaterial(&box->vertexBuffer, 0, 17, id);

    CreateMaterial(&box->vertexBuffer, 0, 35, GetTextureIDByName(g_Manager.texManager, worldName));
    VECTOR pos;
    CreateVector(50.0f, 0.0f, 0.0f, 1.0f, pos);

    VECTOR scales;
    CreateVector(1.0f, 1.0f, 1.0f, 1.0f, scales);

    SetupLTM(pos, up, right, forward, 
             scales,
             1.0f, box->ltm);

    CreateWorldMatrixLTM(box->ltm, box->world);         

    box->update_object = NULL;
    
    InitVBO(box, VBO_SPHERE);

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
    SetupGameObjectPrimRegs(shotBox, color, RENDERTEXTUREMAPPED);

    u64 id = GetTextureIDByName(g_Manager.texManager, worldName);

    CreateMaterial(&shotBox->vertexBuffer, 0, 35, id);
    id = GetTextureIDByName(g_Manager.texManager, wowwer);
   // CreateMaterial(&shotBox->vertexBuffer, 18, shotBox->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

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
    SetupGameObjectPrimRegs(bodyCollision, color, RENDERTEXTUREMAPPED);

    u64 id = GetTextureIDByName(g_Manager.texManager, wowwer);

    CreateMaterial(&bodyCollision->vertexBuffer, 0, bodyCollision->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount - 1, id);

    VECTOR pos = {-50.0f, 100.0f, 0.0f, 1.0f};

    VECTOR scales = {10.0f, 10.0f, 10.0f, 1.0f};

    SetupLTM(pos, up, right, forward,
             scales,
             1.0f, bodyCollision->ltm);

    bodyCollision->update_object = NULL;

    PitchLTM(bodyCollision->ltm, 90.0f);

    //InitVBO(bodyCollision, VBO_FIXED);

    CreateGraphicsPipeline(bodyCollision, "Clipper");

    AddObjectToRenderWorld(world, bodyCollision);
}

static void SetupGameObjects()
{

    // InitSkybox();
    //SetupBody();
    // SetupOBBBody();
   // SetupGrid();
   SetupAABBBox();

   
    
     
   
     
    //SetupShootBoxBox();
    // SetupShootBigBoxBox();
    //  SetupMultiSphere();

    // SetupRoom();

}

static void CleanUpGame()
{
    CleanCameraObject(cam);
    DestoryRenderWorld(world);
    DestoryRenderWorld(roomWorld);
    ClearManagerStruct(&g_Manager);
}

void DrawShadowQuad(int height, int width, int xOffset, int yOffset, u32 destTest, u32 setFrameMask, u8 alpha, u8 red, u8 green, u8 blue)
{
    bool destTestEnable = destTest;
    BeginCommand();
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    DestinationAlphaTest(destTestEnable, destTest);
    FrameBufferMaskWord(setFrameMask);
    DepthBufferMask(true);
    SetRegSizeAndType(2, DRAW_RGBAQ_REGLIST);
    PrimitiveType(GS_SET_PRIM(PRIM_TRIANGLE_STRIP, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED));
    DrawCountDirectRegList(4);
    DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, -), 0xFFFFFF));
    DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, -), 0xFFFFFF));
    DrawPairU64(GIF_SET_RGBAQ(red, green, blue, alpha, 1), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    FrameBufferMask(0, 0, 0, 0);
    DepthBufferMask(false);
    SubmitCommand(false);
}

void DrawTexturedQuad(int height, int width, Texture *tex)
{
    height >>= 1;
    width >>= 1;
    BeginCommand();
    BindTexture(tex, true);
    DepthTest(true, 1);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);

    DepthBufferMask(true);
    SetRegSizeAndType(3, DRAW_RGBAQ_UV_REGLIST);
    PrimitiveType(GS_SET_PRIM(PRIM_TRIANGLE_STRIP, DRAW_DISABLE, DRAW_ENABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED));
    DrawCountDirectRegList(4);
    
     DrawPairU64(GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 1), GIF_SET_UV(0, 0));

    DrawPairU64(GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, -), 0xFFFFFF), GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 1));

    DrawPairU64(GIF_SET_UV(0, tex->height << 4), GIF_SET_XYZ(CreateGSScreenCoordinates(width, -), CreateGSScreenCoordinates(height, +), 0xFFFFFF));

    DrawPairU64(GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 1), GIF_SET_UV(tex->width << 4, 0));

    DrawPairU64(GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, -), 0xFFFFFF), GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 1));

    DrawPairU64(GIF_SET_UV(tex->width << 4, tex->height << 4), GIF_SET_XYZ(CreateGSScreenCoordinates(width, +), CreateGSScreenCoordinates(height, +), 0xFFFFFF));
    
    SubmitCommand(false);
}


static void RenderShadowVertices(VECTOR *verts, u32 numVerts, MATRIX m)
{

    BeginCommand();
    ShaderHeaderLocation(16);
    ShaderProgram(8, 0);
    DepthTest(true, 3);
    SourceAlphaTest(ATEST_KEEP_FRAMEBUFFER, ATEST_METHOD_ALLPASS, 0xFF);
    FrameBufferMask(0xFF, 0xFF, 0xFF, 0x00);
    DepthBufferMask(true);

    AllocateShaderSpace(16, 0);
    PushMatrix(g_DrawCamera->viewProj, 0, sizeof(MATRIX));
    PushMatrix(m, 4, sizeof(MATRIX));
    PushScaleVector();
    PushColor(0, 0, 0, 0x80, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushFloats(volLightPos, 11, 12);
    PushInteger(0x3, 11, 3);
    PushMatrix(*GetPositionVectorLTM(cam->ltm), 15, sizeof(VECTOR));
    DrawUpload(numVerts);
    BindVectors(verts, numVerts, true, 1);
    StartVertexShader();
    AllocateShaderSpace(3, 9);
    PushColor(0, 0, 0, 0, 9);
    PushPairU64(GIF_SET_TAG(0, 1, 1, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_FLAT, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_UV, g_Manager.gs_context, PRIM_UNFIXED), 0, 2), DRAW_RGBAQ_REGLIST, 10);
    PushFloats(volLightPos, 11, 12);
    PushInteger(0x0, 11, 3);
    DrawUpload(numVerts);
    BindVectors(verts, numVerts, true, 1);
    StartVertexShader();
    FrameBufferMask(0x0, 0x0, 0x0, 0x0);
    DepthBufferMask(false);
    SubmitCommand(false);
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
        // VectorAddXYZ(rayray2.origin, temp, rayray2.origin);
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
        // DEBUGLOG("HERE");
        ret = RayIntersectsTriangle(&rayray2, v[0], v[1], v[2], v[3]);
    }
    if (objectIndex == 1)
    {
        BoundingBox *boxx = (BoundingBox *)shotBox->vboContainer->vbo;

        // MoveBox(boxx, temp);

        VectorAddXYZ(rayray2.origin, temp, rayray2.origin);

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
       // float time1 = getTicks(g_Manager.timer);
        ret = RayIntersectBox(&rayray2, boxx, v[3], &t);
       // DEBUGLOG("%f", getTicks(g_Manager.timer) - time1);
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

    colors[0] = &highlight;
    held = &lcolors[0];
    colors[1] = &lcolors[1];
    colors[2] = &lcolors[2];
    colors[3] = &lcolors[3];
    colors[4] = &lcolors[4];
    float lastTime = getTicks(g_Manager.timer);
    bool helpme = true;
    for (;;)
    {
        float currentTime = getTicks(g_Manager.timer);
        float delta = (currentTime - lastTime) * 0.001f;
        lastTime = currentTime;

       // TestObjects();

        UpdatePad();

        if (body) {
            UpdateAnimator(body->objAnimator, delta);
            UpdateVU1BoneMatrices(body->vertexBuffer.meshAnimationData->finalBones, 
                body->vertexBuffer.meshAnimationData->root, body->objAnimator, 
                body->vertexBuffer.meshAnimationData->joints, 
                body->vertexBuffer.meshAnimationData->jointsCount);
        }

        float time1 = getTicks(g_Manager.timer);

        StartFrame();

        ClearScreen(g_Manager.targetBack, g_Manager.gs_context, 0xFF, 0xFF, 0xFF, 0x80);

        DrawWorld(world);

       

        MATRIX ident;
        MatrixIdentity(ident);

      // RenderSphereLine(&lolSphere, *colors[3], 40);

       //  RenderPlaneLine(&plane2, *colors[1], 20);

       //RenderRay(&rayray2, *colors[2], 25);
        
      //  RenderLine(&mainLine, *colors[3]);
        
      //  RenderAABBBoxLine(shotBox->vboContainer->vbo, *colors[2], ident);
         
       // DrawShadowQuad(g_Manager.ScreenHeight, g_Manager.ScreenWidth, 0, 0, 0, 0x00FFFFFF, 0, 0, 0, 0);
    
       // RenderShadowVertices(adjs, count, m);
        
      // ClippVerts(box);

       // DrawShadowQuad(g_Manager.ScreenHeight, g_Manager.ScreenWidth, 0, 0, 1, 0xFF000000, 0, 0, 0, 0);

      // CreateWorldMatrixLTM(body->ltm, body->world);

       // RenderGameObject(body);

       extern int res;
    
        snprintf(print_out, 150, "%d", res);

       // 
       // dump_packet(g_Manager.drawBuffers->readvif, 5, 0);
       // DrawTexturedQuad(448, 640, GetTexByName(g_Manager.texManager, wowwer));

        PrintText(myFont, print_out, -310, -220, LEFT);
       // PrintOut();
     // while(true);
        StitchDrawBuffer(true);

      //  DEBUGLOG("%f", getTicks(g_Manager.timer)- time1);
      // dump_packet(g_Manager.drawBuffers->readvif, 100, 0);
      // DEBUGLOG("%d",g_Manager.drawBuffers->currentvif - g_Manager.drawBuffers->readvif );
      
        DispatchDrawBuffers();


        //ReadFromVU(vu1_data_address, 100*4, 0);

        EndRendering(cam);

        EndFrame(true);

       // ReadFromVU(vu1_data_address, 128*4, 0);

        
        //DEBUGLOG("heye");
        //while(true);

        // ReadFromVU(vu1_data_address, 100*4, true)
    
        FrameCounter++;


       // while(true);
    }

    return 0;
}
#include "system/ps_spr.h"
static void LoadInTextures()
{
    char _file[MAX_FILE_NAME];

    char _folder[11] = "TEXTURES\\";


    
    Texture *tex;

    

    AppendString(_folder, dudeer, _file, MAX_FILE_NAME);

    tex = AddAndCreateTexture(_file, READ_BMP, 1, 0xFF, TEX_ADDRESS_WRAP);

    SetFilters(tex, PS_FILTER_NNEIGHBOR);


    AppendString(_folder, worldName, _file, MAX_FILE_NAME);

    tex = AddAndCreateTexture(_file, READ_BMP, 1, 0xFF, TEX_ADDRESS_WRAP);

    SetFilters(tex, PS_FILTER_NNEIGHBOR);

    AppendString(_folder, wowwer, _file, MAX_FILE_NAME);

    tex = AddAndCreateTexture(_file, READ_PNG, 1, 0xFF, TEX_ADDRESS_CLAMP);

    SetFilters(tex, PS_FILTER_BILINEAR);
   
}



void StartUpSystem()
{
    ManagerInfo info;
    info.doublebuffered = true;
    info.drawBufferSize = 3000;
    info.fsaa = false;
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

    InitializeController(&mainController, 0, 0);

    float totalTime;

    float startTime = totalTime = getTicks(g_Manager.timer);

    float endTime = getTicks(g_Manager.timer);

    startTime = getTicks(g_Manager.timer);

     SetupFont();

    LoadInTextures();

   // 

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("texes %f", endTime - startTime);

   // CreateLights();

    

    startTime = getTicks(g_Manager.timer);

    SetupGameObjects();



    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("gos %f", endTime - startTime);

    endTime = getTicks(g_Manager.timer);

    DEBUGLOG("total %f", endTime - totalTime);

    audsrv_adpcm_t sample;

    VagFile *vag = LoadVagFile("SOUNDS\\TEST.VAG");

    audsrv_load_adpcm(&sample, vag->samples, vag->header.dataLength + 16);
    DEBUGLOG("%d %d %d %d %d", sample.pitch, sample.loop, sample.channels, sample.size, vag->header.sampleRate);
    int channel = audsrv_ch_play_adpcm(-1, &sample);
    audsrv_adpcm_set_volume(channel, 0);

    Render();

    DestroyVAGFile(vag);

    CleanUpGame();

    SleepThread();

    return 0;
}
#pragma GCC diagnostic pop
