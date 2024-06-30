#include "geometry/ps_adjacency.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"
#include "math/ps_plane.h"
#include <limits.h>

#include <stdlib.h>

typedef struct unique_vertex_t
{
    VECTOR *vec;
    u32 index;
    u32 faceCount;
    u32 faceIndices[64]; //just a temporary amount, linked list might be better
} UniqueVertex;

typedef struct tri_list_t
{

} TriList;


static int VertexUnique(VECTOR vertex, UniqueVertex *unique, u32 numUnique)
{
    for (int i = 0; i<numUnique; i++)
    {

        if (EqualVectors(vertex, *unique[i].vec))
        {
            return i;
        }
    }
    return -1;
}

FaceVertexTable ComputeFaceToVertexTable(VECTOR *vertices, u32 numVertices)
{

    u32 faceCount = numVertices/3;
    WingedTriangle *faces = (WingedTriangle*)malloc(sizeof(WingedTriangle) * faceCount);

    UniqueVertex *unique = (UniqueVertex*)malloc(sizeof(UniqueVertex) * numVertices);
    u32 *reverse = (u32*)malloc(4 * numVertices);
    

    u32 numUnique = 0;

    for (int i = 0, g = 0; i<numVertices; i+=3, g++)
    {
        u32 *faceIndex = &faces[g].v1;
        for (int j = i; j<i+3; j++)
        {
            int index = VertexUnique(vertices[j], unique, numUnique);
            if (index == -1)
            {
                unique[numUnique].index = j;
                unique[numUnique].vec = &vertices[j];
                unique[numUnique].faceCount = 0;
                reverse[j] = numUnique;
                index = numUnique;
                numUnique++;
            }
            *faceIndex = unique[index].index;
            faceIndex++;
            unique[index].faceIndices[unique[index].faceCount++] = g;
        }
    }


    for (int i = 0; i<faceCount; i++)
    {
       WingedTriangle *triangle = &faces[i];
       u32 *facesIndex = &triangle->v1;
       s32 *neighbor = &triangle->t1;
       
       
       for (int j = 0; j<3; j++)
       {
            u32 idx1 = facesIndex[j];
            u32 idx2 = facesIndex[(j+1)%3];
            UniqueVertex *vert = &unique[reverse[idx1]];
            *neighbor = -1;
            for (int h = 0; h<vert->faceCount; h++)
            {
                u32 faceIdx = vert->faceIndices[h];
                if (faceIdx == i)
                    continue;
                WingedTriangle *test = &faces[faceIdx];
                u32 cIdx = test->v1;
                u32 cIdx2 = test->v2;
                u32 cIdx3 = test->v3;
                
                if ((idx1 == cIdx && idx2 == cIdx2) || (idx1 == cIdx2 && idx2 == cIdx) ||
                    (idx1 == cIdx2 && idx2 == cIdx3) || (idx1 == cIdx3 && idx2 == cIdx2) ||
                    (idx1 == cIdx3 && idx2 == cIdx) || (idx1 == cIdx && idx2 == cIdx3))
                {
                    *neighbor = faceIdx;
                    break;
                }
            }
            neighbor++;
       }
       VECTOR diff1, diff2, cross;
       VectorSubtractXYZ(vertices[triangle->v2], vertices[triangle->v1], diff1);
       VectorSubtractXYZ(vertices[triangle->v3], vertices[triangle->v2], diff2);
       CrossProduct(diff1, diff2, cross);
       Normalize(cross, cross);
       ComputePlane(vertices[triangle->v1], cross, triangle->plane);

    }

    
/*
    WingedTriangle *tri = faces;
    WingedTriangle *tri3;
    DEBUGLOG("%d %d %d", tri->v1, tri->v2, tri->v3);
    DEBUGLOG("%d %d %d", tri->t1, tri->t2, tri->t3);
    s32 *tri2 = &tri->t1;
    for (int i = 0; i<3; i++)
    {
        tri3 = &faces[*tri2];
        DEBUGLOG("%d %d %d", tri3->v1, tri3->v2, tri3->v3);
        tri2++;
    } 
*/
    free(reverse);

    free(unique);

    return faces;
}

static u32 GetOppositeIndex(WingedTriangle *tri, u32 idx1, u32 idx2)
{
    u32 *vidx = &tri->v1;
    for (int j = 0; j<3; j++)
    {
        if (vidx[j] != idx1 && vidx[j] != idx2)
        {
            return vidx[j];
        }
    }
    ERRORLOG("Made it here in GetOppositeIndex");
    return INT_MAX;
}


VECTOR *CreateAdjacencyVertices(FaceVertexTable table, VECTOR *verts, u32 numVerts, u32 *numAdjVerts)
{
    u32 faceCount = numVerts / 3;
    *numAdjVerts = (faceCount * 6) + faceCount;
    VECTOR *adjVerts = (VECTOR*)malloc(sizeof(VECTOR) * *numAdjVerts);
    VECTOR *out = adjVerts;
    for (int i = 0; i<faceCount; i++)
    {
        WingedTriangle *tri = table+i;
        u32 *vidx = &tri->v1;
        s32 *tidx = &tri->t1;
        VECTOR *faceBools = out++;
        VECTOR *mainTriangle = out;
        VECTOR *oppo = out+3;
        u32 outCount = 6;
        for (int j = 0; j<3; j++)
        {
            if (tidx[j] == -1)
            {
                faceBools[0][j] = -1.0f;
                outCount -= 1;
                *numAdjVerts -= 1;
                VectorCopy(mainTriangle[0], verts[vidx[j]]);
                mainTriangle++;
                continue;
            }
            else
            {
                faceBools[0][j] = 1.0f;
            }    
            u32 v1 = vidx[j], v2 = vidx[(j+1)%3];
            u32 v3 = GetOppositeIndex(&table[tidx[j]], v1, v2);
            if (v3 == INT_MAX)
            {
                free(adjVerts);
                return NULL;
            }
            VectorCopy(mainTriangle[0], verts[v1]);
            mainTriangle++;
            
            VectorCopy(oppo[0], verts[v3]);
            oppo++;
        }
        *((u32*)&(faceBools[0][3])) = outCount; 
        out += outCount;
       // u32 what = (*(u32*)&faceBools[0][3]);
       // DEBUGLOG("%d %d", what, outCount);
    }


    return adjVerts;
}