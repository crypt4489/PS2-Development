#include "physics/ps_vbo.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "system/ps_vumanager.h"
#include "system/ps_vif.h"
#include "gamemanager/ps_manager.h"
#include "math/ps_fast_maths.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "dma/ps_dma.h"
#include "system/ps_timer.h"
#include "math/ps_misc.h"
#include "math/ps_plane.h"

extern volatile u32 *vu1_data_address;

#define NOCOLLISION 1
#define COLLISION 0

void DestroyVBO(ObjectBounds *bound)
{
    if (bound)
    {
        if (bound->vbo)
            free(bound->vbo);
        free(bound);
    }
}

void InitVBO(GameObject *obj, int type)
{
    obj->vboContainer = (ObjectBounds *)malloc(sizeof(ObjectBounds));

    obj->vboContainer->type = type;

    if (type == VBO_FIT || type == VBO_FIXED)
    {
        obj->vboContainer->vbo = (void *)malloc(sizeof(BoundingBox));
        FindAABBMaxAndMinVerticesVU0(obj);
    }
    else if (type == VBO_SPHERE)
    {
        obj->vboContainer->vbo = (void *)malloc(sizeof(BoundingSphere));
        ComputeBoundingSphere(obj);
    }
}

void ReadVBOFromVU1(GameObject *obj)
{
    u32 *ptr = vu1_data_address + (14 * 4);
    Bin2Float val;

    BoundingBox *box = (BoundingBox *)obj->vboContainer->vbo;

    val.int_x = *ptr;
    box->bottom[0] = val.float_x;
    ptr++;
    val.int_x = *ptr;
    box->bottom[1] = val.float_x;
    ptr++;
    val.int_x = *ptr;
    box->bottom[2] = val.float_x;
    ptr += 2;
    box->bottom[3] = 0.0f;

    val.int_x = *ptr;
    box->top[0] = val.float_x;
    ptr++;
    val.int_x = *ptr;
    box->top[1] = val.float_x;
    ptr++;
    val.int_x = *ptr;
    box->top[2] = val.float_x;
    ptr += 2;
    box->top[3] = 0.0f;
}

int AABBCollision(VECTOR top1, VECTOR bottom1, VECTOR top2, VECTOR bottom2)
{
    if ((top1[0] > bottom2[0]) && (bottom1[0] < top2[0]) && (top1[1] > bottom2[1]) && (bottom1[1] < top2[1]) && (top1[2] > bottom2[2]) && (bottom1[2] < top2[2]))
    {
        return NOCOLLISION;
    }
    return COLLISION;
}

int SphereCollision(BoundingSphere *s1, BoundingSphere *s2)
{
    VECTOR traj;
    VectorSubtractXYZ(s1->center, s2->center, traj);
    float radiisum = s1->radius + s2->radius;
    if (DotProduct(traj, traj) <= (radiisum * radiisum)) return COLLISION;
    return NOCOLLISION;
}

int SpherePlaneCollision(BoundingSphere *s, Plane *p)
{
    float dist = DistanceFromPlane(p->planeEquation, s->center);
    return Abs(dist) <= s->radius;
}

int PlaneCollision(Plane *p1, Plane *p2, VECTOR axisOfIntersect, VECTOR point)
{
    CrossProduct(p1->planeEquation, p2->planeEquation, axisOfIntersect);

    float den = DotProduct(axisOfIntersect, axisOfIntersect);

    if (den < EPSILON) return NOCOLLISION;

    float invDen = 1.0f / den;

    VECTOR cross1, cross2;

    VectorScaleXYZ(cross1, p2->planeEquation, -p1->planeEquation[3]);
    VectorScaleXYZ(cross2, p1->planeEquation, -p2->planeEquation[3]);

    VectorSubtractXYZ(cross1, cross2, cross1);

    CrossProduct(cross1, axisOfIntersect, cross1);

    VectorScaleXYZ(point, cross1, invDen);

    return COLLISION;
}

static int PlaneRotatedBox(VECTOR right, VECTOR up, VECTOR forward, VECTOR planeEquation,
                            VECTOR half, VECTOR center)
{
    VECTOR dots;
    dots[0] = Abs(DotProduct(planeEquation, right));
    dots[1] = Abs(DotProduct(planeEquation, up));
    dots[2] = Abs(DotProduct(planeEquation, forward));
    
    float r = DotProduct(half, dots);
    float s = DotProduct(planeEquation, center) + planeEquation[3];
    if (Abs(s) <= r) return COLLISION;
    return NOCOLLISION;
}

int PlaneOBBCollision(Plane *p, BoundingOrientBox *box)
{
    return PlaneRotatedBox( box->axes[0], box->axes[1], 
                            box->axes[2], p->planeEquation, 
                            box->halfwidths, box->center );
}


int PlaneRotatedAABBCollision(Plane *p, BoundingBox *box, VECTOR right, VECTOR up, VECTOR forward)
{
    VECTOR center, half, dots;
    FindCenterAndHalfAABB(box, center, half);
    return PlaneRotatedBox(right, up, forward, p->planeEquation, half, center);
}

int PlaneAABBCollision(Plane *plane, BoundingBox *box)
{
    VECTOR center, half;
    FindCenterAndHalfAABB(box, center, half);
    float r, s;
    asm __volatile__(
        "lqc2 $vf1, 0x00(%2)\n"
        "lqc2 $vf2, 0x00(%3)\n"
        "lqc2 $vf4, 0x00(%4)\n"
        "vmul.xyz $vf3, $vf1, $vf4\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "qmfc2 %1, $vf3\n"
        "vabs.xyz $vf1, $vf1\n"
        "vmul.xyz $vf3, $vf1, $vf2\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "qmfc2 %0, $vf3\n"
        : "=r"(r), "=r"(s)
        : "r"(plane->planeEquation), "r"(half), "r"(center)
        : "memory");

    if (Abs(s + plane->planeEquation[3]) <= r) return COLLISION;

    return NOCOLLISION;
}

void FindCenterOfVBO(void *collisionData, int type, VECTOR center)
{
    
    switch(type)
    {
        case VBO_FIT:
        case VBO_FIXED:
            BoundingBox *box = (BoundingBox *)collisionData;
            VectorAddXYZ(box->top, box->bottom, center);
            VectorScaleXYZ(center, center, 0.5f);
            break;
        case VBO_SPHERE:
            BoundingSphere *sphere = (BoundingSphere *)collisionData;
            VectorCopy(center, sphere->center);
            break;
        default:
            ERRORLOG("Invalid OOB type %d", type);
            break;
    }
}

int CheckCollision(GameObject *obj1, GameObject *obj2, ...)
{
    int ret = 0, firstcondition = 0;
    va_list vectorArgs;
    va_start(vectorArgs, obj2);
    ObjectBounds *obb1 = obj1->vboContainer;
    ObjectBounds *obb2 = obj2->vboContainer;
    if (obb1->type == VBO_FIT && obb2->type == VBO_FIT)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->vbo;
        BoundingBox *box2 = (BoundingBox *)obb2->vbo;
        VECTOR top1, bottom1, move;
        VectorCopy(move, va_arg(vectorArgs, float *));
        VectorScaleXYZ(move, move, 0.25f);
        VectorAddXYZ(box1->top, move, top1);
        VectorAddXYZ(box1->bottom, move, bottom1);

        ret = AABBCollision(top1, bottom1, box2->top, box2->bottom);
    }
    else if (obb1->type == VBO_FIXED && obb2->type == VBO_FIT)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->vbo;
        BoundingBox *box2 = (BoundingBox *)obb2->vbo;
        VECTOR newAxisX, newAxisY, newAxisZ, pos, outCenter1, outHalf1, outCenter2, outHalf2, boxVector, obj1Scales;

        GetScaleVectorLTM(obj1->ltm, obj1Scales);

        VectorCopy(pos, va_arg(vectorArgs, float *));
        VectorCopy(newAxisX, va_arg(vectorArgs, float *));
        VectorCopy(newAxisY, va_arg(vectorArgs, float *));
        VectorCopy(newAxisZ, va_arg(vectorArgs, float *));

        FindCenterAndHalfRotatedAABB(box1, pos, obj1Scales, newAxisX, newAxisY, newAxisZ, outCenter1, outHalf1);
        FindCenterAndHalfAABB(box2, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, newAxisX, newAxisY, newAxisZ, right, up, forward);
    }
    else if (obb1->type == VBO_FIXED && obb2->type == VBO_FIXED)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->vbo;
        BoundingBox *box2 = (BoundingBox *)obb2->vbo;
        VECTOR newAxisX, newAxisY, newAxisZ, pos, outCenter1, outHalf1, outCenter2, outHalf2, boxVector, obj1Scales, obj2Scales;
        VECTOR *obj2Pos, *obj2Up, *obj2Forward, *obj2Right;

        obj2Pos = GetPositionVectorLTM(obj2->ltm);
        obj2Right = GetRightVectorLTM(obj2->ltm);
        obj2Forward = GetForwardVectorLTM(obj2->ltm);
        obj2Up = GetUpVectorLTM(obj2->ltm);

        GetScaleVectorLTM(obj1->ltm, obj1Scales);
        GetScaleVectorLTM(obj2->ltm, obj2Scales);

        VectorCopy(pos, va_arg(vectorArgs, float *));
        VectorCopy(newAxisX, va_arg(vectorArgs, float *));
        VectorCopy(newAxisY, va_arg(vectorArgs, float *));
        VectorCopy(newAxisZ, va_arg(vectorArgs, float *));

        FindCenterAndHalfRotatedAABB(box1, pos, obj1Scales, newAxisX, newAxisY, newAxisZ, outCenter1, outHalf1);
        FindCenterAndHalfRotatedAABB(box2, *obj2Pos, obj2Scales, *obj2Right, *obj2Up, *obj2Forward, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, newAxisX, newAxisY, newAxisZ, *obj2Right, *obj2Up, *obj2Forward);
    }
    else if (obb1->type == VBO_FIT && obb2->type == VBO_FIXED)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->vbo;
        BoundingBox *box2 = (BoundingBox *)obb2->vbo;
        VECTOR top1, bottom1, move;
        VECTOR outCenter1, outHalf1, outCenter2, outHalf2, boxVector, obj2Scales;
        VECTOR *obj2Pos, *obj2Up, *obj2Forward, *obj2Right;

        obj2Pos = GetPositionVectorLTM(obj2->ltm);
        obj2Right = GetRightVectorLTM(obj2->ltm);
        obj2Forward = GetForwardVectorLTM(obj2->ltm);
        obj2Up = GetUpVectorLTM(obj2->ltm);
        GetScaleVectorLTM(obj2->ltm, obj2Scales);

        VectorCopy(move, va_arg(vectorArgs, float *));
        VectorScaleXYZ(move, move, 1.5f);
        VectorAddXYZ(box1->top, move, top1);
        VectorAddXYZ(box1->bottom, move, bottom1);
        FindCenterAndHalfAABB(box1, outCenter1, outHalf1);
        FindCenterAndHalfRotatedAABB(box2, *obj2Pos, obj2Scales, *obj2Right, *obj2Up, *obj2Forward, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, right, up, forward, *obj2Right, *obj2Up, *obj2Forward);
    } 
    else if (obb1->type == VBO_SPHERE && obb2->type == VBO_SPHERE)
    {
        ret = SphereCollision((BoundingSphere *)obb1->vbo, (BoundingSphere *)obb2->vbo);
    }
    else if ((firstcondition = (obb1->type == VBO_FIT && obb2->type == VBO_SPHERE)) || (obb1->type == VBO_SPHERE  && obb2->type == VBO_FIT))
    {
        BoundingBox *box;
        BoundingSphere *sphere;
        if (firstcondition)
        {
            box = (BoundingBox*)obb1->vbo;
            sphere = (BoundingSphere*)obb2->vbo;
        }
        else 
        {
            box = (BoundingBox*)obb2->vbo;
            sphere = (BoundingSphere*)obb1->vbo;
        }

        float d = SqrDistFromAABB(sphere->center, box);
        
        if (d <= sphere->radius * sphere->radius) ret = COLLISION;
        else ret = NOCOLLISION;
    } else if ((firstcondition = (obb1->type == VBO_FIXED && obb2->type == VBO_SPHERE)) || (obb1->type == VBO_SPHERE  && obb2->type == VBO_FIXED))
    {
        VECTOR half, pos, right, up, forward, center;
        BoundingBox *box;
        BoundingSphere *sphere;
        if (firstcondition)
        {
            box = (BoundingBox*)obb1->vbo;
            sphere = (BoundingSphere*)obb2->vbo;
        }
        else 
        {
            box = (BoundingBox*)obb2->vbo;
            sphere = (BoundingSphere*)obb1->vbo;
        }
    }

    va_end(vectorArgs);
    return ret;
}

int PerformSAT(VECTOR pos, VECTOR half1, VECTOR half2, VECTOR xAxis1, VECTOR yAxis1, VECTOR zAxis1, VECTOR xAxis2, VECTOR yAxis2, VECTOR zAxis2)
{
    // create 9 axes for all 15
    VECTOR x1x2, x1y2, x1z2;
    VECTOR y1x2, y1y2, y1z2;
    VECTOR z1x2, z1y2, z1z2;
    CrossProduct(xAxis1, xAxis2, x1x2);
    CrossProduct(xAxis1, yAxis2, x1y2);
    CrossProduct(xAxis1, zAxis2, x1z2);

    CrossProduct(yAxis1, xAxis2, y1x2);
    CrossProduct(yAxis1, yAxis2, y1y2);
    CrossProduct(yAxis1, zAxis2, y1z2);

    CrossProduct(zAxis1, xAxis2, z1x2);
    CrossProduct(zAxis1, yAxis2, z1y2);
    CrossProduct(zAxis1, zAxis2, z1z2);

    int ret = (CheckSeparatingPlane(pos, xAxis1, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, yAxis1, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, zAxis1, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, xAxis2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, yAxis2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, zAxis2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, x1x2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, x1y2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, x1z2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, y1x2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, y1y2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, y1z2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, z1x2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, z1y2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2) ||
               CheckSeparatingPlane(pos, z1z2, half1, half2, xAxis1, yAxis1, zAxis1, xAxis2, yAxis2, zAxis2));

    if (ret == 0)
    {
        return 1;
    }

    return 0;
}

int CheckSeparatingPlane(VECTOR pos, VECTOR plane, VECTOR half1, VECTOR half2, VECTOR xAxis1, VECTOR yAxis1, VECTOR zAxis1, VECTOR xAxis2, VECTOR yAxis2, VECTOR zAxis2)
{
    int ret = 0;
    VECTOR xProj1, yProj1, zProj1;
    VECTOR xProj2, yProj2, zProj2;

    VectorScaleXYZ(xProj1, xAxis1, half1[0]);
    VectorScaleXYZ(yProj1, yAxis1, half1[1]);
    VectorScaleXYZ(zProj1, zAxis1, half1[2]);

    VectorScaleXYZ(xProj2, xAxis2, half2[0]);
    VectorScaleXYZ(yProj2, yAxis2, half2[1]);
    VectorScaleXYZ(zProj2, zAxis2, half2[2]);

    float pdp = Abs(DotProduct(pos, plane));
    float x1dp = Abs(DotProduct(xProj1, plane));
    float y1dp = Abs(DotProduct(yProj1, plane));
    float z1dp = Abs(DotProduct(zProj1, plane));
    float x2dp = Abs(DotProduct(xProj2, plane));
    float y2dp = Abs(DotProduct(yProj2, plane));
    float z2dp = Abs(DotProduct(zProj2, plane));

    if (pdp > (x1dp + y1dp + z1dp + x2dp + y2dp + z2dp))
    {
        ret = 1;
    }

    return ret;
}

void MoveBox(BoundingBox *box, VECTOR move)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x00(%2)\n"
        "vadd.xyz $vf1, $vf3, $vf1\n"
        "vadd.xyz $vf2, $vf3, $vf2\n"
        "sqc2 $vf1, 0x00(%0) \n"
        "sqc2 $vf2, 0x00(%1) \n"
        :
        : "r"(box->top), "r"(box->bottom), "r"(move)
        : "memory");
}

void FindCenterAndHalfRotatedAABB(BoundingBox *box, VECTOR pos, VECTOR scale, VECTOR xAxis, VECTOR yAxis, VECTOR zAxis, VECTOR outCenter, VECTOR outHalf)
{
    VECTOR center, worldTop, worldBottom;
    MATRIX world;

    CreateWorldMatrixFromVectors(pos, yAxis, zAxis, xAxis, scale, world);
    MatrixVectorMultiply(worldTop, world, box->top);
    MatrixVectorMultiply(worldBottom, world, box->bottom);

    VectorAddXYZ(worldTop, worldBottom, center);
    VectorScaleXYZ(outCenter, center, 0.5f);

    outHalf[0] = Abs(worldTop[0] - outCenter[0]);
    outHalf[1] = Abs(worldTop[1] - outCenter[1]);
    outHalf[2] = Abs(worldTop[2] - outCenter[2]);
}

void FindCenterAndHalfAABB(BoundingBox *box, VECTOR outCenter, VECTOR outHalf)
{
    VECTOR center;
    VectorAddXYZ(box->top, box->bottom, center);
    VectorScaleXYZ(outCenter, center, 0.5f);

    outHalf[0] = Abs(box->top[0] - outCenter[0]);
    outHalf[1] = Abs(box->top[1] - outCenter[1]);
    outHalf[2] = Abs(box->top[2] - outCenter[2]);
}

void FindAABBMaxAndMinVerticesVU0(GameObject *obj)
{
    if (!(obj->vboContainer->type == VBO_FIT || obj->vboContainer->type == VBO_FIXED))
    {
        ERRORLOG("Use Max and Min update for non-OBB bounding box");
        return;
    }
    VECTOR *verts = obj->vertexBuffer.meshData[MESHVERTICES]->vertices;
    u32 vertexCount = obj->vertexBuffer.meshData[MESHVERTICES]->vertexCount;
    MATRIX world;

    CreateWorldMatrixLTM(obj->ltm, world);
    BoundingBox *bounds = (BoundingBox *)obj->vboContainer->vbo;

    bounds->top[0] = bounds->top[1] = bounds->top[2] = FLT_MIN;
    bounds->bottom[0] = bounds->bottom[1] = bounds->bottom[2] = FLT_MAX; 

    u32 offset = 0;

    asm __volatile__(
        "lqc2 $vf1, 0x00(%0)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        :
        : "r"(bounds->top), "r"(bounds->bottom)
        : "memory");

    if (obj->vboContainer->type == VBO_FIT)
    {
        asm __volatile__(
            "lqc2 $vf4, 0x00(%0)\n"
            "lqc2 $vf5, 0x10(%0)\n"
            "lqc2 $vf6, 0x20(%0)\n"
            "lqc2 $vf7, 0x30(%0)\n"
            :
            : "r"(world)
            : "memory");

        while (vertexCount-- > 0)
        {
            asm __volatile__(
                "lqc2 $vf3, 0x00(%0)\n"
                "vmulax.xyzw $ACC, $vf4, $vf3\n"
                "vmadday.xyzw $ACC, $vf5, $vf3\n"
                "vmaddaz.xyzw $ACC, $vf6, $vf3\n"
                "vmaddw.xyzw $vf3, $vf7, $vf3\n"
                "vmax.xyz $vf1, $vf1, $vf3\n"
                "vmini.xyz $vf2, $vf2, $vf3\n"
                :
                : "r"(verts[offset++])
                : "memory");
        }
    } else {
        while (vertexCount-- > 0)
        {
            asm __volatile__(
                "lqc2 $vf3, 0x00(%0)\n"
                "vmax.xyz $vf1, $vf1, $vf3\n"
                "vmini.xyz $vf2, $vf2, $vf3\n"
                :
                : "r"(verts[offset++])
                : "memory");
        }
    }

    asm __volatile__(
        "sqc2 $vf1, 0x00(%0) \n"
        "sqc2 $vf2, 0x00(%1) \n"
        :
        : "r"(bounds->top), "r"(bounds->bottom)
        : "memory");
    bounds->top[3] = bounds->bottom[3] = 1.0f;
}

void ClosestPointToAABB(VECTOR p, BoundingBox *box, VECTOR out)
{
    asm __volatile__(
        "lqc2 $vf1, 0x00(%3)\n"
        "lqc2 $vf2, 0x00(%0)\n"
        "lqc2 $vf3, 0x00(%1)\n"
        "vmax.xyz $vf1, $vf1, $vf3\n"
        "vmini.xyz $vf1, $vf2, $vf1\n"
        "sqc2 $vf1, 0x00(%2)\n"
        :
        : "r"(box->top), "r"(box->bottom), "r"(out), "r"(p)
        : "memory");
}

float SqrDistFromAABB(VECTOR p, BoundingBox *box)
{
    float sqDist = 0.0f;
    asm __volatile__(
        "lqc2 $vf1, 0x00(%3)\n"
        "lqc2 $vf2, 0x00(%1)\n"
        "lqc2 $vf3, 0x00(%2)\n"
        "vmini.xyz $vf7, $vf1, $vf3\n"
        "vmax.xyz $vf6, $vf2, $vf1\n"
        "vsub.xyz $vf4, $vf3, $vf7\n"
        "vsub.xyz $vf5, $vf6, $vf2\n"
        "vmul.xyz $vf4, $vf4, $vf4\n"
        "vmul.xyz $vf5, $vf5, $vf5\n"
        "vaddy.x $vf5x, $vf5x, $vf5y\n"
        "vaddz.x $vf5x, $vf5x, $vf5z\n"
        "vaddy.x $vf4x, $vf4x, $vf4y\n"
        "vaddz.x $vf4x, $vf4x, $vf4z\n"
        "vadd.x $vf5, $vf4, $vf5\n"
        "qmfc2 %0, $vf5"
        : "=r"(sqDist)
        : "r"(box->top), "r"(box->bottom), "r"(p)
        : "memory");
    return sqDist;
}

float SqDistToOBB(VECTOR p, VECTOR center, VECTOR halfwidths)
{
    float dist = 0.0f;
    VECTOR d;
    VectorSubtractXYZ(p, center, d);

    for (int i = 0; i<3; i++)
    {
        float duh = d[i];
        float excess = 0.0f;
        float hw = halfwidths[i];
        if (duh < -hw) 
        {
            excess = hw + duh;
        }
        else if (duh > hw)
        {
            excess = duh - hw;
        } 
        dist += excess * excess;
    }

    return dist;

}

void ClosestPointToOBB(VECTOR p, VECTOR center, VECTOR halfWidths, VECTOR q)
{
    VECTOR d, haflwidthneg;
    VectorSubtractXYZ(p, center, d);
    VectorScaleXYZ(haflwidthneg, halfWidths, -1.0f);

    asm __volatile__(
        "lqc2 $vf4, 0x00(%3)\n" //halfwidths
        "lqc2 $vf5, 0x00(%4)\n" //halfwidthsneg
        "lqc2 $vf6, 0x00(%6)\n" //dists
        "lqc2 $vf7, 0x00(%0)\n" //q
        "vmini.xyz $vf6, $vf6, $vf4\n"
        "vmax.xyz $vf6, $vf6, $vf5\n"
        "vadd.xyz $vf7, $vf6, $vf7\n"
        "sqc2 $vf7, 0x00(%5)\n"
        :
        : "r"(center), "r"(up), "r"(forward), "r"(halfWidths), "r"(haflwidthneg), "r"(q), "r"(d)
        : "memory");
}

static void MostSeparatedPointsOnAABB(int *min, int *max, VECTOR *verts, u32 vertCount);
static void ExpandSphere(VECTOR center, float *radius, VECTOR *verts, u32 vertCount, int randomize);

void ComputeBoundingSphereIterative(GameObject *obj)
{
#define ITERATIONS 8
    srand(getTimeMs(g_Manager.timer));
    ComputeBoundingSphere(obj);
    BoundingSphere *mainSphere = (BoundingSphere*)obj->vboContainer->vbo;
    BoundingSphere checkSphere = *mainSphere;
    VECTOR *verts = obj->vertexBuffer.meshData[MESHVERTICES]->vertices;
    u32 vertexCount = obj->vertexBuffer.meshData[MESHVERTICES]->vertexCount;
    for (int i = 0; i<ITERATIONS; i++)
    {
        checkSphere.radius = checkSphere.radius * 0.95f;
        ExpandSphere(checkSphere.center, &checkSphere.radius, verts, vertexCount, 1);
        if (checkSphere.radius < mainSphere->radius)
        {
            *mainSphere = checkSphere;
        }

    }
}

void ComputeBoundingSphere(GameObject *obj)
{
    VECTOR *verts = obj->vertexBuffer.meshData[MESHVERTICES]->vertices;
    u32 vertexCount = obj->vertexBuffer.meshData[MESHVERTICES]->vertexCount;
    BoundingSphere *sphere = (BoundingSphere*)obj->vboContainer->vbo;
    int min, max;
    MostSeparatedPointsOnAABB(&min, &max, verts, vertexCount);

    VECTOR temp;
    VectorAddXYZ(verts[min], verts[max], temp);
    VectorScaleXYZ(sphere->center, temp, 0.5f);
    VectorSubtractXYZ(verts[max], sphere->center, temp);
    sphere->radius = Sqrt(DotProduct(temp, temp));
    ExpandSphere(sphere->center, &sphere->radius, verts, vertexCount, 0);
}

static void ExpandSphere(VECTOR center, float *radius, VECTOR *verts, u32 vertCount, int randomize)
{
    VECTOR d;
    float temp = *radius * *radius;
    for (int i = 0; i<vertCount; i++)
    {
        int j = i;
        if (randomize)
        {
            int size = vertCount - (i+1);
            j = (rand() % size) + (i+1); 
        }
        VectorSubtractXYZ(verts[j], center, d);
        float dist2 = DotProduct(d, d);
        if (dist2 < temp)
        {
            dist2 = Sqrt(dist2);
            temp = (*radius + dist2) * 0.5f;
            float k = (temp - *radius) / dist2;
            *radius = temp;
            temp *= temp;
            VectorScaleXYZ(d, d, k);
            VectorAddXYZ(center, d, center);
        }
    }
}

static void MostSeparatedPointsOnAABB(int *min, int *max, VECTOR *verts, u32 vertCount)
{
    int mxX = 0, mnX = 0, mxY = 0, mnY = 0, mxZ = 0, mnZ = 0;
    for (int i = 1; i < vertCount; i++)
    {
        // Check for minimum and maximum in X dimension
        if (verts[mnX][0] > verts[i][0])
            mnX = i;
        if (verts[mxX][0] < verts[i][0])
            mxX = i;

        // Check for minimum and maximum in Y dimension
        if (verts[mnY][1] > verts[i][1])
            mnY = i;
        if (verts[mxY][1] < verts[i][1])
            mxY = i;

        // Check for minimum and maximum in Z dimension
        if (verts[mnZ][2] > verts[i][2])
            mnZ = i;
        if (verts[mxZ][2] < verts[i][2])
            mxZ = i;
    }

    VECTOR xSub, ySub, zSub;
    VectorSubtractXYZ(verts[mxX], verts[mnX], xSub);
    VectorSubtractXYZ(verts[mxY], verts[mnY], ySub);
    VectorSubtractXYZ(verts[mxZ], verts[mnZ], zSub);

    float distXs = DotProduct(xSub, xSub);
    float distYs = DotProduct(ySub, ySub);
    float distZs = DotProduct(zSub, zSub);

    *min = mnX;
    *max = mxX;
    if (distYs > distXs && distYs > distZs)
    {
        *max = mxY;
        *min = mnY;
    }

    if (distZs > distXs && distZs > distYs)
    {
        *max = mxZ;
        *min = mnZ;
    }
}