#include "ps_lights.h"
#include "ps_misc.h"
#include "ps_fast_maths.h"
#include <stdlib.h>

qword_t * create_light_color_vu1_vector(qword_t* q, VECTOR color)
{
    ((float *)q->sw)[0] = (255.0f * color[0]);
    ((float *)q->sw)[1] = (255.0f * color[1]);
    ((float *)q->sw)[2] = (255.0f * color[2]);
    q++;
    return q;
}

qword_t * create_light_vu1_vector(qword_t* q, VECTOR vec)
{
    ((float *)q->sw)[0] =  vec[0];
    ((float *)q->sw)[1] = vec[1];
    ((float *)q->sw)[2] = vec[2];
    q++;
    return q;
   
}

LightStruct *CreateLightStruct(u32 type)
{
    LightStruct *light = (LightStruct*)malloc(sizeof(LightStruct));
    light->theta = 0.0f;
    light->radius = 0.0f;
    light->type = type;
    matrix_unit(light->ltm);
    return light;
}
void SetLightColor(LightStruct *light, VECTOR color)
{
    vector_copy(light->color, color);
}

void SetLightRadius(LightStruct *light, float rad)
{
    light->radius = rad;
}

void SetLightTheta(LightStruct *light, float ang)
{
    light->theta = Cos(DegToRad(ang));
}

qword_t* QWordLightAddFloat(qword_t *q, float val)
{
    ((float *)q->sw)[3] = val;
    return q;
}

qword_t * SetLightTypeVU1(qword_t *q, u32 type)
{
    q->sw[3] = type;
    return q;
}   

qword_t *PackLightIntoQWord(qword_t *q, LightStruct *light)
{
    qword_t *b =q;
    VECTOR adjust;
    b = SetLightTypeVU1(b, light->type);
    if (light->type == PS_AMBIENT_LIGHT)
    {
        
        b = create_light_color_vu1_vector(b, light->color);
    } 
    else if (light->type == PS_DIRECTIONAL_LIGHT)
    {
        normalize(*GetForwardVectorLTM(light->ltm), adjust);
        b = create_light_vu1_vector(b, adjust);
        b = create_light_color_vu1_vector(b, light->color);
    }
    else if (light->type == PS_POINT_LIGHT)
    {
        b = create_light_vu1_vector(b, *GetPositionVectorLTM(light->ltm));
        b = QWordLightAddFloat(b, light->radius);
        b = create_light_color_vu1_vector(b, light->color);
    } 
    else if (light->type == PS_SPOT_LIGHT)
    { 
        b = create_light_vu1_vector(b, *GetPositionVectorLTM(light->ltm));
        b = QWordLightAddFloat(b, light->radius);
        b = create_light_color_vu1_vector(b, light->color);
        b = QWordLightAddFloat(b, light->theta);
        normalize(*GetForwardVectorLTM(light->ltm), adjust);
        b = create_light_vu1_vector(b, adjust);
    }


    return b;
}

qword_t *InitVU1LightHeader(qword_t *q, u32 count)
{
    q->sw[0] = count;
    q->sw[1] = count;
    q->sw[2] = count;
    q->sw[3] = count;
    q++;
    return q;
}

