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

extern u32 port;
extern u32 slot;
extern char padBuf[256];
static u32 old_pad = 0;
static u32 new_pad;
static u32 currData;

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

extern float k;

extern Color *colors[4];
extern Color highlight;
extern Color *held;

extern int objectIndex;
extern int moveX;
extern int moveY;
extern int moveZ;

void UpdatePad()
{
    s32 state = padGetState(port, 0);

    if (state == PAD_STATE_DISCONN)
    {
        ERRORLOG("Pad(%d, %d) is disconnected", port, slot);
        return;
    }

    int toggleZ = 0;

    moveX = moveY = moveZ = 0;

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
            
        }
        if (new_pad & PAD_CIRCLE)
        {
           
        }
        if (new_pad & PAD_TRIANGLE)
        {
           
        }
        if (new_pad & PAD_CROSS)
        {
       
        }

        if (new_pad & PAD_L1)
        {
           colors[objectIndex] = held;
           objectIndex = (objectIndex - 1) & 0x3;
           held = colors[objectIndex];
           colors[objectIndex] = &highlight;
        }

        if (new_pad & PAD_R1)
        {
           colors[objectIndex] = held;
           objectIndex = (objectIndex + 1) & 0x3;
           held = colors[objectIndex];
           colors[objectIndex] = &highlight;
        }

        if (currData & PAD_L2)
        {
            toggleZ = 1;
            
        }

        if (new_pad & PAD_R2)
        {
        }
        if (new_pad & PAD_DOWN)
        {
            if (toggleZ) moveZ = -1;
            else moveY = -1;
        }
        if (new_pad & PAD_UP)
        {
            if (toggleZ) moveZ = 1;
            else moveY = 1;
        }
        if (new_pad & PAD_LEFT)
        {
            moveX = -1;
        }
        if (new_pad & PAD_RIGHT)
        {
            moveX = 1;
        }
    }
}
