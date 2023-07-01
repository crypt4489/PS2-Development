#include "ps_ltm.h"

void SetLastAndDirtyLTM(MATRIX m, float w)
{
    SetDirtyLTM(m);
    SetLastLTM(m, w);
}

void CreateWorldMatrixLTM(MATRIX ltm, MATRIX m)
{
    MATRIX work;
    matrix_unit(work);
    work[0] = ltm[0] * ltm[3];
    work[5] = ltm[5] * ltm[7];
    work[10] = ltm[10] * ltm[11];

    work[1] = ltm[1] * ltm[3];
    work[2] = ltm[2] * ltm[3];

    work[4] = ltm[4] * ltm[7];
    work[6] = ltm[6] * ltm[7];

    work[8] = ltm[8] * ltm[11];
    work[9] = ltm[9] * ltm[11];

    work[12] = ltm[12];
    work[13] = ltm[13];
    work[14] = ltm[14];
    work[15] = GetLastLTM(ltm);

    matrix_copy(m, work);
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