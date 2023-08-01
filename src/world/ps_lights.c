#include "world/ps_lights.h"

#include <stdlib.h>

#include "math/ps_fast_maths.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

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
