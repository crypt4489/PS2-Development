#include "math/ps_quat.h"

#include <stdio.h>

#include "math/ps_vector.h"
#include "math/ps_fast_maths.h"
#include "log/ps_log.h"

void CreateQuatRotationAxes(const VECTOR right, const VECTOR up, const VECTOR forward, VECTOR out)
{

  float trace = right[0] + up[1] + forward[2];
  if (trace > 1.0f)
  {
    float s = 0.5f / Sqrt(trace + 1.0f);
    out[0] = 0.25f / s;
    out[1] = (up[2] - forward[1]) * s;
    out[2] = (forward[0] - right[2]) * s;
    out[3] = (right[1] - up[0]) * s;
    // DEBUGLOG("HERE1");
  }
  else
  {
    if (right[0] > up[1] && right[0] > forward[2])
    {
      float s = 2.0f * Sqrt(1.0f + right[0] - up[1] - forward[2]);
      float invS = 1.0f / s;
      out[0] = (up[2] - forward[1]) * invS;
      out[1] = 0.25f * s;
      out[2] = (right[1] + up[0]) * invS;
      out[3] = (right[2] + forward[0]) * invS;
      // DEBUGLOG("HERE2");
    }
    else if (up[1] > forward[2])
    {
      float s = 2.0f * Sqrt(1.0f + up[1] - right[0] - forward[2]);
      float invS = 1.0f / s;
      out[0] = (forward[0] - right[2]) * invS;
      out[1] = (right[1] + up[0]) * invS;
      out[2] = 0.25f * s;
      out[3] = (up[2] + forward[1]) * invS;
      // DEBUGLOG("HERE3");
    }
    else
    {
      float s = 2.0f * Sqrt(1.0f + forward[2] - right[0] - up[1]);
      float invS = 1.0f / s;
      out[0] = (right[1] - up[0]) * invS;
      out[1] = (right[2] + forward[0]) * invS;
      out[2] = (up[2] + forward[1]) * invS;
      out[3] = 0.25f * s;
      // DEBUGLOG("HERE4");
    }
  }
}

void CreateRotationMatFromQuat(const VECTOR quat, MATRIX m)
{
  float q0 = quat[0];
  float q1 = quat[1];
  float q2 = quat[2];
  float q3 = quat[3];

  float q0q0 = q0 * q0;
  float q0q1 = q0 * q1;
  float q0q2 = q0 * q2;
  float q0q3 = q0 * q3;

  float q1q2 = q1 * q2;
  float q1q3 = q1 * q3;

  float q2q3 = q2 * q3;

  m[0] = 2 * (q0q0 + q1 * q1) - 1;
  m[4] = 2 * (q1q2 - q0q3);
  m[8] = 2 * (q1q3 + q0q2);

  m[1] = 2 * (q1q2 + q0q3);
  m[5] = 2 * (q0q0 + q2 * q2) - 1;
  m[9] = 2 * (q2q3 - q0q1);

  m[2] = 2 * (q1q3 - q0q2);
  m[6] = 2 * (q2q3 + q0q1);
  m[10] = 2 * (q0q0 + q3 * q3) - 1;

  m[15] = 1.0f;
}

void Slerp(VECTOR q1, VECTOR q2, float delta, VECTOR out)
{
    float dot4 = DotProductFour(q1, q2);
    if (Abs(dot4) >= 1.0f)
    {
        //  DEBUGLOG("QUICK COPY");
        VectorCopy(out, q1);
        return;
    }

    float halfTheta = ACos(dot4);
    float sinHalfTheta = Sqrt(1.0f - dot4 * dot4);

    if (Abs(sinHalfTheta) < EPSILON)
    {
        // DEBUGLOG("HERE");
        out[0] = (q1[0] * 0.5f + q2[0] * 0.5);
        out[1] = (q1[1] * 0.5f + q2[1] * 0.5);
        out[2] = (q1[2] * 0.5f + q2[2] * 0.5);
        out[3] = (q1[3] * 0.5f + q2[3] * 0.5);
        return;
    }

    float ratioA = Sin((1.0f - delta) * halfTheta) / sinHalfTheta;
    float ratioB = Sin(delta * halfTheta) / sinHalfTheta;
    // DEBUGLOG("FINAL COPY");
    out[0] = (q1[0] * ratioA + q2[0] * ratioB);
    out[1] = (q1[1] * ratioA + q2[1] * ratioB);
    out[2] = (q1[2] * ratioA + q2[2] * ratioB);
    out[3] = (q1[3] * ratioA + q2[3] * ratioB);

    return;
}

void QuaternionNormalize(VECTOR in, VECTOR out)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%1)\n"
        "vsuba.xyzw $ACC, $vf0, $vf0\n"
        "vmul.xyzw $vf2, $vf1, $vf1\n"
        "vmaddax.w $ACC, $vf0, $vf2\n"
        "vmadday.w $ACC, $vf0, $vf2\n"
        "vmaddaz.w $ACC, $vf0, $vf2\n"
        "vmaddw.w $vf2, $vf0, $vf2\n"
        "vrsqrt $Q, $vf0w, $vf2w\n"
        "vwaitq \n"
        "vmulq.xyzw $vf1, $vf1, $Q \n"
        "sqc2 $vf1, 0x00(%0) \n"
        :
        : "r"(out), "r"(in)
        : "memory");
}