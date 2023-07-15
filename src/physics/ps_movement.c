#include "physics/ps_movement.h"

#include "math/ps_misc.h"
#include "math/ps_fast_maths.h"

void StrafeLTMMove(MATRIX ltm, float d, VECTOR newPos)
{

    VECTOR *right, *pos;

    right = GetRightVectorLTM(ltm);
    pos = GetPositionVectorLTM(ltm);

    VECTOR tempOut;

    tempOut[0] = d * (*right)[0] + (*pos)[0];
    tempOut[1] = d * (*right)[1] + (*pos)[1];
    tempOut[2] = d * (*right)[2] + (*pos)[2];
    tempOut[3] = 1.0f;

    vector_copy(newPos, tempOut);
}

void WalkLTMMove(MATRIX ltm, float d, VECTOR newPos)
{
    VECTOR *forward, *pos;

    forward = GetForwardVectorLTM(ltm);
    pos = GetPositionVectorLTM(ltm);

    VECTOR tempOut;

    tempOut[0] = d * (*forward)[0] + (*pos)[0];
    tempOut[1] = d * (*forward)[1] + (*pos)[1];
    tempOut[2] = d * (*forward)[2] + (*pos)[2];
    tempOut[3] = 1.0f;

    vector_copy(newPos, tempOut);
}

void StrafeLTM(MATRIX ltm, float d)
{

    VECTOR *right, *pos;

    right = GetRightVectorLTM(ltm);
    pos = GetPositionVectorLTM(ltm);

    VECTOR tempOut;

    tempOut[0] = d * (*right)[0] + (*pos)[0];
    tempOut[1] = d * (*right)[1] + (*pos)[1];
    tempOut[2] = d * (*right)[2] + (*pos)[2];

    SetPositionVectorLTM(ltm, tempOut);
}

void WalkLTM(MATRIX ltm, float d)
{
    VECTOR *forward, *pos;

    forward = GetForwardVectorLTM(ltm);
    pos = GetPositionVectorLTM(ltm);

    VECTOR tempOut;

    tempOut[0] = d * (*forward)[0] + (*pos)[0];
    tempOut[1] = d * (*forward)[1] + (*pos)[1];
    tempOut[2] = d * (*forward)[2] + (*pos)[2];

    SetPositionVectorLTM(ltm, tempOut);
}

void RotateYLTM(MATRIX ltm, float angle)
{
    VECTOR *upL, *forward, *right;

    upL = GetUpVectorLTM(ltm);
    forward = GetForwardVectorLTM(ltm);
    right = GetRightVectorLTM(ltm);

    MATRIX rotation;

    VECTOR upY = {0.0f, 1.0f, 0.0f, 0.0f};

    matrix_unit(rotation);

    CreateRotationMatrix(upY, DegToRad(angle), rotation);

    Matrix3VectorMultiply(*forward, rotation, *forward);

    Matrix3VectorMultiply(*upL, rotation, *upL);

    Matrix3VectorMultiply(*right, rotation, *right);

    normalize(*forward, *forward);
}

void PitchLTM(MATRIX ltm, float angle)
{
    VECTOR *up, *forward, *right;

    right = GetRightVectorLTM(ltm);
    up = GetUpVectorLTM(ltm);
    forward = GetForwardVectorLTM(ltm);

    MATRIX rotation;

    matrix_unit(rotation);

    CreateRotationMatrix(*right, DegToRad(angle), rotation);

    Matrix3VectorMultiply(*forward, rotation, *forward);

    Matrix3VectorMultiply(*up, rotation, *up);

    normalize(*forward, *forward);
}