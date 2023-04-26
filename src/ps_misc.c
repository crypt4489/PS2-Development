#include "ps_misc.h"
#include "ps_fast_maths.h"
#include "ps_log.h"

#include <stdlib.h>
#include <string.h>
VECTOR forward = {0.0f, 0.0f, 1.0f, 1.0f};
VECTOR up = {0.0f, 1.0f, 0.0f, 1.0f};
VECTOR right = {1.0f, 0.0f, 0.0f, 1.0f};


void VectorSubtractXYZ(VECTOR in, VECTOR in2, VECTOR out)
{
    VECTOR work;
    work[0] = in[0] - in2[0];
    work[1] = in[1] - in2[1];
    work[2] = in[2] - in2[2];
    work[3] = in[3];
    vector_copy(out, work);
}

qword_t *vector_to_qword(qword_t *q, VECTOR v)
{
    ((float *)q->sw)[0] = v[0];
    ((float *)q->sw)[1] = v[1];
    ((float *)q->sw)[2] = v[2];
    ((float *)q->sw)[3] = v[3];
    q++;
    return q;
}

void CreateRotationAndCopyMatFromObjAxes(MATRIX out, VECTOR up, VECTOR forward, VECTOR right)
{
    MATRIX temp_out;
    matrix_unit(temp_out);

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

    matrix_copy(out, temp_out);
}

void CreateVector(float x, float y, float z, float w, VECTOR out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

void dump_packet(qword_t *q)
{
    qword_t *iter = q;
    ; //= obj->pipeline_dma;
    int i = 0;
    while (iter->sw[0] != DMA_DCODE_END)
    {
        float f =  ((float*)iter->sw)[3];
        DEBUGLOG("%x %x %x %f", iter->sw[0], iter->sw[1], iter->sw[2], f);
        iter++;
        i++;
    }
    DEBUGLOG("%d", i);

    DEBUGLOG("________________________________");
}
void VectorAddXYZ(VECTOR in, VECTOR in2, VECTOR out)
{
    out[0] = in[0] + in2[0];
    out[1] = in[1] + in2[1];
    out[2] = in[2] + in2[2];
}

void normalize(VECTOR in, VECTOR out)
{
    float w = in[3];
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vmaddax.w $ACC, $vf0, $vf2\n"
        "vmadday.w $ACC, $vf0, $vf2\n"
        "vmaddz.w $vf2, $vf0, $vf2\n"
        "vrsqrt $Q, $vf0w, $vf2w\n"
        "vsub.w $vf1, $vf0, $vf0\n"
        "vwaitq \n"
        "vmulq.xyz $vf1, $vf1, $Q \n"
        "sqc2 $vf1, 0x00(%0) \n"
        :
        : "r"(out), "r"(in)
        : "memory");
    out[3] = w;
}

void NormalizePlane(VECTOR in, VECTOR out)
{
   // float w = in[3];
  /*  asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vmaddax.w $ACC, $vf0, $vf2\n"
        "vmadday.w $ACC, $vf0, $vf2\n"
        "vmaddz.w $vf2, $vf0, $vf2\n"
        "vrsqrt $Q, $vf0w, $vf2w\n"
        "vsub.w $vf0, $vf0, $vf0\n"
        "vwaitq \n"
        "vmulq.xyzw $vf1, $vf1, $Q \n"
        "sqc2 $vf1, 0x00(%0) \n"
        :
        : "r"(out), "r"(in)
        : "memory"); */
    //out[3] = w;

    DumpVector(in);

    float x = (in[0] * in[0]) + (in[1] * in[1]) + (in[2] * in[2]);

    x = 1.0f / Sqrt(x);

    DEBUGLOG("%f\n", x);

    out[0] = x * in[0];

    out[1] = x * in[1];

    out[2] = x * in[2];

    out[3] = x * in[3];
}

void RandomVectorsInit(VECTOR in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "vrinit $R, $vf1x\n"
        :
        : "r"(in)
        : "memory");
}

void GetRandomVectors(VECTOR in)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "vrget.x $vf1x, $R\n"
        "vrnext.y $vf1y, $R\n"
        "vrnext.z $vf1z, $R\n"
        "sqc2 $vf1, 0x00(%0)"
        :
        : "r"(in)
        : "memory");
}

float dist(VECTOR in)
{
    float d = 0.0f;
    asm __volatile__(
        "mula.s %1, %1 \n"
        "madda.s %2, %2 \n"
        "madd.s %0, %3, %3 \n"
        "sqrt.s %0, %0 \n"
        : "=f"(d)
        : "f"(in[0]), "f"(in[1]), "f"(in[2]));
    return d;
}

void VectorCopyXYZ(VECTOR in, VECTOR out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

float DotProduct(VECTOR in1, VECTOR in2)
{
    float ret = 0.0f;
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "lqc2 $vf2, 0x00(%2)\n"
        "vmul.xyz $vf3, $vf1, $vf2\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "qmfc2 %0, $vf3\n"
        : "=r"(ret)
        : "r"(in1), "r"(in2)
        : "memory");
    return ret;
}

float dotProductFour(VECTOR in1, VECTOR in2)
{
    float ret = 0.0f;
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "lqc2 $vf2, 0x00(%2)\n"
        "vmul.xyzw $vf3, $vf1, $vf2\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "vaddw.x $vf3x, $vf3x, $vf3w\n"
        "qmfc2 %0, $vf3\n"
        : "=r"(ret)
        : "r"(in1), "r"(in2)
        : "memory");
    return ret;
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

float distCOP2(VECTOR in)
{
    float dist = 0.0f;
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vmaddax.w $ACC, $vf0, $vf2\n"
        "vmadday.w $ACC, $vf0, $vf2\n"
        "vmaddz.w $vf2, $vf0, $vf2\n"
        "vsqrt $Q, $vf2w\n"
        "vwaitq \n"
        "vaddq.x $vf1, $vf0, $Q \n"
        "qmfc2 %0, $vf1"
        : "=r"(dist)
        : "r"(in)
        : "memory");
    return dist;
}

void CreateGridVectors(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int v_count = N * M;

    // int faceCount = 2 * (N - 1) * (M - 1);

    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (N - 1);
    float dy = depth / (M - 1);

    // float du = 1.0f / (N - 1);
    // float dv = 1.0f / (M - 1);

    VECTOR *vertices = (VECTOR *)malloc(sizeof(VECTOR) * v_count);

    int index = 0;
    float z, x;
    for (int i = 0; i < M; i++)
    {
        z = halfDepth - dy * i;
        for (int j = 0; j < N; j++)
        {
            x = -halfWidth + dx * j;
            index = i * N + j;
            vertices[index][0] = x;
            vertices[index][1] = 0.0f;
            vertices[index][2] = z;
            vertices[index][3] = 1.0f;
        }
    }

    // go->vertices = (VECTOR*)malloc(sizeof(VECTOR) * index_count);

    for (int i = 0; i < buffer->vertexCount; i++)
    {
        int index = buffer->indices[i];
        vector_copy(buffer->vertices[i], vertices[index]);
        //  vector_copy(go->texCoords[i], uvs[index]);
        // DEBUGLOG("here %d\n", index);
    }

    free(vertices);

    // return go->vertices;
}

void CreateGridUVS(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int v_count = N * M;

    float du = 1.0f / (N - 1);
    float dv = 1.0f / (M - 1);

    VECTOR *uvs = (VECTOR *)malloc(sizeof(VECTOR) * v_count);

    int index = 0;
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            index = i * N + j;
            uvs[index][0] = j * du;
            uvs[index][1] = ((M - 1) - i) * dv;
            uvs[index][2] = 1.0f;
            uvs[index][3] = 0.0f;
        }
    }

    for (int i = 0; i < buffer->vertexCount; i++)
    {
        int index = buffer->indices[i];
        vector_copy(buffer->texCoords[i], uvs[index]);
    }

    free(uvs);
}

MeshBuffers* CreateGrid(int N, int M, float depth, float width, MeshBuffers *buffer)
{


    int faceCount = 2 * (N - 1) * (M - 1);


    int index_count = faceCount * 3;

    buffer->vertexCount = index_count;

    DEBUGLOG("Grid indices count %d", buffer->vertexCount);

    buffer->indices = (u32 *)malloc(sizeof(int) * index_count);
    buffer->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
    buffer->vertices = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
    CreateGridIndices(N, M, width, depth, buffer);
    CreateGridVectors(N, M, width, depth, buffer);
    CreateGridUVS(N, M, width, depth, buffer);
    return buffer;
}

void CreateGridIndices(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int k = 0;
    for (int j = 0; j < M - 1; j++)
    {
        for (int i = 0; i < N - 1; i++)
        {
            buffer->indices[k] = j * N + i;
            buffer->indices[k + 1] = j * N + i + 1;
            buffer->indices[k + 2] = (j + 1) * N + i;
            buffer->indices[k + 3] = (j + 1) * N + i;
            buffer->indices[k + 4] = j * N + i + 1;
            buffer->indices[k + 5] = (j + 1) * N + i + 1;
            k += 6;
        }
    }
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

    matrix_unit(temp);

    temp[0] = w;
    temp[5] = -h;
    temp[10] = (f + n) / (f - n);
    temp[11] = -1;
    temp[14] = (2 * f * n) / (f - n);
    temp[15] = 0;

    matrix_copy(output, temp);
}

void CreateNormalizedTextureCoordinateMatrix(MATRIX src)
{
    src[0] = 0.5 * src[0];
    src[1] = 0.5 * src[1];
    src[2] = 0.5 * src[2];

    src[4] = 0.5 * src[4];
    src[5] = 0.5 * src[5];
    src[6] = 0.5 * src[6];

    src[8] = 0.5 * src[8];
    src[9] = 0.5 * src[9];
    src[10] = 0.5 * src[10];

    src[12] = 0.5f;
    src[13] = 0.5f;
    src[14] = 0.0f;
    src[15] = 1.0f;
}

void CreateOrthoGraphicMatrix(float xLow, float xHigh, float yLow, float yHigh, float near, float far, MATRIX out)
{
    MATRIX temp;
    matrix_unit(temp);

    temp[0] = 2.0f / (yHigh - xLow);
    temp[3] = -(yHigh + xLow) / (yHigh - xLow);
    temp[7] = -(xHigh + yLow) / (xHigh - yLow);
    temp[11] = -(far + near) / (far - near);
    temp[5] = 2.0f / (xHigh - yLow);
    temp[10] = -2.0f / (far - near);


    matrix_copy(out, temp);
}

void CrossProduct(VECTOR m, VECTOR n, VECTOR out)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "vopmula.xyz $ACC, $vf1, $vf2\n"
        "vopmsub.xyz $vf3, $vf2, $vf1\n"
        "vsub.w $vf3, $vf0, $vf0\n"
        "sqc2 $vf3, 0x00(%2)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        :
        : "r"(m), "r"(n), "r"(out)
        : "memory");
}

void ScaleVectorXYZ(VECTOR vec, VECTOR input, float scale)
{
    vec[0] = input[0] * scale;
    vec[1] = input[1] * scale;
    vec[2] = input[2] * scale;
}

void ZeroVector(VECTOR out)
{
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;
}

void CreateRotationMatrix(VECTOR axis, float angle, MATRIX output)
{
    MATRIX work;

    float s = Sin(angle);
    float c = Cos(angle);

    float x = axis[0];
    float y = axis[1];
    float z = axis[2];

    matrix_unit(work);

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

    matrix_copy(output, work);
}

void CreateTranslationMatrix(VECTOR pos, MATRIX output)
{
    MATRIX work;
    matrix_unit(work);

    work[12] = pos[0];
    work[13] = pos[1];
    work[14] = pos[2];

    matrix_copy(output, work);
}
void CreateScaleMatrix(VECTOR scales, MATRIX output)
{
    MATRIX work;
    matrix_unit(work);

    work[0] = scales[0];
    work[5] = scales[1];
    work[10] = scales[2];
    work[15] = 1.0f;

    matrix_copy(output, work);
}

void CreateWorldMatrix(MATRIX output, MATRIX scales, MATRIX rot, MATRIX trans)
{
    MATRIX work;
    matrix_unit(work);

    matrix_multiply(work, work, scales);

    matrix_multiply(work, work, rot);

    matrix_multiply(work, work, trans);

    matrix_copy(output, work);
}

void computeNormal(VECTOR v0, VECTOR v1, VECTOR v2, VECTOR out)
{
    VECTOR u, t, tempOut;

    VectorSubtractXYZ(v1, v0, u);
    VectorSubtractXYZ(v2, v0, t);

    CrossProduct(u, t, tempOut);
    normalize(tempOut, out);
}

void DumpMatrix(MATRIX elem)
{
    printf("%f %f %f %f\n", elem[0], elem[1], elem[2], elem[3]);
    printf("%f %f %f %f\n", elem[4], elem[5], elem[6], elem[7]);
    printf("%f %f %f %f\n", elem[8], elem[9], elem[10], elem[11]);
    printf("%f %f %f %f\n\n", elem[12], elem[13], elem[14], elem[15]);
}

void DumpVector(VECTOR elem)
{
    printf("%f %f %f %f\n", elem[0], elem[1], elem[2], elem[3]);
}


void DumpVectorInt(VectorInt elem)
{
    printf("%d %d %d %d\n", elem[0], elem[1], elem[2], elem[3]);
}

void MatrixInverse(MATRIX src, MATRIX out)
{
    MATRIX inv;
    matrix_unit(inv);

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

    matrix_copy(out, inv);
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
    matrix_copy(src, out);
}

void ComputePlane(VECTOR vec, VECTOR normal, VECTOR plane)
{
    float x0, y0, z0, d;
    x0 = -vec[0] * normal[0];
    y0 = -vec[1] * normal[1];
    z0 = -vec[2] * normal[2];
    d = x0 + y0 + z0;

    CreateVector(normal[0], normal[1], normal[2], d, plane);
}

void PointInPlane(VECTOR plane, VECTOR p, VECTOR pointInPlane, VECTOR planePoint)
{
    VECTOR v, n;
    vector_copy(n, plane);
    VectorSubtractXYZ(pointInPlane, p, v);
    float d = Abs(DotProduct(v, n));
    // printf("%f\n", d);
    ScaleVectorXYZ(n, n, d);
    VectorSubtractXYZ(p, n, planePoint);
}

void SetupPlane(VECTOR planeNormal, VECTOR planePoint, Plane *plane)
{
    vector_copy(plane->pointInPlane, planePoint);
    ComputePlane(planePoint, planeNormal, plane->planeEquation);
}


float DistanceFromPlane(VECTOR planeEquation, VECTOR point)
{
    return DotProduct(planeEquation, point) + planeEquation[3];
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
     matrix_unit(work);
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

    matrix_copy(m, work);
}
MeshBuffers *InitMeshBuffersStruct(u32 count, MeshBuffers *buffer)
{
    buffer->vertices = (VECTOR*)malloc(sizeof(VECTOR)*count);
    buffer->texCoords = (VECTOR*)malloc(sizeof(VECTOR)*count);
    return buffer;
}

void Pathify(const char *name, char *file)
{
    int len = strlen(name);
    file[0] = 92;
    for (int i = 1; i<=len; i++)
    {
        file[i] = name[i-1];
    }
    file[len+1] = 59;
    file[len+2] = 49;
    file[len+3] = 0;
}


// append characters from input2 to input1 and store in output
void AppendString(const char *input1, const char *input2, char *output, u32 max)
{
    const char *iter = input1;
    char *outIter = output;

    u32 len = 0;

    while(*iter != 0 && len < max)
    {
        *outIter = *iter;
        outIter++;
        iter++;
        len++;
    }

    iter = input2;

    while(*iter != 0 && len < max)
    {
         *outIter = *iter;
        outIter++;
        iter++;
        len++;
    }

    if (len >= max)
    {
        ERRORLOG("error appending strings. too long given input size");
    }

    *outIter = 0;
}

// #define DOES_THIS_WORK
// incomplete
static void GribbHartmann(MATRIX m)
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