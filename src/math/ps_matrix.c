#include "math/ps_matrix.h"

#include "math/ps_fast_maths.h"
#include "math/ps_quat.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"
#include "math/ps_plane.h"

void MatrixIdentity(MATRIX m)
{
    asm __volatile__(
        "vmr32 $vf1, $vf0\n"
        "vmr32 $vf2, $vf1\n"
        "vmr32 $vf3, $vf2\n"
        "sqc2 $vf3, 0x00(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf1, 0x20(%0)\n"
        "sqc2 $vf0, 0x30(%0)\n"
        :
        : "r"(m)
        : "memory");
}

void MatrixCopy(MATRIX dest, MATRIX in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "sqc2 $vf1, 0x00(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        :
        : "r"(dest), "r"(in)
        : "memory");
}

void MatrixVoidCopy(void *dest, void *in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "sqc2 $vf1, 0x00(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        :
        : "r"(dest), "r"(in)
        : "memory");
}

void MatrixMultiply(MATRIX out, MATRIX in1, MATRIX in2)
{
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "lqc2		$vf2, 0x10(%1)	\n"
        "lqc2		$vf3, 0x20(%1)	\n"
        "lqc2		$vf4, 0x30(%1)	\n"
        "lqc2		$vf5, 0x00(%2)	\n"
        "lqc2		$vf6, 0x10(%2)	\n"
        "lqc2		$vf7, 0x20(%2)	\n"
        "lqc2		$vf8, 0x30(%2)	\n"
        "vmulax.xyzw		$ACC, $vf5, $vf1\n"
        "vmadday.xyzw	$ACC, $vf6, $vf1\n"
        "vmaddaz.xyzw	$ACC, $vf7, $vf1\n"
        "vmaddw.xyzw		$vf1, $vf8, $vf1\n"
        "vmulax.xyzw		$ACC, $vf5, $vf2\n"
        "vmadday.xyzw	$ACC, $vf6, $vf2\n"
        "vmaddaz.xyzw	$ACC, $vf7, $vf2\n"
        "vmaddw.xyzw		$vf2, $vf8, $vf2\n"
        "vmulax.xyzw		$ACC, $vf5, $vf3\n"
        "vmadday.xyzw	$ACC, $vf6, $vf3\n"
        "vmaddaz.xyzw	$ACC, $vf7, $vf3\n"
        "vmaddw.xyzw		$vf3, $vf8, $vf3\n"
        "vmulax.xyzw		$ACC, $vf5, $vf4\n"
        "vmadday.xyzw	$ACC, $vf6, $vf4\n"
        "vmaddaz.xyzw	$ACC, $vf7, $vf4\n"
        "vmaddw.xyzw		$vf4, $vf8, $vf4\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        "sqc2		$vf2, 0x10(%0)	\n"
        "sqc2		$vf3, 0x20(%0)	\n"
        "sqc2		$vf4, 0x30(%0)	\n"
        :
        : "r"(out), "r"(in1), "r"(in2)
        : "memory");
}

void CreateRotationAndCopyMatFromObjAxes(MATRIX out, VECTOR up, VECTOR forward, VECTOR right)
{
    MATRIX temp_out;
    MatrixIdentity(temp_out);

    temp_out[0] = right[0];
    temp_out[1] = right[1];
    temp_out[2] = right[2];

    temp_out[4] = up[0];
    temp_out[5] = up[1];
    temp_out[6] = up[2];

    temp_out[8] = forward[0];
    temp_out[9] = forward[1];
    temp_out[10] = forward[2];

    temp_out[12] = 0.0f;
    temp_out[13] = 0.0f;
    temp_out[14] = 0.0f;
    temp_out[15] = 1.0f;

    MatrixCopy(out, temp_out);
}

void MatrixVectorMultiply(VECTOR out, MATRIX m, VECTOR in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x10(%1)\n"
        "lqc2 $vf4, 0x20(%1)\n"
        "lqc2 $vf5, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1\n"
        "vmadday.xyzw $ACC, $vf3, $vf1\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1\n"
        "vmaddw.xyzw $vf1, $vf5, $vf1\n"
        "sqc2 $vf1, 0x00(%2)\n"
        :
        : "r"(in), "r"(m), "r"(out)
        : "memory");
}

void Matrix3VectorMultiply(VECTOR out, MATRIX m, VECTOR in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x10(%1)\n"
        "lqc2 $vf4, 0x20(%1)\n"
        "lqc2 $vf5, 0x00(%2)\n"
        "vmulax.xyz $ACC, $vf2, $vf1\n"
        "vmadday.xyz $ACC, $vf3, $vf1\n"
        "vmaddz.xyz $vf5, $vf4, $vf1\n"
        "sqc2 $vf5, 0x00(%2)\n"
        :
        : "r"(in), "r"(m), "r"(out)
        : "memory");
}

void MatrixVectorTransform(VECTOR out, MATRIX m, VECTOR in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x10(%1)\n"
        "lqc2 $vf4, 0x20(%1)\n"
        "lqc2 $vf5, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1\n"
        "vmadday.xyzw $ACC, $vf3, $vf1\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1\n"
        "vmaddw.xyzw $vf1, $vf5, $vf1\n"
        "vdiv $Q, $vf0w, $vf1w\n"
        "vwaitq \n"
        "vmulq.xyz $vf1, $vf1, $Q\n"
        "sqc2 $vf1, 0x00(%2)\n"
        :
        : "r"(in), "r"(m), "r"(out)
        : "memory");
}

void CreateProjectionMatrix(MATRIX output, float width, float height, float aspect, float near, float far, float angle)
{

    MATRIX temp;
    float n = near;                          // Near plane
    float f = far;                           // Far plane
    float FovYdiv2 = DegToRad(angle) * 0.5f; // 60 degree FOV
    // float fAspect = 4.0f / 3.0f;			 // Aspect ratio
    float cotFOV = 1.0f / (Sin(FovYdiv2) / Cos(FovYdiv2));
    // We will be projecting to the 4096 wide drawing area, but we only want
    // the projection matrix to cover the visible area
    float w = cotFOV * (width / 4096.0f) / aspect;
    float h = cotFOV * (height / 4096.0f);

    MatrixIdentity(temp);

    temp[0] = w;
    temp[5] = -h;
    temp[10] = (f + n) / (f - n);
    temp[11] = -1;
    temp[14] = (2 * f * n) / (f - n);
    temp[15] = 0;

    MatrixCopy(output, temp);
}

void CreateNormalizedTextureCoordinateMatrix(MATRIX src)
{
    src[0] *= 0.5;
    src[1] *= 0.5;
    src[2] *= 0.5;

    src[4] *= 0.5;
    src[5] *= 0.5;
    src[6] *= 0.5;

    src[8] *= 0.5;
    src[9] *= 0.5;
    src[10] *= 0.5;

    src[12] = 0.5f;
    src[13] = 0.5f;
    src[14] = 0.0f;
    src[15] = 1.0f;
}

void CreateOrthoGraphicMatrix(float xLow, float xHigh, float yLow, float yHigh, float near, float far, MATRIX out)
{
    MATRIX temp;
    MatrixIdentity(temp);

    temp[0] = 2.0f / (yHigh - xLow);
    temp[3] = -(yHigh + xLow) / (yHigh - xLow);
    temp[7] = -(xHigh + yLow) / (xHigh - yLow);
    temp[11] = -(far + near) / (far - near);
    temp[5] = 2.0f / (xHigh - yLow);
    temp[10] = -2.0f / (far - near);

    MatrixCopy(out, temp);
}

void CreateRotationMatrix(VECTOR axis, float angle, MATRIX output)
{
    MATRIX work;

    float s = Sin(angle);
    float c = Cos(angle);

    VECTOR axisNormalized;

    Normalize(axis, axisNormalized);

    float x = axisNormalized[0];
    float y = axisNormalized[1];
    float z = axisNormalized[2];

    MatrixIdentity(work);

    work[0] = c + (x * x) * (1 - c);
    work[1] = x * y * (1 - c) - (z * s);
    work[2] = x * z * (1 - c) + (y * s);
    work[4] = y * x * (1 - c) + (z * s);
    work[5] = c + (y * y) * (1 - c);
    work[6] = y * x * (1 - c) - (x * s);
    work[8] = z * x * (1 - c) - (y * s);
    work[9] = x * y * (1 - c) + (x * s);
    work[10] = c + (z * z) * (1 - c);

    work[15] = 1.0f;

    MatrixCopy(output, work);
}

void CreateTranslationMatrix(VECTOR pos, MATRIX output)
{
    MATRIX work;
    MatrixIdentity(work);

    work[12] = pos[0];
    work[13] = pos[1];
    work[14] = pos[2];

    MatrixCopy(output, work);
}
void CreateScaleMatrix(VECTOR scales, MATRIX output)
{
    MATRIX work;
    MatrixIdentity(work);

    work[0] = scales[0];
    work[5] = scales[1];
    work[10] = scales[2];
    work[15] = 1.0f;

    MatrixCopy(output, work);
}

void CreateWorldMatrix(MATRIX output, MATRIX scales, MATRIX rot, MATRIX trans)
{
    MATRIX work;
    MatrixIdentity(work);

    MatrixMultiply(work, work, scales);

    MatrixMultiply(work, work, rot);

    MatrixMultiply(work, work, trans);

    MatrixCopy(output, work);
}

void DumpMatrix(MATRIX elem)
{
    printf("%f %f %f %f\n", elem[0], elem[1], elem[2], elem[3]);
    printf("%f %f %f %f\n", elem[4], elem[5], elem[6], elem[7]);
    printf("%f %f %f %f\n", elem[8], elem[9], elem[10], elem[11]);
    printf("%f %f %f %f\n\n", elem[12], elem[13], elem[14], elem[15]);
}

void MatrixInverse(MATRIX src, MATRIX out)
{
    MATRIX inv;
    MatrixIdentity(inv);

    float s0 = src[0] * src[5] - src[4] * src[1];
    float s1 = src[0] * src[6] - src[4] * src[2];
    float s2 = src[0] * src[7] - src[4] * src[3];
    float s3 = src[1] * src[6] - src[5] * src[2];
    float s4 = src[1] * src[7] - src[5] * src[3];
    float s5 = src[2] * src[7] - src[6] * src[3];

    float c5 = src[10] * src[15] - src[14] * src[11];
    float c4 = src[9] * src[15] - src[13] * src[11];
    float c3 = src[9] * src[14] - src[13] * src[10];
    float c2 = src[8] * src[15] - src[12] * src[11];
    float c1 = src[8] * src[14] - src[12] * src[10];
    float c0 = src[8] * src[13] - src[12] * src[9];

    float invdet = 1 / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

    inv[0] = (src[5] * c5 - src[6] * c4 + src[7] * c3) * invdet;
    inv[1] = (-src[1] * c5 + src[2] * c4 - src[3] * c3) * invdet;
    inv[2] = (src[13] * s5 - src[14] * s4 + src[15] * s3) * invdet;
    inv[3] = (-src[9] * s5 + src[10] * s4 - src[11] * s3) * invdet;

    inv[4] = (-src[4] * c5 + src[6] * c2 - src[7] * c1) * invdet;
    inv[5] = (src[0] * c5 - src[2] * c2 + src[3] * c1) * invdet;
    inv[6] = (-src[12] * s5 + src[14] * s2 - src[15] * s1) * invdet;
    inv[7] = (src[8] * s5 - src[10] * s2 + src[11] * s1) * invdet;

    inv[8] = (src[4] * c4 - src[5] * c2 + src[7] * c0) * invdet;
    inv[9] = (-src[0] * c4 + src[1] * c2 - src[3] * c0) * invdet;
    inv[10] = (src[12] * s4 - src[13] * s2 + src[15] * s0) * invdet;
    inv[11] = (-src[8] * s4 + src[9] * s2 - src[11] * s0) * invdet;

    inv[12] = (-src[4] * c3 + src[5] * c1 - src[6] * c0) * invdet;
    inv[13] = (src[0] * c3 - src[1] * c1 + src[2] * c0) * invdet;
    inv[14] = (-src[12] * s3 + src[13] * s1 - src[14] * s0) * invdet;
    inv[15] = (src[8] * s3 - src[9] * s1 + src[10] * s0) * invdet;

    MatrixCopy(out, inv);
}

void MatrixTranspose(MATRIX src)
{
    MATRIX out;
    out[0] = src[0];
    out[1] = src[4];
    out[2] = src[8];
    out[3] = src[12];

    out[4] = src[1];
    out[5] = src[5];
    out[6] = src[9];
    out[7] = src[13];

    out[8] = src[2];
    out[9] = src[6];
    out[10] = src[10];
    out[11] = src[14];

    out[12] = src[3];
    out[13] = src[7];
    out[14] = src[11];
    out[15] = src[15];
    MatrixCopy(src, out);
}

void ComputeReflectionMatrix(VECTOR normal, MATRIX res)
{

    res[0] = 1 - 2 * normal[0] * normal[0];
    res[1] = -2 * normal[0] * normal[1];
    res[2] = -2 * normal[0] * normal[2];
    res[3] = 0;

    res[4] = -2 * normal[1] * normal[0];
    res[5] = 1 - 2 * normal[1] * normal[1];
    res[6] = -2 * normal[1] * normal[2];
    res[7] = 0;

    res[8] = -2 * normal[0] * normal[2];
    res[9] = -2 * normal[2] * normal[1];
    res[10] = 1 - 2 * normal[2] * normal[2];
    res[11] = 0;

    res[12] = -2 * normal[0] * normal[3];
    res[13] = -2 * normal[1] * normal[3];
    res[14] = -2 * normal[2] * normal[3];
    res[15] = 1;
}

void CreateWorldMatrixFromVectors(VECTOR pos, VECTOR up, VECTOR forward, VECTOR right, VECTOR scales, MATRIX m)
{
    MATRIX work;
    MatrixIdentity(work);
    work[0] = right[0] * scales[0];
    work[1] = right[1] * scales[0];
    work[2] = right[2] * scales[0];

    work[4] = up[0] * scales[1];
    work[5] = up[1] * scales[1];
    work[6] = up[2] * scales[1];

    work[8] = forward[0] * scales[2];
    work[9] = forward[1] * scales[2];
    work[10] = forward[2] * scales[2];

    work[12] = pos[0];
    work[13] = pos[1];
    work[14] = pos[2];
    work[15] = 1.0f;

    MatrixCopy(m, work);
}

// #define DOES_THIS_WORK
// incomplete
void GribbHartmann(MATRIX m)
{
    // MATRIX m;
    // MatrixInverse(mat, m);
    // MatrixTranspose(m);

    // MATRIX m;

    // MatrixInverse(mat, m);

    DumpMatrix(m);

    VECTOR m_left_plane, lp;
#ifndef DOES_THIS_WORK
    m_left_plane[0] = m[12] + m[0];
    m_left_plane[1] = m[13] + m[1];
    m_left_plane[2] = m[14] + m[2];
    m_left_plane[3] = m[15] + m[3];
#else
    m_left_plane[0] = m[3] + m[0];
    m_left_plane[1] = m[7] + m[4];
    m_left_plane[2] = m[11] + m[8];
    m_left_plane[3] = m[15] + m[12];
#endif
    NormalizePlane(m_left_plane, m_left_plane);

    DumpVector(m_left_plane);

    DEBUGLOG("#1");

    VECTOR m_right_plane, rp;
#ifndef DOES_THIS_WORK
    m_right_plane[0] = m[12] - m[0];
    m_right_plane[1] = m[13] - m[1];
    m_right_plane[2] = m[14] - m[2];
    m_right_plane[3] = m[15] - m[3];
#else
    m_right_plane[0] = m[3] - m[0];
    m_right_plane[1] = m[7] - m[4];
    m_right_plane[2] = m[11] - m[8];
    m_right_plane[3] = m[15] - m[12];
#endif
    NormalizePlane(m_right_plane, m_right_plane);

    DumpVector(m_right_plane);

    DEBUGLOG("#2");

    VECTOR m_bottom_plane, bp;
#ifndef DOES_THIS_WORK
    m_bottom_plane[0] = m[12] + m[4];
    m_bottom_plane[1] = m[13] + m[5];
    m_bottom_plane[2] = m[14] + m[6];
    m_bottom_plane[3] = m[15] + m[7];
#else
    m_bottom_plane[0] = m[3] + m[1];
    m_bottom_plane[1] = m[7] + m[5];
    m_bottom_plane[2] = m[11] + m[9];
    m_bottom_plane[3] = m[15] + m[13];
#endif
    NormalizePlane(m_bottom_plane, m_bottom_plane);

    DumpVector(m_bottom_plane);

    DEBUGLOG("#3");

    VECTOR m_top_plane, tp;
#ifndef DOES_THIS_WORK
    m_top_plane[0] = m[12] - m[4];
    m_top_plane[1] = m[13] - m[5];
    m_top_plane[2] = m[14] - m[6];
    m_top_plane[3] = m[15] - m[7];
#else
    m_top_plane[0] = m[4] - m[1];
    m_top_plane[1] = m[7] - m[5];
    m_top_plane[2] = m[11] - m[9];
    m_top_plane[3] = m[15] - m[13];
#endif
    NormalizePlane(m_top_plane, m_top_plane);

    DumpVector(m_top_plane);

    DEBUGLOG("#4");

    VECTOR m_near_plane, np;
#ifndef DOES_THIS_WORK
    m_near_plane[0] = m[12] + m[8];
    m_near_plane[1] = m[13] + m[9];
    m_near_plane[2] = m[14] + m[10];
    m_near_plane[3] = m[15] + m[11];
#else
    m_near_plane[0] = m[3] + m[2];
    m_near_plane[1] = m[7] + m[6];
    m_near_plane[2] = m[11] + m[10];
    m_near_plane[3] = m[15] + m[14];
#endif
    NormalizePlane(m_near_plane, np);
    DumpVector(np);

    DEBUGLOG("#5");

    VECTOR m_far_plane, tf;
#ifndef DOES_THIS_WORK
    m_far_plane[0] = m[12] - m[8];
    m_far_plane[1] = m[13] - m[9];
    m_far_plane[2] = m[14] - m[10];
    m_far_plane[3] = m[15] - m[11];
#else
    m_far_plane[0] = m[3] - m[2];
    m_far_plane[1] = m[7] - m[6];
    m_far_plane[2] = m[11] - m[10];
    m_far_plane[3] = m[15] - m[14];
#endif

    NormalizePlane(m_far_plane, tf);
    DumpVector(tf);

    DEBUGLOG("#6");
}

void ExtractVectorFromMatrix(VECTOR trans, VECTOR rot, VECTOR scale, MATRIX m)
{
    trans[0] = m[12];
    trans[1] = m[13];
    trans[2] = m[14];

    float sx = dist(&m[0]);
    float sy = dist(&m[4]);
    float sz = dist(&m[8]);

    scale[0] = sx;
    scale[1] = sy;
    scale[2] = sz;

    MATRIX mat;
    mat[0] = m[0] / sx;
    mat[1] = m[1] / sx;
    mat[2] = m[2] / sx;

    mat[4] = m[4] / sy;
    mat[5] = m[5] / sy;
    mat[6] = m[6] / sy;

    mat[8] = m[8] / sz;
    mat[9] = m[9] / sz;
    mat[10] = m[10] / sz;

    CreateQuatRotationAxes(&mat[0], &mat[4], &mat[8], rot);
}

void CreateWorldMatrixFromQuatScalesTrans(VECTOR trans, VECTOR rot, VECTOR scale, MATRIX m)
{
    MatrixIdentity(m);
    CreateRotationMatFromQuat(rot, m);

    float sx = scale[0];
    float sy = scale[1];
    float sz = scale[2];

    m[0] *= sx;
    m[1] *= sx;
    m[2] *= sx;

    m[4] *= sy;
    m[5] *= sy;
    m[6] *= sy;

    m[8] *= sz;
    m[9] *= sz;
    m[10] *= sz;

    m[12] = trans[0];
    m[13] = trans[1];
    m[14] = trans[2];
    m[15] = 1.0f;
}
