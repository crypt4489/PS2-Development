#include "math/ps_vector.h"

#include "math/ps_fast_maths.h"
#include "math/ps_quat.h"
#include "log/ps_log.h"

VECTOR forward = {0.0f, 0.0f, 1.0f, 1.0f};
VECTOR up = {0.0f, 1.0f, 0.0f, 1.0f};
VECTOR right = {1.0f, 0.0f, 0.0f, 1.0f};

void VectorCopy(VECTOR out, VECTOR in)
{
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(out), "r"(in)
        : "memory");
}

void VectorIntCopy(VectorInt out, VectorInt in)
{
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(out), "r"(in)
        : "memory");
}

void VectorVoidCopy(void* out, void* in)
{
    asm __volatile__(
        "lqc2		$vf1, 0x00(%1)	\n"
        "sqc2		$vf1, 0x00(%0)	\n"
        :
        : "r"(out), "r"(in)
        : "memory");
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

float DotProductFour(VECTOR in1, VECTOR in2)
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

void DumpVector(VECTOR elem)
{
    printf("%f %f %f %f\n", elem[0], elem[1], elem[2], elem[3]);
}

void DumpVectorInt(VectorInt elem)
{
    printf("%d %d %d %d\n", elem[0], elem[1], elem[2], elem[3]);
}

void VectorAddXYZ(VECTOR in, VECTOR in2, VECTOR out)
{
    out[0] = in[0] + in2[0];
    out[1] = in[1] + in2[1];
    out[2] = in[2] + in2[2];
}

void Normalize(VECTOR in, VECTOR out)
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

void CreateVector(float x, float y, float z, float w, VECTOR out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

void VectorSubtractXYZ(VECTOR in, VECTOR in2, VECTOR out)
{
    VECTOR work;
    work[0] = in[0] - in2[0];
    work[1] = in[1] - in2[1];
    work[2] = in[2] - in2[2];
    work[3] = in[3];
    VectorCopy(out, work);
}

qword_t *VectorToQWord(qword_t *q, VECTOR v)
{
    ((float *)q->sw)[0] = v[0];
    ((float *)q->sw)[1] = v[1];
    ((float *)q->sw)[2] = v[2];
    ((float *)q->sw)[3] = v[3];
    q++;
    return q;
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

void ComputeNormal(VECTOR v0, VECTOR v1, VECTOR v2, VECTOR out)
{
    VECTOR u, t, tempOut;

    VectorSubtractXYZ(v1, v0, u);
    VectorSubtractXYZ(v2, v0, t);

    CrossProduct(u, t, tempOut);
    Normalize(tempOut, out);
}

void LerpNum(VECTOR in1, VECTOR in2, VECTOR output, float delta, u32 components)
{
    float temp;
    for (int i = 0; i < components; i++)
    {
        temp = in2[i] - in1[i];
        temp = temp * delta;
        output[i] = temp + in1[i];
    }
}
