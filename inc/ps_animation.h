#ifndef PS_ANIMATION_H
#define PS_ANIMATION_H
#include "ps_global.h"

AnimationData * GetAnimationByIndex(LinkedList *animations, u32 index);
s32 GetKeyIndex(AnimationKey **keys, u32 numKeys, float animationTime);
float GetScaleFactor(float lastTS, float nextTS, float animationTime);
void InterpolatePosition(float animationTime, AnimationKeyHolder *keyHolder, u32 numPositions, VECTOR output);
void InterpolateRotation(float animationTime, AnimationKeyHolder *keyHolder, u32 numRotations, VECTOR output);
void InterpolateScalings(float animationTime, AnimationKeyHolder *keyHolder, u32 numScalings, VECTOR output);
Joint* FindJointByName(Joint **joints, u32 total, const char* name);
void UpdateAnimator(Animator *animator, float animationTime);
void UpdateVU1BoneMatrices(qword_t *q, Animator *animator, Joint **joints, u32 numJoints);
void CalculateBoneTransformVU1( qword_t *q, AnimationData *data,
                                AnimationNode *node, Joint **joints, u32 numJoints,
                                MATRIX transform,
                                float animationTime, int level);
void UpdateJoint(AnimationData *data, u32 index, MATRIX transform, float animationTime);
Animator *CreateAnimator(AnimationData *data);

#endif