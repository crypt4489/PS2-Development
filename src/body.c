#include "body.h"

#include "physics/ps_movement.h"
#include "gamemanager/ps_manager.h"
#include "physics/ps_vbo.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

int HandleBodyMovement(GameObject *obj, u32 type)
{
    VECTOR newPos, tempDir;
    int ret = 0;
    int collision_check = 0;
    VECTOR *right, *up, *forward;
  //  pos = GetPositionVectorLTM(obj->ltm);
    right = GetRightVectorLTM(obj->ltm);
    up = GetUpVectorLTM(obj->ltm);
    forward = GetForwardVectorLTM(obj->ltm);
    GameObject *check = NULL;
    switch (type)
    {
    case 1:
        if (obj->vboContainer->type == VBO_FIT)
        {
            VectorScaleXYZ(tempDir, *forward, -1.0f);
            collision_check = CheckCollision(obj, check, tempDir);
            if (!(collision_check))
            {
                StrafeLTM(obj->ltm, -1.0f);
                ret |= 1;
            }
        }
        else if (obj->vboContainer->type == VBO_FIXED)
        {
            StrafeLTMMove(obj->ltm, -0.5f, newPos);
            collision_check = CheckCollision(obj, check, newPos, *right, *up, *forward);
            if (!(collision_check))
            {
                SetPositionVectorLTM(obj->ltm, newPos);
                ret |= 1;
            }
        }

        break;
    case 2:
        if (obj->vboContainer->type == VBO_FIT)
        {
            collision_check = CheckCollision(obj, check, *right);
            if (!(collision_check))
            {
                StrafeLTM(obj->ltm, +1.0f);
                ret |= 1;
            }
        }
        else if (obj->vboContainer->type == VBO_FIXED)
        {
            StrafeLTMMove(obj->ltm, +0.5f, newPos);
            collision_check = CheckCollision(obj, check, newPos, *right, *up, *forward);
            if (!(collision_check))
            {
                SetPositionVectorLTM(obj->ltm, newPos);
                ret |= 1;
            }
        }
        break;
    case 3:

        RotateYLTM(obj->ltm, +0.5f);
        ret |= 2;
        break;
    case 4:

        RotateYLTM(obj->ltm, -0.5f);
        ret |= 2;
        break;
    case 5:
        if (obj->vboContainer->type == VBO_FIT)
        {
            VectorScaleXYZ(tempDir, *forward, -1.0f);
            collision_check = CheckCollision(obj, check, tempDir);
            if (!(collision_check))
            {
                WalkLTM(obj->ltm, -0.5f);
                ret |= 1;
            }
        }
        else if (obj->vboContainer->type == VBO_FIXED)
        {
            WalkLTMMove(obj->ltm, -0.5f, newPos);
            collision_check = CheckCollision(obj, check, newPos, *right, *up, *forward);
            if (!(collision_check))
            {
                SetPositionVectorLTM(obj->ltm, newPos);
                ret |= 1;
            }
        }
        break;
    case 6:
        if (obj->vboContainer->type == VBO_FIT)
        {
            collision_check = CheckCollision(obj, check, *forward);
            if (!(collision_check))
            {
                WalkLTM(obj->ltm, +0.5f);
                ret |= 1;
            }
        }
        else if (obj->vboContainer->type == VBO_FIXED)
        {
            WalkLTMMove(obj->ltm, +0.5f, newPos);
            collision_check = CheckCollision(obj, check, newPos, *right, *up, *forward);
            if (!(collision_check))
            {
                SetPositionVectorLTM(obj->ltm, newPos);
                ret |= 1;
            }
        }

        break;
    case 7:

        PitchLTM(obj->ltm, +0.5f);
        ret |= 2;
        break;
    case 8:

        PitchLTM(obj->ltm, -0.5f);
        ret |= 2;
        break;

    default:

        break;
    }
    return ret;
}
