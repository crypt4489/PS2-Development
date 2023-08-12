#ifndef PS_LIGHTS_H
#define PS_LIGHTS_H
#include "ps_global.h"


LightStruct *CreateLightStruct(u32 type);
void SetLightColor(LightStruct *light, VECTOR color);
void SetLightTheta(LightStruct *light, float ang);
qword_t *PackLightIntoQWord(qword_t *q, LightStruct *light);

inline void SetLightRadius(LightStruct *light, float rad)
{
    light->radius = rad;
};

#endif