#include <stdio.h>
#include "ps_quat.h"
#include "ps_misc.h"
#include "ps_fast_maths.h"
#include "ps_log.h"

void CreateQuatRotationAxes(const VECTOR right, const VECTOR up, const VECTOR forward, VECTOR out)
{

  float trace = right[0] + up[1] + forward[2];
  if( trace > 1.0f ) {
    float s = 0.5f / Sqrt(trace+ 1.0f);
    out[0] = 0.25f / s;
    out[1] = ( up[2] - forward[1] ) * s;
    out[2] = ( forward[0] - right[2] ) * s;
    out[3] = ( right[1] - up[0] ) * s;
   // DEBUGLOG("HERE1");
  } else {
    if ( right[0] > up[1] && right[0] > forward[2] ) {
      float s = 2.0f * Sqrt( 1.0f + right[0] - up[1] - forward[2]);
      out[0] = (up[2] - forward[1] ) / s;
      out[1] = 0.25f * s;
      out[2] = (right[1] + up[0] ) / s;
      out[3] = (right[2] + forward[0] ) / s;
     // DEBUGLOG("HERE2");
    } else if (up[1] > forward[2]) {
      float s = 2.0f * Sqrt( 1.0f + up[1] - right[0] - forward[2]);
      out[0] = (forward[0] - right[2] ) / s;
      out[1] = (right[1] + up[0] ) / s;
      out[2] = 0.25f * s;
      out[3] = (up[2] + forward[1] ) / s;
     // DEBUGLOG("HERE3");
    } else {
      float s = 2.0f * Sqrt( 1.0f + forward[2] - right[0] - up[1] );
      out[0] = (right[1] - up[0] ) / s;
      out[1] = (right[2] + forward[0] ) / s;
      out[2] = (up[2] + forward[1] ) / s;
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

    m[0] = 2 * (q0 * q0 + q1 * q1) - 1;
    m[4] = 2 * (q1 * q2 - q0 * q3);
    m[8] = 2 * (q1 * q3 + q0 * q2);

    m[1] = 2 * (q1 * q2 + q0 * q3);
    m[5] = 2 * (q0 * q0 + q2 * q2) - 1;
    m[9] = 2 * (q2 * q3 - q0 * q1);

    m[2] = 2 * (q1 * q3 - q0 * q2);
    m[6] = 2 * (q2 * q3 + q0 * q1);
    m[10] = 2 * (q0 * q0 + q3 * q3) - 1;

    m[15] = 1.0f;
}
