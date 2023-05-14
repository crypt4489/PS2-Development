#include "ps_animation.h"
#include "ps_misc.h"
#include "ps_quat.h"
#include "ps_log.h"
#include "ps_fast_maths.h"
#include <string.h>
#include <stdlib.h>


//static VECTOR transVec, rotVec, scaleVec;
static MATRIX final, nodeTrans;;

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

void InterpolatePosition(float animationTime, AnimationKeyHolder *keyHolder, u32 numPositions, VECTOR output)
{
    if (numPositions == 1)
    {
       // CreateTranslationMatrix(keyHolder->keys[0]->key, output);
        vector_copy(output, keyHolder->keys[0]->key);
        return;
    }

    s32 p0Index = GetKeyIndex(keyHolder->keys, numPositions, animationTime);
    s32 p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor( keyHolder->keys[p0Index]->timeStamp,
                                        keyHolder->keys[p1Index]->timeStamp,
                                        animationTime );
    VECTOR pos;
    LerpNum(keyHolder->keys[p0Index]->key, keyHolder->keys[p1Index]->key, pos, scaleFactor, 3);
    //DEBUGLOG("POSSSESES");
   // DumpVector(pos);
   // CreateTranslationMatrix(pos, output);
   vector_copy(output, pos);
}

void InterpolateRotation(float animationTime, AnimationKeyHolder *keyHolder, u32 numRotations, VECTOR output)
{
    VECTOR quat;
    if (numRotations == 1)
    {
       // QuaternionNormalize(keyHolder->keys[0]->key, quat);
       // CreateRotationMatFromQuat(keyHolder->keys[0]->key, output);
        vector_copy(output, keyHolder->keys[0]->key);
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
   // CreateRotationMatFromQuat(quat, output);
    vector_copy(output, quat);
}

void InterpolateScalings(float animationTime, AnimationKeyHolder *keyHolder, u32 numScalings, VECTOR output)
{
    if (numScalings == 1)
    {
        //vector_copy(output, keyHolder->keys[0]->key);
        //CreateScaleMatrix(keyHolder->keys[0]->key, output);
        vector_copy(output, keyHolder->keys[0]->key);
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
  //  CreateScaleMatrix(scales, output);
    vector_copy(output, scales);
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
    parent, animator->currentTime, 0);
  //  DEBUGLOG("Printing Bones!");
}

void UpdateJoint(AnimationData *data, u32 index, MATRIX transform, float animationTime)
{
    VECTOR scale, rot, trans;
    AnimationKeyHolder *holder = data->keyPositions[index];
    InterpolatePosition(animationTime, holder, holder->count, trans);

    holder = data->keyRotations[index];
    InterpolateRotation(animationTime, holder, holder->count, rot);

    holder = data->keyScalings[index];
    InterpolateScalings(animationTime, holder, holder->count, scale);

   // matrix_unit(transform);

   // matrix_multiply(transform, transform, trans);
   // matrix_multiply(transform, trans, rot);
    //matrix_multiply(transform, scale, transform);
    CreateWorldMatrixFromQuatScalesTrans(trans, rot, scale, transform);
  //  DumpMatrix(trans);
    //DEBUGLOG("------------------");

    //DumpMatrix(rot);
    //DEBUGLOG("------------------");

    //DumpMatrix(scale);
    //DEBUGLOG("------------------");

    //DumpMatrix(transform);
}

static qword_t* LoadQWordForVU1Bones(qword_t *q, u32 index, MATRIX final)
{
    VECTOR rotVec, scaleVec, transVec;
    scaleVec[3] = 0.0f;

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

   // DumpVector(&mat[0]);
    //DumpVector(&mat[4]);
   // DumpVector(&mat[8]);

    CreateQuatRotationAxes(&mat[0], &mat[4], &mat[8], rotVec);

    MATRIX temp;
    CreateWorldMatrixFromQuatScalesTrans(&final[12], rotVec, scaleVec, temp);
   // DumpMatrix(temp);

    u32 offset = 3 * index;
    qword_t *write = q + offset;
   // write = vector_to_qword(write, trans);
    //write = vector_to_qword(write, rot);
   // write = vector_to_qword(write, scale);
    memcpy(write, &final[12], sizeof(float) * 4);
    write++;
    memcpy(write, rotVec, sizeof(float) * 4);
    write++;
    memcpy(write, scaleVec, sizeof(float) * 4);

   // memcpy(write, &final[12], sizeof(float) * 4);
   // write++;
    return q;
}

void CalculateBoneTransformVU1( qword_t *q, AnimationData *data,
                                AnimationNode *node, Joint **joints, u32 numJoints,
                                MATRIX transform, float animationTime, int level)
{


   // DEBUGLOG("%s", node->name);

   // DumpMatrix(node->transformation);

   MATRIX globalTrans;

    Joint *joint = FindJointByName(joints, numJoints, node->name);


    matrix_unit(globalTrans);
    if (joint != NULL)
    {
        //DEBUGLOG("joint %s %d", joint->name, joint->id);
        UpdateJoint(data, joint->id, nodeTrans, animationTime);
       // DEBUGLOG("--?--");
       // DumpMatrix(nodeTrans);
        matrix_multiply(globalTrans, nodeTrans, transform);

        matrix_multiply(final, joint->offset, globalTrans);
       // DEBUGLOG("---&&&----");
       // DumpMatrix(joint->offset);
       // DEBUGLOG("--@--");

      //  DEBUGLOG("%d", joint->id);



        q = LoadQWordForVU1Bones(q, joint->id, final);
       // DumpMatrix(final);



    }
    else
    {
        matrix_multiply(globalTrans, node->transformation, transform);
    }


    //DEBUGLOG("--^--");
    //DumpMatrix(globalTrans);

    //matrix_copy(transform, globalTrans);
    for (int i = 0; i<node->childrenCount; i++)
        CalculateBoneTransformVU1(q, data, node->children[i], joints, numJoints, globalTrans, animationTime, level+1);
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
void DummyFunc(MeshBuffers *buffer)
{
    u32 count = buffer->vertexCount;
    static int cooked = 1;
    for (u32 i = 0; i<count; i++)
    {
        VECTOR pos;
        pos[0] = 0;
        pos[1] = 0;
        pos[2] = 0;
        pos[3] = 0;
       // float temp = buffer->vertices[i][1];
       // buffer->vertices[i][1] = buffer->vertices[i][2];
       // buffer->vertices[i][2] = -temp;

        //DumpVectorInt(buffer->bones[i]);
        for (int j = 0; j<4; j++)
        {
            u32 boneID = buffer->bones[i][j];
            if (boneID == -1)
                continue;

            VECTOR local;
           // MatrixVectorMultiply(local, globalTranss[boneID], buffer->vertices[i]);
            pos[0] += local[0] * buffer->weights[i][j];
            pos[1] += local[1] * buffer->weights[i][j];
            pos[2] += local[2] * buffer->weights[i][j];
            pos[3] += local[3] * buffer->weights[i][j];
        }
        if (i < 1000)
            DumpVector(buffer->vertices[i]);
        buffer->vertices[i][0] = pos[0];
        buffer->vertices[i][1] = pos[1];
        buffer->vertices[i][2] = pos[2];
        buffer->vertices[i][3] = pos[3];

        if (cooked && i < 1000)
        {
            DumpVector(buffer->vertices[i]);
            DEBUGLOG("%d", i);
        }
        //cooked = 0;
    }
}
