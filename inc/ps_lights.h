#ifndef PS_LIGHTS_H
#define PS_LIGHTS_H
#include "ps_global.h"

qword_t * create_light_color_vu1_vector(qword_t* q, VECTOR color);
LightStruct *CreateLightStruct(u32 type);
void SetLightColor(LightStruct *light, VECTOR color);
void SetLightRadius(LightStruct *light, float rad);
qword_t *PackLightIntoQWord(qword_t *q, LightStruct *light);
qword_t * create_light_vu1_vector(qword_t* q, VECTOR vec);
qword_t * InitVU1LightHeader(qword_t *q, u32 count);
qword_t * SetLightTypeVU1(qword_t *q, u32 type);
void SetLightTheta(LightStruct *light, float ang);
qword_t* QWordLightAddFloat(qword_t *q, float val);


#endif