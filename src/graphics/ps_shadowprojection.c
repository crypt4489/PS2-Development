#include "graphics/ps_shadowprojection.h"

#include "system/ps_vif.h"
#include "system/ps_vumanager.h"
#include "gamemanager/ps_manager.h"

#include "gameobject/ps_gameobject.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "math/ps_plane.h"

extern blend_t blender;

extern VECTOR lightPos;

void find_bottom_top_planes(VECTOR top, VECTOR bottom)
{
    VECTOR tempTop, tempBot;
    if (top[0] >= bottom[0])
    {
        tempTop[0] = top[0];
        tempBot[0] = bottom[0];
    } else {
        tempTop[0] = bottom[0];
        tempBot[0] = top[0];
    }

     if (top[1] >= bottom[1])
    {
        tempTop[1] = top[1];
        tempBot[1] = bottom[1];
    } else {
        tempTop[1] = bottom[1];
        tempBot[1] = top[1];
    }

     if (top[2] >= bottom[2])
    {
        tempTop[2] = top[2];
        tempBot[2] = bottom[2];
    } else {
        tempTop[2] = bottom[2];
        tempBot[2] = top[2];
    }

    VectorCopy(bottom, tempBot);
    VectorCopy(top, tempTop);
}





void createShadowMatrix(VECTOR plane, VECTOR light_pos, MATRIX out)
{

    MATRIX matrix;
    float dot = DotProductFour(plane, light_pos);

    //INFOOG("float val of dot : %f", dot);

    MatrixIdentity(matrix);

    matrix[0] = dot - light_pos[0] * plane[0];
    matrix[4] = -light_pos[0] * plane[1];
    matrix[8] = -light_pos[0] * plane[2];
    matrix[12] = -light_pos[0] * plane[3];

    matrix[1] = -light_pos[1] * plane[0];
    matrix[5] = dot - light_pos[1] * plane[1];
    matrix[9] = -light_pos[1] * plane[2];
    matrix[13] = -light_pos[1] * plane[3];

    matrix[2] = -light_pos[2] * plane[0];
    matrix[6] = -light_pos[2] * plane[1];
    matrix[10] = dot - light_pos[2] * plane[2];
    matrix[14] = -light_pos[2] * plane[3];

    matrix[3] = -light_pos[3] * plane[0];
    matrix[7] = -light_pos[3] * plane[1];
    matrix[11] = -light_pos[3] * plane[2];
    matrix[15] = dot - light_pos[3] * plane[3];

    MatrixCopy(out, matrix);
}


#define epsilon 000001
#define THETA 10.0

int plane_collide_with_light_vector(VECTOR pointOnPoly, VECTOR lightPos, VECTOR planeEqu)
{

    VECTOR lightDir;
    VectorSubtractXYZ(lightPos,pointOnPoly, lightDir);
    float dot = DotProduct(planeEqu, lightDir);

    if (dot > epsilon+THETA)
    {
        return 1;
    }



    return 0;
}
int check_shadow_collides_with_plane(GameObject *currObj, VECTOR lightPos, VECTOR outPlane, VECTOR top, VECTOR bottom, int index)
{
    GameObject *currCollidePoly = NULL;
    if (!currCollidePoly )
    {
        return -1;
    }
    int count = currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount;
    int res = 0;
    int indexCount = index;
    MATRIX checkWorld, collideWorld;
    CreateWorldMatrixLTM(currObj->ltm, checkWorld);
    CreateWorldMatrixLTM(currCollidePoly->ltm, collideWorld);
    while(currCollidePoly)
    {
        int polyCollideIndyCount = currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount;
        VECTOR x1, y1, z1, plane_n, plane, tempBottom, tempTop;
        VectorCopy(x1, currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertices[0]);
        VectorCopy(y1, currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertices[1]);
        VectorCopy(z1, currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertices[2]);

        MatrixVectorMultiply(x1, collideWorld, x1);
        MatrixVectorMultiply(y1, collideWorld, y1);
        MatrixVectorMultiply(z1, collideWorld, z1);

        ComputeNormal(x1, y1, z1, plane_n);

        ComputePlane(x1, plane_n, plane);

      //  DumpVector(plane_n);

        for (int i = 0; i<count; i++)
        {
            VECTOR t;
            MatrixVectorMultiply(t, checkWorld, currObj->vertexBuffer.meshData[MESHTRIANGLES]->vertices[i]);
           res = plane_collide_with_light_vector(t, lightPos, plane_n);


           if (res == 1)
           {
              VectorCopy(outPlane, plane);
              MatrixVectorMultiply(tempTop, collideWorld, currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertices[0]);
              VectorCopy(top, tempTop);
               MatrixVectorMultiply(tempBottom, collideWorld, currCollidePoly->vertexBuffer.meshData[MESHTRIANGLES]->vertices[polyCollideIndyCount-1]);
              VectorCopy(bottom, tempBottom);
              return indexCount;
           }
        }

         if (res == 0)
           {
               //currCollidePoly = currCollidePoly->next;
               indexCount ++;
           }
    }

    return -1;

}
