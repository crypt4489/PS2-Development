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

extern Controller mainController;

extern char *print_out;

extern Camera *cam;


void UpdatePad()
{
    int state = GetPadWhenReady(&mainController);

    if (state < 0)
    {
        ERRORLOG("Pad(%d, %d) is disconnected", mainController.port, mainController.slot);
        return;
    }

    state = ReadPad(&mainController);

    if (state) { ERRORLOG("Cannot read from pad %d %d", mainController.port, mainController.slot); }
   
    int moveCamera = 0;

    if (mainController.buttons.rjoy_h <= 50)
    {
        moveCamera = HandleCamMovement(cam, 4);
    }
    else if (mainController.buttons.rjoy_h >= 200)
    {
        moveCamera = HandleCamMovement(cam, 3);
    }

    if (mainController.buttons.ljoy_h <= 50)
    {
        moveCamera = HandleCamMovement(cam, 1);
    }
    else if (mainController.buttons.ljoy_h >= 200)
    {
        moveCamera = HandleCamMovement(cam, 2);
    }

    if (mainController.buttons.rjoy_v <= 50)
    {
        moveCamera = HandleCamMovement(cam, 8);
    }
    else if (mainController.buttons.rjoy_v >= 200)
    {
        moveCamera = HandleCamMovement(cam, 7);
    }

    if (mainController.buttons.ljoy_v >= 200)
    {
        moveCamera = HandleCamMovement(cam, 6);
    }
    else if (mainController.buttons.ljoy_v <= 50)
    {
        moveCamera = HandleCamMovement(cam, 5);
    }

    if (moveCamera)
    {
        UpdateCameraMatrix(cam);
    }


    u32 new_pad = mainController.buttonsToggledDown;
    u32 toggledUp = mainController.buttonsToggledUp;

    if (toggledUp & PAD_SQUARE)
    {
        DEBUGLOG("Square Toggled Up");
    }
    if (toggledUp & PAD_CIRCLE)
    {
        DEBUGLOG("Circle Toggled Up");
    }
    if (toggledUp & PAD_TRIANGLE)
    {
        DEBUGLOG("Triangle Toggled Up");
    }
    if (toggledUp & PAD_CROSS)
    {
        DEBUGLOG("Cross Toggled Up");
    }

    if (new_pad & PAD_SQUARE)
    {
        DEBUGLOG("Square Toggled Down");
    }
    if (new_pad & PAD_CIRCLE)
    {
        DEBUGLOG("Circle Toggled Down");
    }
    if (new_pad & PAD_TRIANGLE)
    {
        DEBUGLOG("Triangle Toggled Down");
    }
    if (new_pad & PAD_CROSS)
    {
        DEBUGLOG("Cross Toggled Down");
    }

    if (new_pad & PAD_L1)
    {
    
    }

    if (new_pad & PAD_R1)
    {
    
    }

    if (mainController.currentButtonsPressed & PAD_L2)
    {
        DEBUGLOG("L2 Held Down");
    }

    if (new_pad & PAD_R2)
    {

    }

    if (new_pad & PAD_DOWN)
    {
        
    }

    if (new_pad & PAD_UP)
    {
        
    }

    if (new_pad & PAD_LEFT)
    {
        
    }
    if (new_pad & PAD_RIGHT)
    {
    
    }
}
