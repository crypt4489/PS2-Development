#include "geometry/ps_adjacency.h"
#include "math/ps_vector.h"
#include "log/ps_log.h"

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
       WingedTriangle **neighbor = &triangle->t1;
       
       
       for (int j = 0; j<3; j++)
       {
            u32 idx1 = facesIndex[j];
            u32 idx2 = facesIndex[(j+1)%3];
            UniqueVertex *vert = &unique[reverse[idx1]];
            *neighbor = NULL;
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
                    *neighbor = test;
                    break;
                }
            }
            neighbor++;
       }
    }

    /*

    WingedTriangle *tri = faces;

    DEBUGLOG("%d %d %d", tri->v1, tri->v2, tri->v3);
    WingedTriangle **tri2 = &tri->t1;
    for (int i = 0; i<3; i++)
    {
        tri = *tri2;
        DEBUGLOG("%d %d %d", tri->v1, tri->v2, tri->v3);
        tri2++;
    } */

    free(reverse);

    free(unique);

    return faces;
}