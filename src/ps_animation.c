#include "ps_animation.h"

#include "ps_misc.h"
#include "ps_quat.h"
#include "ps_log.h"
#include "ps_fast_maths.h"

#include <string.h>
#include <stdlib.h>

static MATRIX final, nodeTrans;
static MATRIX boneMatricesStack[256];

AnimationData *GetAnimationByIndex(LinkedList *animations, u32 index)
{
    u32 indexZero = index - 1;

    LinkedList *ret = animations;

    while (indexZero > 0)
    {
        ret = ret->next;
        indexZero--;
    }

    return (AnimationData *)ret->data;
}

static s32 GetKeyIndex(AnimationKey **keys, u32 numKeys, float animationTime)
{
    for (s32 i = 0; i < numKeys - 1; i++)
    {
        if (animationTime < keys[i + 1]->timeStamp)
        {
            return i;
        }
    }

    return -1;
}

static float GetScaleFactor(float lastTS, float nextTS, float animationTime)
{
    float midway = animationTime - lastTS;
    float frameDiff = nextTS - lastTS;
    if (frameDiff == 0.0f)
    {
        ERRORLOG("divide by zero for scalefactor");
        return 0.0f;
    }
    float scaleFactor = midway / frameDiff;
    return scaleFactor;
}

static void InterpolatePosition(float animationTime, AnimationKeyHolder *keyHolder, u32 numPositions, VECTOR output)
{
    if (numPositions == 1)
    {
        // CreateTranslationMatrix(keyHolder->keys[0]->key, output);
        vector_copy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numPositions, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    VECTOR pos;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, pos, scaleFactor, 3);
    vector_copy(output, pos);
}

static void InterpolateRotation(float animationTime, AnimationKeyHolder *keyHolder, u32 numRotations, VECTOR output)
{
    VECTOR quat;
    if (numRotations == 1)
    {
        vector_copy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numRotations, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    Slerp(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scaleFactor, quat);
    vector_copy(output, quat);
}

static void InterpolateScalings(float animationTime, AnimationKeyHolder *keyHolder, u32 numScalings, VECTOR output)
{
    if (numScalings == 1)
    {
        vector_copy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numScalings, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    VECTOR scales;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scales, scaleFactor, 3);
    vector_copy(output, scales);
}

Joint *FindJointByName(Joint **joints, u32 total, const char *name)
{
    u32 strLength = strlen(name);
    
    for (u32 i = 0; i < total; i++)
    {
        Joint *ret = joints[i];
        
        u32 jointNameLen = strlen(ret->name);

        if (jointNameLen > strLength)
        {
            continue;
        }
        
        if (strncmp(ret->name, name, strLength) == 0)
        {
            return ret;
        }
    }
    return NULL;
}

void UpdateAnimator(Animator *animator, float animationTime)
{
    animator->deltaTime = animationTime;
    if (animator->animation != NULL)
    {
        animator->currentTime += animator->animation->ticksPerSecond * animationTime;
        animator->currentTime = Mod(animator->currentTime, animator->animation->duration);
    }
}

void UpdateVU1BoneMatrices(qword_t *q, Animator *animator, Joint **joints, u32 numJoints)
{
    // DEBUGLOG("Calculating Bones!");
    MATRIX parent;
    matrix_unit(parent);

    CalculateBoneTransformVU1(q, animator->animation, animator->animation->root, joints, numJoints,
                              parent, animator->currentTime);
    //  DEBUGLOG("Printing Bones!");
}

static void UpdateJoint(AnimationData *data, u32 index, MATRIX transform, float animationTime)
{
    VECTOR scale, rot, trans;
    AnimationKeyHolder *holder = data->keyPositions[index];
    InterpolatePosition(animationTime, holder, holder->count, trans);

    holder = data->keyRotations[index];
    InterpolateRotation(animationTime, holder, holder->count, rot);

    holder = data->keyScalings[index];
    InterpolateScalings(animationTime, holder, holder->count, scale);

    CreateWorldMatrixFromQuatScalesTrans(trans, rot, scale, transform);
}

static qword_t *LoadQWordForVU1Bones(qword_t *q, u32 index, MATRIX final)
{
    VECTOR rotVec, scaleVec;

    float sx = dist(&final[0]);
    float sy = dist(&final[4]);
    float sz = dist(&final[8]);

    scaleVec[0] = sx;
    scaleVec[1] = sy;
    scaleVec[2] = sz;

    MATRIX mat;

    mat[0] = final[0] / sx;
    mat[1] = final[1] / sx;
    mat[2] = final[2] / sx;

    mat[4] = final[4] / sy;
    mat[5] = final[5] / sy;
    mat[6] = final[6] / sy;

    mat[8] = final[8] / sz;
    mat[9] = final[9] / sz;
    mat[10] = final[10] / sz;

    CreateQuatRotationAxes(&mat[0], &mat[4], &mat[8], rotVec);

    u32 offset = 3 * index;
    qword_t *write = q + offset;

    memcpy(write, &final[12], sizeof(float) * 4);
    write++;
    memcpy(write, rotVec, sizeof(float) * 4);
    write++;
    memcpy(write, scaleVec, sizeof(float) * 4);

    return q;
}

static void CalculateBoneTransformVU1(qword_t *q, AnimationData *data,
                               AnimationNode *node, Joint **joints, u32 numJoints,
                               MATRIX transform, float animationTime)
{
    MATRIX globalTrans;

    Joint *joint = FindJointByName(joints, numJoints, node->name);

    matrix_unit(globalTrans);

    if (joint != NULL)
    {
        UpdateJoint(data, joint->id, nodeTrans, animationTime);

        matrix_multiply(globalTrans, nodeTrans, transform);

        matrix_multiply(final, joint->offset, globalTrans);

        q = LoadQWordForVU1Bones(q, joint->id, final);
    }
    else
    {
        matrix_multiply(globalTrans, node->transformation, transform);
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransformVU1(q, data, node->children[i], joints, numJoints, globalTrans, animationTime);
}

static void CalculateBoneTransformVU1NoRecurse(qword_t *q, AnimationData *data,
                               AnimationNode *node, Joint **joints, u32 numJoints,
                               MATRIX transform, float animationTime)
{
    AnimationData *current = node;
    for (u32 i = 0; i<numJoints; i++)
    {    
        MATRIX globalTrans;

        Joint *joint = FindJointByName(joints, numJoints, node->name);

        matrix_unit(globalTrans);

        if (joint != NULL)
        {
            UpdateJoint(data, joint->id, nodeTrans, animationTime);

            matrix_multiply(globalTrans, nodeTrans, transform);

            matrix_multiply(final, joint->offset, globalTrans);

            q = LoadQWordForVU1Bones(q, joint->id, final);
        }
        else
        {
            matrix_multiply(globalTrans, node->transformation, transform);
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransformVU1(q, data, node->children[i], joints, numJoints, globalTrans, animationTime);
    }
}

Animator *CreateAnimator(AnimationData *data)
{
    Animator *animator = (Animator *)malloc(sizeof(Animator));
    if (animator == NULL)
    {
        ERRORLOG("We cannot create animator!");
        return NULL;
    }
    animator->animation = data;
    animator->currentTime = animator->deltaTime = 0.0f;
    return animator;
}
