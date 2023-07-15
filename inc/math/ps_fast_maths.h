#ifndef __PS2MATHS_H__
#define __PS2MATHS_H__

// Morten "Sparky" Mikkelsen's fast maths routines (From a post on the forums
// at www.playstation2-linux.com)
#include "ps_global.h"


float Abs(const float x);
float Sqrt(const float x);
float Max(const float a, const float b);
float Min(const float a, const float b);
float Mod(const float a, const float b);
float ASin(float x);
float ACos(float x);
float Cos(float v);
float Sin(float v);
float DegToRad(float Deg);

#endif
