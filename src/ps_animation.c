#include "ps_animation.h"
#include "ps_misc.h"
#include "ps_quat.h"
#include "ps_log.h"
#include <string.h>
#include <stdlib.h>

AnimationData * GetAnimationByIndex(LinkedList *animations, u32 index)
{
    u32 indexZero = index - 1;

    LinkedList *ret = animations;

    while(indexZero > 0)
    {
        ret = ret->next;
        indexZero--;
    }

    return (AnimationData*)ret->data;
}

s32 GetKeyIndex(AnimationKey **keys, u32 numKeys, float animationTime)
{
    for (s32 i = 0; i<numKeys-1; i++)
    {
        if (animationTime < keys[i+1]->timeStamp)
        {
            return i;
        }
    }

    return -1;
}

float GetScaleFactor(float lastTS, float nextTS, float animationTime)
{
    float scaleFactor = 0.0f;
    float midway = animationTime - lastTS;
    float frameDiff = nextTS - lastTS;
    scaleFactor = midway / frameDiff;
    return scaleFactor;
}

void InterpolatePosition(float animationTime, AnimationKeyHolder *keyHolder, u32 numPositions, MATRIX output)
{
    if (numPositions == 1)
    {
        CreateTranslationMatrix(keyHolder->keys[0]->key, output);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numPositions, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor( keyHolder->keys[p0Index]->timeStamp,
                                        keyHolder->keys[p1Index]->timeStamp,
                                        animationTime );
    VECTOR pos;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, pos, scaleFactor, 3);
   // DumpVector(pos);
    CreateTranslationMatrix(pos, output);
}

void InterpolateRotation(float animationTime, AnimationKeyHolder *keyHolder, u32 numRotations, MATRIX output)
{
    VECTOR quat;
    if (numRotations == 1)
    {
       // QuaternionNormalize(keyHolder->keys[0]->key, quat);
        CreateRotationMatFromQuat(keyHolder->keys[0]->key, output);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numRotations, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor( keyHolder->keys[p0Index]->timeStamp,
                                        keyHolder->keys[p1Index]->timeStamp,
                                        animationTime );
    Slerp(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scaleFactor, quat);
   // DumpVector(quat);
    //QuaternionNormalize(quat, quat);
    CreateRotationMatFromQuat(quat, output);
}

void InterpolateScalings(float animationTime, AnimationKeyHolder *keyHolder, u32 numScalings, MATRIX output)
{
    if (numScalings == 1)
    {
        //vector_copy(output, keyHolder->keys[0]->key);
        CreateScaleMatrix(keyHolder->keys[0]->key, output);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numScalings, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor( keyHolder->keys[p0Index]->timeStamp,
                                        keyHolder->keys[p1Index]->timeStamp,
                                        animationTime );
    VECTOR scales;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scales, scaleFactor, 3);
    //DumpVector(scales);
    CreateScaleMatrix(scales, output);
}

Joint* FindJointByName(Joint **joints, u32 total, const char* name)
{
    u32 strLength = strlen(name);
    for (u32 i = 0; i<total; i++)
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
        animator->currentTime = animator->animation->ticksPerSecond * animationTime;
    }
}

void UpdateVU1BoneMatrices(qword_t *q, Animator *animator, Joint **joints, u32 numJoints)
{
    MATRIX parent;
    matrix_unit(parent);
    CalculateBoneTransformVU1(q, animator->animation, animator->animation->root, joints, numJoints, parent, 0.0279192);
}

void UpdateJoint(AnimationData *data, u32 index, MATRIX transform, float animationTime)
{
    MATRIX scale, rot, trans;
    AnimationKeyHolder *holder = data->keyPositions[index];
    InterpolatePosition(animationTime, holder, holder->count, trans);

    holder = data->keyRotations[index];
    InterpolateRotation(animationTime, holder, holder->count, rot);

    holder = data->keyScalings[index];
    InterpolateScalings(animationTime, holder, holder->count, scale);

    matrix_unit(transform);

    matrix_multiply(transform, trans, transform);
    matrix_multiply(transform, rot, transform);
    matrix_multiply(transform, scale, transform);

   // DumpMatrix(trans);
   // DEBUGLOG("------------------");

    //DumpMatrix(rot);
    //DEBUGLOG("------------------");

    //DumpMatrix(scale);
    //DEBUGLOG("------------------");

    //DumpMatrix(transform);
}

void CalculateBoneTransformVU1( qword_t *q, AnimationData *data,
                                AnimationNode *node, Joint **joints, u32 numJoints,
                                MATRIX transform, float animationTime)
{
    MATRIX nodeTrans;

    //DEBUGLOG("%s %d", node->name, numJoints);

   // DumpMatrix(node->transformation);

    Joint *joint = FindJointByName(joints, numJoints, node->name);

    if (joint != NULL)
    {
        //DEBUGLOG("joint %s %d", joint->name, joint->id);
        UpdateJoint(data, joint->id, nodeTrans, animationTime);
       // DEBUGLOG("--?--");
       // DumpMatrix(nodeTrans);

    }
    else
    {
        matrix_copy(nodeTrans, node->transformation);
    }

    MATRIX globalTrans, final;

    matrix_unit(globalTrans);

    matrix_multiply(globalTrans, nodeTrans, transform);

    if (joint != NULL)
    {
        matrix_unit(final);
        matrix_multiply(final, joint->offset, globalTrans);
       // DEBUGLOG("---&&&----");
       // DumpMatrix(joint->offset);
       // DEBUGLOG("--@--");
       // DumpMatrix(final);
       VECTOR trans, rot, scale;
        ExtractVectorFromMatrix(final, trans, rot, scale);
       // DumpVector(trans);
        //DumpVector(rot);
        //DumpVector(scale);
    }
    //DEBUGLOG("--^--");
    //DumpMatrix(globalTrans);
    for (int i = 0; i<node->childrenCount; i++)
        CalculateBoneTransformVU1(q, data, node->children[i], joints, numJoints, globalTrans, animationTime);
}

Animator *CreateAnimator(AnimationData *data)
{
    Animator *animator = (Animator*)malloc(sizeof(Animator));
    if (animator == NULL)
    {
        ERRORLOG("We cannot create animator!");
        return NULL;
    }
    animator->animation = data;
    animator->currentTime = 0.0f;
    animator->deltaTime = 0.0f;
    return animator;
}
