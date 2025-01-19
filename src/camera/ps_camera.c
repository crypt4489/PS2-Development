#include "camera/ps_camera.h"

#include <stdlib.h>

#include "math/ps_vector.h"
#include "physics/ps_movement.h"
#include "math/ps_fast_maths.h"
#include "gameobject/ps_gameobject.h"
#include "math/ps_quat.h"
#include "log/ps_log.h"
#include "math/ps_matrix.h"
#include "math/ps_plane.h"


Camera *g_DrawCamera = NULL;
extern VECTOR forward;
extern VECTOR up;
extern VECTOR right;

void CreateCameraQuat(Camera *cam, VECTOR quat)
{
    CreateQuatRotationAxes(*GetRightVectorLTM(cam->ltm), *GetUpVectorLTM(cam->ltm), *GetForwardVectorLTM(cam->ltm), quat);
}

Camera *EndRendering(Camera *cam)
{
    ClearDirtyLTM(cam->ltm);
    return cam;
}

void InitCameraVBOContainer(Camera *cam, float x, float y, float z, u32 type)
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

    cam->vboContainer = (ObjectBounds *)malloc(sizeof(ObjectBounds));

    cam->vboContainer->type = type;

    if (type == VBO_FIT || type == VBO_FIXED)
    {
        BoundingBox *box = (BoundingBox *)malloc(sizeof(BoundingBox));
        VectorCopy(box->top, top);
        VectorCopy(box->bottom, bot);
        cam->vboContainer->vbo = (void *)box;
    }
    else if (type == VBO_SPHERE)
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

    Normalize(camLook, camLook);

    CrossProduct(up, camLook, camRight);

    Normalize(camRight, camRight);

    CrossProduct(camLook, camRight, camUp);

    SetPositionVectorLTM(cam->ltm, pos);
    SetRotationVectorsLTM(cam->ltm, camUp, camRight, camLook);
}

void CleanCameraObject(Camera *cam)
{
    if (cam)
    {
        if (cam->vboContainer)
        {
            free(cam->vboContainer);
        }

        if (cam->frus[0])
        {
            free(cam->frus[0]);
        }

        if (cam->frus[1])
        {
            free(cam->frus[1]);
        }

        free(cam);
    }
}

void UpdateCameraMatrix(Camera *cam)
{
    VECTOR R, U, L, P;

    VectorCopy(P, *GetPositionVectorLTM(cam->ltm));
    VectorCopy(U, *GetUpVectorLTM(cam->ltm));
    VectorCopy(L, *GetForwardVectorLTM(cam->ltm));
    VectorCopy(R, *GetRightVectorLTM(cam->ltm));

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
    MatrixIdentity(temp_out);

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

    MatrixCopy(output, temp_out);
}
#define WIDTHEPSILON 0.275
void CreateCameraFrustum(Camera *cam)
{
    Frustum *frus = (Frustum *)malloc(sizeof(Frustum));

    float angle = DegToRad(cam->angle) * .5f;
    float tanAngle = Sin(angle) / Cos(angle);

    float nh;

    nh = cam->near * tanAngle;

    float nw;

    nw = (cam->aspect * cam->near) - WIDTHEPSILON;

    frus->nwidth = nw;
    frus->nheight = nh;

    VECTOR nearCenter, farCenter;
    VECTOR tempScale, tempOut, tempNormal;

    VectorScaleXYZ(tempScale, forward, -1.0f);

    VectorScaleXYZ(farCenter, tempScale, cam->far);
    // VectorSubtractXYZ(cam->pos, tempScale, farCenter);
    VectorScaleXYZ(nearCenter, tempScale, cam->near);
    // VectorSubtractXYZ(cam->pos, tempScale, nearCenter);
    tempOut[3] = tempScale[3] = nearCenter[3] = farCenter[3] = 1.0f;

    SetupPlane(tempScale, nearCenter, &frus->sides[0]);
    SetupPlane(forward, farCenter, &frus->sides[1]);

    // side top

    VectorScaleXYZ(tempOut, up, nh);
    VectorAddXYZ(nearCenter, tempOut, tempOut);

    // VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    Normalize(tempNormal, tempNormal);

    CrossProduct(tempNormal, right, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[2]);

    /// side bottom
    VectorScaleXYZ(tempOut, up, nh);
    VectorSubtractXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    Normalize(tempNormal, tempNormal);

    CrossProduct(right, tempNormal, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[3]);

    /// side right
    VectorScaleXYZ(tempOut, right, nw);
    VectorSubtractXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);
    Normalize(tempNormal, tempNormal);

    CrossProduct(tempNormal, up, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[4]);

    /// side left
    VectorScaleXYZ(tempOut, right, nw);
    VectorAddXYZ(nearCenter, tempOut, tempOut);

    //  VectorSubtractXYZ(tempOut, cam->pos, tempNormal);

    VectorCopyXYZ(tempOut, tempNormal);

    Normalize(tempNormal, tempNormal);

    CrossProduct(up, tempNormal, tempNormal);

    SetupPlane(tempNormal, tempOut, &frus->sides[5]);

    cam->frus[0] = frus;
    cam->frus[1] = (Frustum *)malloc(sizeof(Frustum));
    cam->frus[1]->nwidth = nw;
    cam->frus[1]->nheight = nh;



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
    cam->frus[0] = cam->frus[1] = NULL;
    return cam;
}

void FindPosAndNegVertexvbo(VECTOR topExtent, VECTOR bottomExtent, VECTOR normal, VECTOR pVertex, VECTOR nVertex)
{
    VectorCopy(nVertex, topExtent);
    VectorCopy(pVertex, bottomExtent);

    if (normal[0] > EPSILON)
    {
        pVertex[0] = topExtent[0];
        nVertex[0] = bottomExtent[0];
    }

    if (normal[1] > EPSILON)
    {
        pVertex[1] = topExtent[1];
        nVertex[1] = bottomExtent[1];
    }

    if (normal[2] > EPSILON)
    {
        pVertex[2] = topExtent[2];
        nVertex[2] = bottomExtent[2];
    }

}
int res = 0;
int TestObjectInCameraFrustum(Camera *cam, GameObject *obj)
{
    int ret = 1;

    res = 0;

    if (obj->vboContainer->type == VBO_FIXED || obj->vboContainer->type == VBO_FIT)
    {
        BoundingBox *box = (BoundingBox *)obj->vboContainer->vbo;

        VECTOR maxExtent, minExtent, topExtWorld, botExtWorld;

        if (obj->vboContainer->type == VBO_FIXED)
        {
            MatrixVectorMultiply(topExtWorld, obj->world, box->top);
            MatrixVectorMultiply(botExtWorld, obj->world, box->bottom);
            asm __volatile__(
                "lqc2 $vf1, 0x00(%0)\n"
                "lqc2 $vf2, 0x00(%1)\n"
                "vmax.xyz $vf3, $vf1, $vf2\n"
                "vmini.xyz $vf4, $vf1, $vf2\n"
                "sqc2 $vf3, 0x00(%2)\n"
                "sqc2 $vf4, 0x00(%3)\n"
            :
            : "r"(topExtWorld), "r"(botExtWorld), "r"(maxExtent), "r"(minExtent)
            : "memory");

        }
        else if (obj->vboContainer->type == VBO_FIT)
        {
           VectorCopy(topExtWorld, box->top);
           VectorCopy(botExtWorld, box->bottom);
        }
        
        for (int i = 0; i < 6; i++)
        {
            VECTOR pVert, nVert;
       
            FindPosAndNegVertexvbo(maxExtent, minExtent, cam->frus[1]->sides[i].planeEquation, pVert, nVert);
            
            if (DistanceFromPlane(cam->frus[1]->sides[i].planeEquation, pVert) < 0)
            {
                res = i+1;
                DEBUGLOG("%d", i+1);
                return 0;
            }

            if (DistanceFromPlane(cam->frus[1]->sides[i].planeEquation, nVert) < 0)
            {
                ret = 2;
            }
        }
    }
    else if (obj->vboContainer->type == VBO_SPHERE)
    {
        BoundingSphere *sphere = (BoundingSphere *)obj->vboContainer->vbo;

        VECTOR worldCenter;
        MatrixVectorTransform(worldCenter, obj->world, sphere->center);
        float r = sphere->radius ;

        for (int i = 0; i<6; i++)
        {
            float d = DistanceFromPlane(cam->frus[1]->sides[i].planeEquation, worldCenter);
            // determine if the sphere is behind the plane (negative halfspace)
            if (d < -r)
            {
                res = i+1;
                return 0;
            }
        }
    }
    return ret;
}

void DumpCameraFrustum(Camera *cam)
{
    MATRIX m;
    Frustum *frum = cam->frus[0];
    CreateCameraWorldMatrix(cam, m);
    VECTOR temp;
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