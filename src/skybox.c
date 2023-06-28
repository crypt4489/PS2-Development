#include "skybox.h"

#include "ps_pipelines.h"
#include "ps_vif.h"
#include "ps_vumanager.h"
#include "ps_manager.h"
#include "ps_movement.h"
#include "ps_gameobject.h"
#include "ps_vu1pipeline.h"
#include "ps_renderworld.h"
#include "ps_misc.h"
#include "ps_file_io.h"
#include "ps_texture.h"

extern const char *face1Name; // = "FACE1";
extern const char *face2Name; // = "FACE2";
extern const char *face3Name; // = "FACE3";
extern const char *face4Name; // = "FACE4";
extern const char *face5Name; // = "FACE5";
extern const char *face6Name;
extern const char *glossName;
extern const char *NewYorkName;

extern RenderWorld *world;

static void UpdateSkybox(GameObject *obj)
{
    SetPositionVectorLTM(obj->ltm, *GetPositionVectorLTM(world->cam->ltm));
    SetDirtyLTM(obj->ltm);
}

void InitSkybox()
{
    color_t color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    GameObject *skybox = InitializeGameObject();
    ReadModelFile("MODELS\\BOX16.CBIN", &skybox->vertexBuffer);
    VECTOR skyboxPos = {0.0f, 15.0f, -25.0f, 1.0f};
    SetupGameObjectPrimRegs(skybox, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};

    SetPositionVectorLTM(skybox->ltm, skyboxPos);
    SetRotationVectorsLTM(skybox->ltm, up, right, forward);
    SetScaleVectorLTM(skybox->ltm, scales);

    SetLastAndDirtyLTM(skybox->ltm, 1.0f);

    u32 beg = 0;
    u32 end = 95;

    CreateMaterial(&skybox->vertexBuffer, beg, end, GetTextureIDByName(face1Name, g_Manager.texManager));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(face2Name, g_Manager.texManager));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(face3Name, g_Manager.texManager));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(face4Name, g_Manager.texManager));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(face5Name, g_Manager.texManager));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(face6Name, g_Manager.texManager));

    skybox->update_object = UpdateSkybox;

    CreateGraphicsPipeline(skybox, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, skybox);
}
