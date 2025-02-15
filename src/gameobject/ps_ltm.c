#include "gameobject/ps_ltm.h"

#include "math/ps_matrix.h"

void SetLastAndDirtyLTM(MATRIX m, float w)
{
    SetDirtyLTM(m);
    SetLastLTM(m, w);
}

void CreateScaleMatrixLTM(MATRIX ltm, MATRIX m)
{
    MatrixIdentity(m);
    m[0] = ltm[3];
    m[5] = ltm[7];
    m[10] = ltm[11];
    m[15] = 1.0f;
}

void CreateTransScaleMatrixLTM(MATRIX ltm, MATRIX m)
{
    MATRIX work;
    MatrixIdentity(work);
    work[0] = ltm[3];
    work[5] = ltm[7];
    work[10] = ltm[11];

    work[12] = ltm[12];
    work[13] = ltm[13];
    work[14] = ltm[14];
    work[15] = GetLastLTM(ltm);

    MatrixCopy(m, work);
}

void CreateWorldMatrixLTM(MATRIX ltm, MATRIX m)
{
    MATRIX work;
    MatrixIdentity(work);
    work[0] = ltm[0] * ltm[3];
    work[5] = ltm[5] * ltm[7];
    work[10] = ltm[10] * ltm[11];


    work[1] = ltm[1] * ltm[3];
    work[2] = ltm[2] * ltm[3];
    work[3] = 0;

    work[4] = ltm[4] * ltm[7];
    work[6] = ltm[6] * ltm[7];
    work[7] = 0;

    work[8] = ltm[8] * ltm[11];
    work[9] = ltm[9] * ltm[11];
    work[11] = 0;

    work[12] = ltm[12];
    work[13] = ltm[13];
    work[14] = ltm[14];
    work[15] = GetLastLTM(ltm);

    MatrixCopy(m, work);
}

void SetupLTM(VECTOR pos, VECTOR up, VECTOR right,
              VECTOR forward, VECTOR scales,
              float q, MATRIX ltm)
{
    SetPositionVectorLTM(ltm, pos);
    SetRotationVectorsLTM(ltm, up, right, forward);
    SetScaleVectorLTM(ltm, scales);
    SetLastAndDirtyLTM(ltm, q);
}