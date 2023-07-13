#include "camera/ps_camera.h"
#include "math/ps_misc.h"
#include "physics/ps_movement.h"
#include "math/ps_fast_maths.h"
#include "gameobject/ps_gameobject.h"
#include "math/ps_quat.h"
#include "log/ps_log.h"
#include <malloc.h>
#include <stdlib.h>

Camera *g_DrawCamera = NULL;

void CreateCameraQuat(Camera *cam, VECTOR quat)
{
    CreateQuatRotationAxes(*GetRightVectorLTM(cam->ltm), *GetUpVectorLTM(cam->ltm), *GetForwardVectorLTM(cam->ltm), quat);
}

Camera *EndRendering(Camera *cam)
{
    ClearDirtyLTM(cam->ltm);
    return cam;
}

void InitCameraObb(Camera *cam, float x, float y, float z, u32 type)
{
    VECTOR top, bot;
    top[0] = x;
    top[1] = y;
    top[2] = z;
    top[3] = 1.0f;

    bot[0] = -x;
    bot[1] = -y;
    bot[2] = -z;
    bot[3] = 1.0f;

    cam->obb = (ObjectBounds *)malloc(sizeof(ObjectBounds));

    cam->obb->type = type;

    if (type == BBO_FIT || type == BBO_FIXED)
    {
        BoundingBox *box = (BoundingBox *)malloc(sizeof(BoundingBox));
        vector_copy(box->top, top);
        vector_copy(box->bottom, bot);
        cam->obb->obb = (void *)box;
    }
    else if (type == BBO_SPHERE)
    {
    }
}

void CameraLookAt(Camera *cam, VECTOR pos, VECTOR target, VECTOR up)
{
    VECTOR camLook, camRight, camUp;

    camLook[0] = -target[0] + pos[0];
    camLook[1] = -target[1] + pos[1];
    camLook[2] = -target[2] + pos[2];
    camLook[3] = -target[3] + pos[3];

    normalize(camLook, camLook);

    CrossProduct(up, camLook, camRight);

    normalize(camRight, camRight);

    CrossProduct(camLook, camRight, camUp);

    SetPositionVectorLTM(cam->ltm, pos);
    SetRotationVectorsLTM(cam->ltm, camUp, camRight, camLook);
}

void CleanCameraObject(Camera *cam)
{
    if (cam)
    {
        if (cam->obb)
        {
            free(cam->obb);
        }

        if (cam->frus)
        {
            free(cam->frus);
        }

        free(cam);
    }
}

void UpdateCameraMatrix(Camera *cam)
{
    VECTOR R, U, L, P;

    vector_copy(P, *GetPositionVectorLTM(cam->ltm));
    vector_copy(U, *GetUpVectorLTM(cam->ltm));
    vector_copy(L, *GetForwardVectorLTM(cam->ltm));
    vector_copy(R, *GetRightVectorLTM(cam->ltm));

    float x = -DotProduct(R, P);
    float y = -DotProduct(U, P);
    float z = -DotProduct(L, P);

    cam->view[0] = R[0];
    cam->view[4] = R[1];
    cam->view[8] = R[2];

    cam->view[1] = U[0];
    cam->view[5] = U[1];
    cam->view[9] = U[2];

    cam->view[2] = L[0];
    cam->view[6] = L[1];
    cam->view[10] = L[2];

    cam->view[12] = x;
    cam->view[13] = y;
    cam->view[14] = z;
    cam->view[15] = 1.0f;

    // DumpMatrix(temp_out);
}

typedef void (*LTM_array)(float *, float);

static LTM_array funcs[8] = {StrafeLTM, StrafeLTM, RotateYLTM, RotateYLTM, WalkLTM, WalkLTM, PitchLTM, PitchLTM};
static float dirs[8] = {-1.0f, +1.0f, +0.5f, -0.5f, -1.0f, +1.0f, +0.5f, -0.5f};

int HandleCamMovement(Camera *cam, u32 type)
{
    int ret = 0;
    if (type >= 1 && type <= 8)
    {
        funcs[type - 1](cam->ltm, dirs[type - 1]);
        ret = 1;
        SetDirtyLTM(cam->ltm);
    }
    return ret;
}

void CreateCameraWorldMatrix(Camera *cam, MATRIX output)
{
    MATRIX temp_out;

    temp_out[0] = cam->ltm[0];
    temp_out[1] = cam->ltm[1];
    temp_out[2] = cam->ltm[2];

    temp_out[4] = cam->ltm[4];
    temp_out[5] = cam->ltm[5];
    temp_out[6] = cam->ltm[6];

    temp_out[8] = cam->ltm[8];
    temp_out[9] = cam->ltm[9];
    temp_out[10] = cam->ltm[10];

    temp_out[12] = cam->ltm[12];
    temp_out[13] = cam->ltm[13];
    temp_out[14] = cam->ltm[14];
    temp_out[15] = 1.0f;

    matrix_copy(output, temp_out);
}

void CreateCameraFrustum(Camera *cam)
{
    Frustum *frus = (Frustum *)malloc(sizeof(Frustum));

    float angle = DegToRad(cam->angle) * .5f;
    float tanAngle = Sin(angle) / Cos(angle);

    float nh;

    nh = cam->near * tanAngle;

    float nw;

    nw = cam->aspect * cam->near;

    frus->nwidth = nw;
    frus->nheight = nh;

    VECTOR nearCenter, farCenter;
    VECTOR tempScale, tempOut, tempNormal;

    ScaleVectorXYZ(tempScale, forward, -1.0f);

    ScaleVectorXYZ(farCenter, tempScale, cam->far);
    // VectorSubtractXYZ(cam->pos, tempScale, farCenter);
    ScaleVectorXYZ(nearCenter, tempScale, cam->near);
    // VectorSubtractXYZ(cam->pos, tempScale, nearCenter);
    tempOut[3] = tempScale[3] = nearCenter[3] = farCenter[3] = 1.0f;

    SetupPlane(tempScale, nearCenter, &frus->sides[0]);
    SetupPlane(forward, farCenter, &frus->sides[1]);

    // side top

    ScaleVectorXYZ(tempOut, up, nh);
    VectorAddXYZ(nearCenter, tempOut, tempOut);

    // VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    normalize(tempNormal, tempNormal);

    CrossProduct(tempNormal, right, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[2]);

    /// side bottom
    ScaleVectorXYZ(tempOut, up, nh);
    VectorSubtractXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    normalize(tempNormal, tempNormal);

    CrossProduct(right, tempNormal, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[3]);

    /// side right
    ScaleVectorXYZ(tempOut, right, nw);
    VectorSubtractXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);
    normalize(tempNormal, tempNormal);

    CrossProduct(tempNormal, up, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[4]);

    /// side left
    ScaleVectorXYZ(tempOut, right, nw);
    VectorAddXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    normalize(tempNormal, tempNormal);

    CrossProduct(up, tempNormal, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[5]);

    cam->frus = frus;

    /*

        DumpVector(frus->sides[0].planeEquation);
       // frus->sides[0].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[0].pointInPlane);
    DEBUGLOG("---------------------------\n");

        DumpVector(frus->sides[1].planeEquation);
      //  frus->sides[1].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[1].pointInPlane);

    DEBUGLOG("---------------------------\n");

        DumpVector(frus->sides[2].planeEquation);
        frus->sides[2].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[2].pointInPlane);

    DEBUGLOG("---------------------------\n");

        DumpVector(frus->sides[3].planeEquation);
        frus->sides[3].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[3].pointInPlane);

        DEBUGLOG("---------------------------\n");
        DumpVector(frus->sides[4].planeEquation);
        frus->sides[4].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[4].pointInPlane);

    DEBUGLOG("---------------------------\n");

        DumpVector(frus->sides[5].planeEquation);
        frus->sides[5].pointInPlane[3] = 1.0f;
        DumpVector(frus->sides[5].pointInPlane);

        DEBUGLOG("---------------------------\n");

       // while(1) {}
    */
}

Camera *InitCamera(int width, int height, float near, float far, float aspect, float angle)
{
    Camera *cam = (Camera *)malloc(sizeof(Camera));
    cam->aspect = aspect;
    cam->far = far;
    cam->near = near;
    cam->width = width;
    cam->height = height;
    cam->angle = angle;
    cam->frus = NULL;
    return cam;
}

void FindPosAndNegVertexOBB(VECTOR topExtent, VECTOR bottomExtent, VECTOR normal, VECTOR pVertex, VECTOR nVertex)
{
    vector_copy(nVertex, topExtent);
    vector_copy(pVertex, bottomExtent);

    if (normal[0] > 0.001f)
    {
        pVertex[0] = topExtent[0];
        nVertex[0] = bottomExtent[0];
    }

    if (normal[1] > 0.001f)
    {

        pVertex[1] = topExtent[1];
        nVertex[1] = bottomExtent[1];
    }

    if (normal[2] > 0.001f)
    {
        pVertex[2] = topExtent[2];
        nVertex[2] = bottomExtent[2];
    }

}

int TestObjectInCameraFrustum(Camera *cam, GameObject *obj)
{
    MATRIX camMatrix, worldMatrix;

    int ret = 1;

    CreateWorldMatrixLTM(obj->ltm, worldMatrix);

    if (obj->obb->type == BBO_FIXED || obj->obb->type == BBO_FIT)
    {
        BoundingBox *box = (BoundingBox *)obj->obb->obb;

        VECTOR maxExtent, minExtent, topExtWorld, botExtWorld;

        if (obj->obb->type == BBO_FIXED)
        {
            MatrixVectorMultiply(topExtWorld, worldMatrix, box->top);
            MatrixVectorMultiply(botExtWorld, worldMatrix, box->bottom);

            CreateVector(Min(topExtWorld[0], botExtWorld[0]), Min(topExtWorld[1], botExtWorld[1]), Min(topExtWorld[2], botExtWorld[2]), 0.0f, minExtent);
            CreateVector(Max(topExtWorld[0], botExtWorld[0]), Max(topExtWorld[1], botExtWorld[1]), Max(topExtWorld[2], botExtWorld[2]), 0.0f, maxExtent);
        }
        else if (obj->obb->type == BBO_FIT)
        {

            CreateVector(Min(box->top[0], box->bottom[0]), Min(box->top[1], box->bottom[1]), Min(box->top[2], box->bottom[2]), 0.0f, minExtent);
            CreateVector(Max(box->top[0], box->bottom[0]), Max(box->top[1], box->bottom[1]), Max(box->top[2], box->bottom[2]), 0.0f, maxExtent);
        }


        CreateCameraWorldMatrix(cam, camMatrix);
        // DumpCameraFrustum(cam);
        for (int i = 0; i < 6; i++)
        {
            VECTOR pVert, nVert, tempPlane, tempNormal, tempPoint;

            Matrix3VectorMultiply(tempNormal, camMatrix, cam->frus->sides[i].planeEquation);
            MatrixVectorMultiply(tempPoint, camMatrix, cam->frus->sides[i].pointInPlane);

            normalize(tempNormal, tempNormal);

            ComputePlane(tempPoint, tempNormal, tempPlane);

            // DumpVector(tempPlane);
            FindPosAndNegVertexOBB(maxExtent, minExtent, tempNormal, pVert, nVert);

            if (DistanceFromPlane(tempPlane, pVert) < 0)
            {
                //DEBUGLOG("%d", i);
                return 0;
            }

            if (DistanceFromPlane(tempPlane, nVert) < 0)
            {
                ret = 2;
            }
        }
    }
    else if (obj->obb->type == BBO_SPHERE)
    {

    }
    return ret;
}

void DumpCameraFrustum(Camera *cam)
{
    MATRIX m;
    Frustum *frum = cam->frus;
    CreateCameraWorldMatrix(cam, m);
    VECTOR temp, temp1;
    Matrix3VectorMultiply(temp, m, frum->sides[0].planeEquation);
    DEBUGLOG("Front Normal");
    DumpVector(temp);
    DumpVector(frum->sides[0].planeEquation);
    Matrix3VectorMultiply(temp, m, frum->sides[1].planeEquation);
    DEBUGLOG("Back Normal");
    DumpVector(temp);
    DumpVector(frum->sides[1].planeEquation);
    Matrix3VectorMultiply(temp, m, frum->sides[2].planeEquation);
    DEBUGLOG("Top Normal");
    DumpVector(temp);
    DumpVector(frum->sides[2].planeEquation);
    Matrix3VectorMultiply(temp, m, frum->sides[3].planeEquation);
    DEBUGLOG("Bottom Normal");
    DumpVector(temp);
    DumpVector(frum->sides[3].planeEquation);
    Matrix3VectorMultiply(temp, m, frum->sides[4].planeEquation);
    DEBUGLOG("Right Normal");
    DumpVector(temp);
    DumpVector(frum->sides[4].planeEquation);
    DEBUGLOG("Left Normal");
    Matrix3VectorMultiply(temp, m, frum->sides[5].planeEquation);
    DumpVector(temp);
    DumpVector(frum->sides[5].planeEquation);
    DEBUGLOG("-----------------------------------");

    MatrixVectorMultiply(temp, m, frum->sides[0].pointInPlane);
    DEBUGLOG("Front Point");
    DumpVector(temp);
    MatrixVectorMultiply(temp, m, frum->sides[1].pointInPlane);
    DEBUGLOG("Back Point");
    DumpVector(temp);
    MatrixVectorMultiply(temp, m, frum->sides[2].pointInPlane);
    DEBUGLOG("Top Point");
    DumpVector(temp);
    MatrixVectorMultiply(temp, m, frum->sides[3].pointInPlane);
    DEBUGLOG("Bottom Point");
    DumpVector(temp);
    MatrixVectorMultiply(temp, m, frum->sides[4].pointInPlane);
    DEBUGLOG("Right Point");
    DumpVector(temp);
    DEBUGLOG("Left Point");
    MatrixVectorMultiply(temp, m, frum->sides[5].pointInPlane);
    DumpVector(temp);
    DEBUGLOG("-----------------------------------");
}