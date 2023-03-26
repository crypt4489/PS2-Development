#ifndef PS_LIGHTS_H
#define PS_LIGHTS_H
#include "ps_global.h"


LightStruct *CreateLightStruct(u32 type);
void SetLightColor(LightStruct *light, VECTOR color);
void SetLightTheta(LightStruct *light, float ang);
qword_t *PackLightIntoQWord(qword_t *q, LightStruct *light);

inline qword_t *CreateLightColorVectorVU1(qword_t* q, VECTOR color)
{
    ((float *)q->sw)[0] = (255.0f * color[0]);
    ((float *)q->sw)[1] = (255.0f * color[1]);
    ((float *)q->sw)[2] = (255.0f * color[2]);
    q++;
    return q;
};

inline qword_t *CreateLightTranslateRotationVectorVU1(qword_t* q, VECTOR vec)
{
    ((float *)q->sw)[0] =  vec[0];
    ((float *)q->sw)[1] = vec[1];
    ((float *)q->sw)[2] = vec[2];
    q++;
    return q;
};

inline qword_t *InitVU1LightHeader(qword_t *q, u32 count)
{
    q->sw[0] = count;
    q->sw[1] = count;
    q->sw[2] = count;
    q->sw[3] = count;
    q++;
    return q;
};

inline void SetLightRadius(LightStruct *light, float rad)
{
    light->radius = rad;
};

inline qword_t* QWordLightAddFloat(qword_t *q, float val)
{
    ((float *)q->sw)[3] = val;
    return q;
};

inline qword_t * SetLightTypeVU1(qword_t *q, u32 type)
{
    q->sw[3] = type;
    return q;
};

#endif