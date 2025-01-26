#ifndef PS_GAMEOBJECT_H
#define PS_GAMEOBJECT_H

#include "ps_global.h"
#include "gameobject/ps_material.h"

#define CREATE_PRIM_STRUCT(prim, primType, shade, map, fog, blend, antialias, map_type, fix) \
    prim.type = primType,                                                                    \
    prim.shading = shade,                                                                    \
    prim.mapping = map,                                                                      \
    prim.fogging = fog,                                                                      \
    prim.blending = blend,                                                                   \
    prim.antialiasing = antialias,                                                           \
    prim.mapping_type = map_type,                                                            \
    prim.colorfix = fix

#define CREATE_ALPHA_REGS(blender, c1, c2, c3, a, fixed) \
    blender.color1 = c1,                                 \
    blender.color2 = c2,                                 \
    blender.color3 = c3,                                 \
    blender.alpha = a,                                   \
    blender.fixed_alpha = fixed

#define CREATE_RGBAQ_STRUCT(c, red, green, blue, alpha, qv) \
    c.r = red,                                              \
    c.g = green,                                            \
    c.b = blue,                                             \
    c.a = alpha,                                            \
    c.q = qv

void CleanGameObject(GameObject *obj);
GameObject *InitializeGameObject();
VertexType GetVertexType(ObjectProperties *state);
void SetupGameObjectPrimRegs(GameObject *obj, Color color, u32 renderState);

#endif