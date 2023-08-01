#include "pad.h"

#include "camera/ps_camera.h"
#include "pad/ps_pad.h"
#include "gamemanager/ps_manager.h"
#include "body.h"
#include "pipelines/ps_vu1pipeline.h"
#include "world/ps_renderworld.h"
#include "world/ps_lights.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

char padBuf[256] __attribute__((aligned(64)));
static u32 old_pad = 0;
static u32 new_pad;
static u32 currData;
u32 port = 0;
u32 slot = 0;
struct padButtonStatus buttons;
extern char *print_out;

extern GameObject *box;
extern GameObject *sphere;

extern const char *waterName;
extern const char *NewYorkName;

extern Camera *cam;
extern RenderWorld *world;
extern LightStruct *point;
extern LightStruct *spotLight;
// extern float rad;

float localHighAngle = 90.0f;
float localLowAngle = 25.0f;
VECTOR pointpos = {+0.0f, +0.0f, 0.0f, 0.0f};
extern int alpha;

void UpdatePad()
{
    s32 state = padGetState(port, 0);

    if (state == PAD_STATE_DISCONN)
    {
        ERRORLOG("Pad(%d, %d) is disconnected", port, slot);
        return;
    }

    state = padRead(port, 0, &buttons);

    if (state != 0)
    {
        currData = 0xffff ^ buttons.btns;

        new_pad = currData & ~old_pad;
        old_pad = currData;

        int moveCamera = 0;

        if (buttons.rjoy_h <= 50)
        {
            moveCamera = HandleCamMovement(cam, 4);
        }
        else if (buttons.rjoy_h >= 200)
        {
            moveCamera = HandleCamMovement(cam, 3);
        }

        if (buttons.ljoy_h <= 50)
        {
            moveCamera = HandleCamMovement(cam, 1);
        }
        else if (buttons.ljoy_h >= 200)
        {
            moveCamera = HandleCamMovement(cam, 2);
        }

        if (buttons.rjoy_v <= 50)
        {
            moveCamera = HandleCamMovement(cam, 8);
        }
        else if (buttons.rjoy_v >= 200)
        {
            moveCamera = HandleCamMovement(cam, 7);
        }

        if (buttons.ljoy_v >= 200)
        {
            moveCamera = HandleCamMovement(cam, 6);
        }
        else if (buttons.ljoy_v <= 50)
        {
            moveCamera = HandleCamMovement(cam, 5);
        }

        if (moveCamera)
        {
            UpdateCameraMatrix(cam);
        }

        if (new_pad & PAD_SQUARE)
        {
            //  localHighAngle+=5.5f;
            // SetLightHighAngle(spotLight, localHighAngle);
            //  INFOLOG("hi %f",localHighAngle);
            pointpos[0] += 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }
        if (new_pad & PAD_CIRCLE)
        {
            //   localHighAngle-=5.5f;
            //   SetLightHighAngle(spotLight, localHighAngle);
            //  INFOLOG("hi %f",localHighAngle);
            pointpos[0] -= 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }
        if (new_pad & PAD_TRIANGLE)
        {
            //  localLowAngle+=5.5f;
            // SetLightRadius(point, localLowAngle);
            //  INFOLOG("low %f",localLowAngle);
            pointpos[1] += 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }
        if (new_pad & PAD_CROSS)
        {
            //   localLowAngle-=5.5f;
            // SetLightRadius(point, localLowAngle);
            //  INFOLOG("low %f", localLowAngle);
            pointpos[1] -= 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }

        if (new_pad & PAD_L1)
        {
            pointpos[2] += 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }

        if (new_pad & PAD_R1)
        {
            pointpos[2] -= 0.5f;
            SetPositionVectorLTM(spotLight->ltm, pointpos);
            DumpVector(pointpos);
        }

        if (new_pad & PAD_L2)
        {
            localLowAngle += 5.5f;
            SetLightRadius(spotLight, localLowAngle);
            INFOLOG("low %f", localLowAngle);
        }

        if (new_pad & PAD_R2)
        {
            localLowAngle -= 5.5f;
            SetLightRadius(spotLight, localLowAngle);
            INFOLOG("low %f", localLowAngle);
        }
        if (new_pad & PAD_DOWN)
        {
            // SetActivePipelineByName(sphere, "WIREFRAME_PIPELINE");
            // sphere->renderState.state.render_state.LIGHTING_ENABLE = 0;
            // INFOLOG("here wire?");
            AddObjectToRenderWorld(world, sphere);
        }
        if (new_pad & PAD_UP)
        {
            // SetActivePipelineByName(sphere, "LIGHTS_PIPELINE");
            // sphere->renderState.state.render_state.LIGHTING_ENABLE = 1;
            // INFOLOG("here gneeric?");
            RemoveObjectFromRenderWorld(world, sphere);
        }
        if (new_pad & PAD_LEFT)
        {
            // world = RemoveObjectFromRenderWorld(world, sphere);
            alpha++;
            INFOLOG("%d", alpha);
        }
        if (new_pad & PAD_RIGHT)
        {
            // AddObjectToRenderWorld(world, sphere);
            alpha--;
            INFOLOG("%d", alpha);
        }

        if (new_pad & PAD_R1)
        {
        }
    }
}