#include "world/ps_lights.h"

#include <stdlib.h>

#include "math/ps_fast_maths.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

static inline qword_t *CreateLightColorVectorVU1(qword_t* q, VECTOR color)
{
    ((float *)q->sw)[0] = (255.0f * color[0]);
    ((float *)q->sw)[1] = (255.0f * color[1]);
    ((float *)q->sw)[2] = (255.0f * color[2]);
    q++;
    return q;
}

static inline qword_t *CreateLightTranslateRotationVectorVU1(qword_t* q, VECTOR vec)
{
    ((float *)q->sw)[0] =  vec[0];
    ((float *)q->sw)[1] = vec[1];
    ((float *)q->sw)[2] = vec[2];
    q++;
    return q;
}

static inline qword_t* QWordLightAddFloat(qword_t *q, float val)
{
    ((float *)q->sw)[3] = val;
    return q;
}

static inline qword_t * SetLightTypeVU1(qword_t *q, u32 type)
{
    q->sw[3] = type;
    return q;
}

LightStruct *CreateLightStruct(u32 type)
{
    LightStruct *light = (LightStruct *)malloc(sizeof(LightStruct));
    light->theta = 0.0f;
    light->radius = 0.0f;
    light->type = type;
    MatrixIdentity(light->ltm);
    return light;
}
void SetLightColor(LightStruct *light, VECTOR color)
{
    VectorCopy(light->color, color);
}

void SetLightTheta(LightStruct *light, float ang)
{
    light->theta = Cos(DegToRad(ang));
}

qword_t *PackLightIntoQWord(qword_t *q, LightStruct *light)
{
    qword_t *b = q;
    VECTOR adjust;
    b = SetLightTypeVU1(b, light->type);
    if (light->type == PS_AMBIENT_LIGHT)
    {

        b = CreateLightColorVectorVU1(b, light->color);
    }
    else if (light->type == PS_DIRECTIONAL_LIGHT)
    {
        Normalize(*GetForwardVectorLTM(light->ltm), adjust);
        b = CreateLightTranslateRotationVectorVU1(b, adjust);
        b = CreateLightColorVectorVU1(b, light->color);
    }
    else if (light->type == PS_POINT_LIGHT)
    {
        b = CreateLightTranslateRotationVectorVU1(b, *GetPositionVectorLTM(light->ltm));
        b = QWordLightAddFloat(b, light->radius);
        b = CreateLightColorVectorVU1(b, light->color);
    }
    else if (light->type == PS_SPOT_LIGHT)
    {
        b = CreateLightTranslateRotationVectorVU1(b, *GetPositionVectorLTM(light->ltm));
        b = QWordLightAddFloat(b, light->radius);
        b = CreateLightColorVectorVU1(b, light->color);
        b = QWordLightAddFloat(b, light->theta);
        Normalize(*GetForwardVectorLTM(light->ltm), adjust);
        b = CreateLightTranslateRotationVectorVU1(b, adjust);
    }

    return b;
}
