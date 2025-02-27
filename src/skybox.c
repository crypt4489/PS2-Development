#include "skybox.h"

#include "pipelines/ps_pipelines.h"
#include "system/ps_vif.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"
#include "physics/ps_movement.h"
#include "gameobject/ps_gameobject.h"
#include "pipelines/ps_vu1pipeline.h"
#include "world/ps_renderworld.h"
#include "io/ps_file_io.h"
#include "io/ps_model_io.h"
#include "textures/ps_texture.h"
#include "textures/ps_texturemanager.h"
#include "math/ps_matrix.h"
#include "util/ps_misc.h"

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
    Color color;

    CREATE_RGBAQ_STRUCT(color, 0x80, 0x80, 0x80, 0x80, 1.0f);

    GameObject *skybox = InitializeGameObject();
    ReadModelFile("MODELS\\BOX16.CBIN", &skybox->vertexBuffer);
    VECTOR skyboxPos = {0.0f, 15.0f, -25.0f, 1.0f};
    SetupGameObjectPrimRegs(skybox, color, RENDER_STATE(1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    VECTOR scales = {1.0f, 1.0f, 1.0f, 1.0f};

    SetPositionVectorLTM(skybox->ltm, skyboxPos);
    SetRotationVectorsLTM(skybox->ltm, up, right, forward);
    SetScaleVectorLTM(skybox->ltm, scales);

    SetLastAndDirtyLTM(skybox->ltm, 1.0f);

    u32 beg = 0;
    u32 end = 95;

    CreateMaterial(&skybox->vertexBuffer, beg, end, GetTextureIDByName(g_Manager.texManager, face1Name));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(g_Manager.texManager, face2Name));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(g_Manager.texManager, face3Name));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(g_Manager.texManager, face4Name));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(g_Manager.texManager, face5Name));

    CreateMaterial(&skybox->vertexBuffer, beg += 96, end += 96, GetTextureIDByName(g_Manager.texManager, face6Name));

    skybox->update_object = UpdateSkybox;

    CreateGraphicsPipeline(skybox, GEN_PIPELINE_NAME);

    AddObjectToRenderWorld(world, skybox);
}
