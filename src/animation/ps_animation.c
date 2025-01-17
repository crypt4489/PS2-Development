#include "animation/ps_animation.h"

#include <string.h>
#include <stdlib.h>

#include "math/ps_quat.h"
#include "log/ps_log.h"
#include "math/ps_fast_maths.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "util/ps_linkedlist.h"

static MATRIX final, nodeTrans;
static MATRIX boneMatricesStack[256];
typedef struct anim_stack_node_t
{
    AnimationNode *data;
    struct anim_stack_node_t *next;
    u32 parentMatIndex;
} AnimStackNode;
static AnimStackNode nodes[256];


static void LoadQWordForVU1Bones(VECTOR *verts, u32 index, MATRIX final)
{
    VECTOR rotVec, scaleVec, trans;

    ExtractVectorFromMatrix(trans, rotVec, scaleVec, final);

    u32 offset = 3 * index;
    VECTOR *write = verts + offset;

    memcpy(*write, trans, sizeof(float) * 4);
    write++;
    memcpy(*write, rotVec, sizeof(float) * 4);
    write++;
    memcpy(*write, scaleVec, sizeof(float) * 4);
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
        VectorCopy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numPositions, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    VECTOR pos;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, pos, scaleFactor, 3);
    VectorCopy(output, pos);
}

static void InterpolateRotation(float animationTime, AnimationKeyHolder *keyHolder, u32 numRotations, VECTOR output)
{
    VECTOR quat;
    if (numRotations == 1)
    {
        VectorCopy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numRotations, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    Slerp(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scaleFactor, quat);
    VectorCopy(output, quat);
}

static void InterpolateScalings(float animationTime, AnimationKeyHolder *keyHolder, u32 numScalings, VECTOR output)
{
    if (numScalings == 1)
    {
        VectorCopy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numScalings, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyHolder->keys[p0Index]->timeStamp,
                                       keyHolder->keys[p1Index]->timeStamp,
                                       animationTime);
    VECTOR scales;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, scales, scaleFactor, 3);
    VectorCopy(output, scales);
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

static void CalculateBoneTransformVU1(VECTOR *vecs, AnimationData *data,
                                               AnimationNode *node, Joint **joints, u32 numJoints,
                                               float animationTime)
{
    u32 stackptr = 0;
    AnimStackNode *current = &nodes[stackptr++];
    current->data = node;
    current->next = NULL;
    current->parentMatIndex = 0;
    u32 currentBoneStack = 1;
    AnimStackNode *parent = current;
    u32 count = 0;
    while (current && count < 256)
    {

        MATRIX globalTrans;

        Joint *joint = FindJointByName(joints, numJoints, current->data->name);

        MatrixIdentity(globalTrans);

        if (joint)
        {
            UpdateJoint(data, joint->id, nodeTrans, animationTime);

            MatrixMultiply(globalTrans, nodeTrans, boneMatricesStack[current->parentMatIndex]);

            MatrixMultiply(final, joint->offset, globalTrans);

            LoadQWordForVU1Bones(vecs, joint->id, final);
        }
        else
        {
            MatrixMultiply(globalTrans, current->data->transformation, boneMatricesStack[current->parentMatIndex]);
        }

        MatrixCopy(boneMatricesStack[currentBoneStack], globalTrans);

        for (u32 i = 0; i < current->data->childrenCount; i++)
        {
            AnimStackNode *child = &nodes[stackptr++];
            child->data = &node[current->data->children[i]];
            child->parentMatIndex = currentBoneStack;
            child->next = NULL;
            parent->next = child;
            parent = child;
        }

        current = current->next;
        currentBoneStack++;
        count++;
    }
}

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



Joint *FindJointByName(Joint **joints, u32 total, const char *name)
{
    u32 strLength = strnlen(name, MAX_JOINT_NAME);

    for (u32 i = 0; i < total; i++)
    {
        Joint *ret = joints[i];

        u32 jointNameLen = strnlen(ret->name, MAX_JOINT_NAME);

        if (jointNameLen > strLength)
        {
            continue;
        }

        if (!strncmp(ret->name, name, strLength))
        {
            return ret;
        }
    }
    return NULL;
}

void UpdateAnimator(Animator *animator, float animationTime)
{
    animator->deltaTime = animationTime;
    if (animator->animation)
    {
        animator->currentTime += animator->animation->ticksPerSecond * animationTime;
        animator->currentTime = Mod(animator->currentTime, animator->animation->duration);
    }
}


void UpdateVU1BoneMatrices(VECTOR *verts, AnimationNode *root, Animator *animator, Joint **joints, u32 numJoints)
{
     //DEBUGLOG("Calculating Bones!");
    MatrixIdentity(boneMatricesStack[0]);

    CalculateBoneTransformVU1(verts, animator->animation, root, joints, numJoints,
                               animator->currentTime);
}

Animator *CreateAnimator(AnimationData *data)
{
    Animator *animator = (Animator *)malloc(sizeof(Animator));
    if (!animator)
    {
        ERRORLOG("We cannot create animator!");
        return animator;
    }
    animator->animation = data;
    animator->currentTime = animator->deltaTime = 0.0f;
    return animator;
}
