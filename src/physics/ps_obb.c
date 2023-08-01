#include "physics/ps_obb.h"

#include <stdarg.h>
#include <stdlib.h>

#include "system/ps_vumanager.h"
#include "system/ps_vif.h"
#include "gamemanager/ps_manager.h"
#include "math/ps_fast_maths.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

extern volatile u32 *vu1_data_address;

void DestroyOBB(ObjectBounds *bound)
{
    if (bound)
    {
        if (bound->obb)
            free(bound->obb);
        free(bound);
    }
}

void InitOBB(GameObject *obj, int type)
{
    float xMin, yMin, zMin, xMax, yMax, zMax;

    VECTOR v, top, bot;

    MATRIX world;

    obj->obb = (ObjectBounds *)malloc(sizeof(ObjectBounds));

    if (type == BBO_FIT || type == BBO_FIXED)
    {
        obj->obb->obb = (void *)malloc(sizeof(BoundingBox));
    }
    else if (type == BBO_SPHERE)
    {
        obj->obb->obb = (void *)malloc(sizeof(BoundingSphere));
    }

    CreateWorldMatrixLTM(obj->ltm, world);

    int vertCount = obj->vertexBuffer.vertexCount;

    VECTOR *ptr = obj->vertexBuffer.vertices;

    VectorCopy(v, *ptr);

    if (type == BBO_FIT)
    {
        MatrixVectorMultiply(v, world, v);
    }

    xMin = xMax = v[0];
    yMin = yMax = v[1];
    zMin = zMax = v[2];
    ptr++;

    for (int i = 1; i < vertCount; i++)
    {

        VectorCopy(v, *ptr);

        if (type == BBO_FIT)
        {
            MatrixVectorMultiply(v, world, v);
        }

        xMin = Min(xMin, v[0]);
        yMin = Min(yMin, v[1]);
        zMin = Min(zMin, v[2]);

        xMax = Max(xMax, v[0]);
        yMax = Max(yMax, v[1]);
        zMax = Max(zMax, v[2]);

        ptr++;
    }

    top[0] = xMax;
    top[1] = yMax;
    top[2] = zMax;
    top[3] = 1.0f;

    bot[0] = xMin;
    bot[1] = yMin;
    bot[2] = zMin;
    bot[3] = 1.0f;

    obj->obb->type = type;
    if (type == BBO_FIT || type == BBO_FIXED)
    {
        BoundingBox *box = (BoundingBox *)obj->obb->obb;

        VectorCopy(box->top, top);
        VectorCopy(box->bottom, bot);
    }
    else if (type == BBO_SPHERE)
    {
        BoundingSphere *sphere = (BoundingSphere *)obj->obb->obb;
        VECTOR add;
        VectorSubtractXYZ(top, bot, add);
        ScaleVectorXYZ(sphere->center, add, 0.5f);
        float mag = (top[0] - sphere->center[0]) * (top[0] - sphere->center[0]);
        mag +=  (top[1] - sphere->center[1]) * (top[1] - sphere->center[1]);
        mag +=  (top[2] - sphere->center[2]) * (top[2] - sphere->center[2]);
        sphere->radius = Sqrt(mag);
        DEBUGLOG("SPHERE RADIUS AND CENTER %f", sphere->radius);
        DumpVector(sphere->center);
    }
}

void ReadOBBFromVU1(GameObject *obj)
{
    u32 *ptr = vu1_data_address + (14 * 4);
    Bin2Float val;

    BoundingBox *box = (BoundingBox *)obj->obb->obb;

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
        return 1;
    }
    return 0;
}

void FindCenterOfOBB(void *collisionData, int type, VECTOR center)
{
    if (type == BBO_FIT || type == BBO_FIXED)
    {
        BoundingBox *box = (BoundingBox *)collisionData;
        VectorSubtractXYZ(box->top, box->bottom, center);
        ScaleVectorXYZ(center, center, 0.5f);
    }
    else if (type == BBO_SPHERE)
    {
        BoundingSphere *sphere = (BoundingSphere *)collisionData;
        VectorCopy(center, sphere->center);
    }
}

int CheckCollision(GameObject *obj1, GameObject *obj2, ...)
{
    int ret = 0;
    va_list vectorArgs;
    va_start(vectorArgs, obj2);
    ObjectBounds *obb1 = obj1->obb;
    ObjectBounds *obb2 = obj2->obb;
    if (obb1->type == BBO_FIT && obb2->type == BBO_FIT)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->obb;
        BoundingBox *box2 = (BoundingBox *)obb2->obb;
        VECTOR top1, bottom1, move;
        VectorCopy(move, va_arg(vectorArgs, float *));
        ScaleVectorXYZ(move, move, 0.25f);
        VectorAddXYZ(box1->top, move, top1);
        VectorAddXYZ(box1->bottom, move, bottom1);

        ret = AABBCollision(top1, bottom1, box2->top, box2->bottom);
    }
    else if (obb1->type == BBO_FIXED && obb2->type == BBO_FIT)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->obb;
        BoundingBox *box2 = (BoundingBox *)obb2->obb;
        VECTOR newAxisX, newAxisY, newAxisZ, pos, outCenter1, outHalf1, outCenter2, outHalf2, boxVector, obj1Scales;

        GetScaleVectorLTM(obj1->ltm, obj1Scales);

        VectorCopy(pos, va_arg(vectorArgs, float *));
        VectorCopy(newAxisX, va_arg(vectorArgs, float *));
        VectorCopy(newAxisY, va_arg(vectorArgs, float *));
        VectorCopy(newAxisZ, va_arg(vectorArgs, float *));

        FindCenterAndHalfOBB(box1, pos, obj1Scales, newAxisX, newAxisY, newAxisZ, outCenter1, outHalf1);
        FindCenterAndHalfAABB(box2, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, newAxisX, newAxisY, newAxisZ, right, up, forward);
    }
    else if (obb1->type == BBO_FIXED && obb2->type == BBO_FIXED)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->obb;
        BoundingBox *box2 = (BoundingBox *)obb2->obb;
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

        FindCenterAndHalfOBB(box1, pos, obj1Scales, newAxisX, newAxisY, newAxisZ, outCenter1, outHalf1);
        FindCenterAndHalfOBB(box2, *obj2Pos, obj2Scales, *obj2Right, *obj2Up, *obj2Forward, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, newAxisX, newAxisY, newAxisZ, *obj2Right, *obj2Up, *obj2Forward);
    }
    else if (obb1->type == BBO_FIT && obb2->type == BBO_FIXED)
    {
        BoundingBox *box1 = (BoundingBox *)obb1->obb;
        BoundingBox *box2 = (BoundingBox *)obb2->obb;
        VECTOR top1, bottom1, move;
        VECTOR outCenter1, outHalf1, outCenter2, outHalf2, boxVector, obj2Scales;
        VECTOR *obj2Pos, *obj2Up, *obj2Forward, *obj2Right;

        obj2Pos = GetPositionVectorLTM(obj2->ltm);
        obj2Right = GetRightVectorLTM(obj2->ltm);
        obj2Forward = GetForwardVectorLTM(obj2->ltm);
        obj2Up = GetUpVectorLTM(obj2->ltm);
        GetScaleVectorLTM(obj2->ltm, obj2Scales);

        VectorCopy(move, va_arg(vectorArgs, float *));
        ScaleVectorXYZ(move, move, 1.5f);
        VectorAddXYZ(box1->top, move, top1);
        VectorAddXYZ(box1->bottom, move, bottom1);
        FindCenterAndHalfAABB(box1, outCenter1, outHalf1);
        FindCenterAndHalfOBB(box2, *obj2Pos, obj2Scales, *obj2Right, *obj2Up, *obj2Forward, outCenter2, outHalf2);
        VectorSubtractXYZ(outCenter1, outCenter2, boxVector);
        ret = PerformSAT(boxVector, outHalf1, outHalf2, right, up, forward, *obj2Right, *obj2Up, *obj2Forward);
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
    VectorCopy(xProj1, xAxis1);
    VectorCopy(yProj1, yAxis1);
    VectorCopy(zProj1, zAxis1);

    VectorCopy(xProj2, xAxis2);
    VectorCopy(yProj2, yAxis2);
    VectorCopy(zProj2, zAxis2);

    ScaleVectorXYZ(xProj1, xProj1, half1[0]);
    ScaleVectorXYZ(yProj1, yProj1, half1[1]);
    ScaleVectorXYZ(zProj1, zProj1, half1[2]);

    ScaleVectorXYZ(xProj2, xProj2, half2[0]);
    ScaleVectorXYZ(yProj2, yProj2, half2[1]);
    ScaleVectorXYZ(zProj2, zProj2, half2[2]);

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

void FindCenterAndHalfOBB(BoundingBox *box, VECTOR pos, VECTOR scale, VECTOR xAxis, VECTOR yAxis, VECTOR zAxis, VECTOR outCenter, VECTOR outHalf)
{
    VECTOR center, worldTop, worldBottom;
    MATRIX world;
    // MATRIX rot, trans, tempGlobal;
    /* MatrixIdentity(rot);
     MatrixIdentity(tempGlobal);
     MatrixIdentity(trans);
     CreateRotationAndCopyMatFromObjAxes(rot, yAxis, zAxis, xAxis);
     CreateTranslationMatrix(pos, trans);
     CreateWorldMatrix(tempGlobal, scale, rot, trans); */
    CreateWorldMatrixFromVectors(pos, yAxis, zAxis, xAxis, scale, world);
    MatrixVectorMultiply(worldTop, world, box->top);
    MatrixVectorMultiply(worldBottom, world, box->bottom);

    VectorSubtractXYZ(worldTop, worldBottom, center);
    ScaleVectorXYZ(center, center, 0.5f);
    VectorAddXYZ(worldBottom, center, center);

    outHalf[0] = Abs(worldTop[0] - center[0]);
    outHalf[1] = Abs(worldTop[1] - center[1]);
    outHalf[2] = Abs(worldTop[2] - center[2]);
    VectorCopy(outCenter, center);
}

void FindCenterAndHalfAABB(BoundingBox *box, VECTOR outCenter, VECTOR outHalf)
{
    VECTOR center;
    VectorSubtractXYZ(box->top, box->bottom, center);
    ScaleVectorXYZ(center, center, 0.5f);
    VectorAddXYZ(center, box->bottom, center);
    outHalf[0] = Abs(box->top[0] - center[0]);
    outHalf[1] = Abs(box->top[1] - center[1]);
    outHalf[2] = Abs(box->top[2] - center[2]);
    VectorCopy(outCenter, center);
}
