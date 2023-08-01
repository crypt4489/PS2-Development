#include "math/ps_misc.h"

#include <stdlib.h>
#include <string.h>

#include "math/ps_fast_maths.h"
#include "math/ps_quat.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"



void dump_packet(qword_t *q, int max, int usefloat)
{
    qword_t *iter = q;
    ; //= obj->pipeline_dma;
    int i = 0;
    while (iter->sw[0] != DMA_DCODE_END && i < max)
    {
        if (usefloat)
        {
            float f = ((float *)iter->sw)[3];
            float f1 = ((float *)iter->sw)[0];
            float f2 = ((float *)iter->sw)[1];
            float f3 = ((float *)iter->sw)[2];
            DEBUGLOG("%d %f %f %f %f", i, f1, f2, f3, f);
        }
        else
        {
            DEBUGLOG("%d %x %x %x %x", i, iter->sw[0], iter->sw[1], iter->sw[2], iter->sw[3]);
        }
        iter++;
        i++;
    }
    DEBUGLOG("%d", i);

    DEBUGLOG("________________________________");
}

void CreateGridVectors(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int v_count = N * M;

    // int faceCount = 2 * (N - 1) * (M - 1);

    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (N - 1);
    float dy = depth / (M - 1);

    // float du = 1.0f / (N - 1);
    // float dv = 1.0f / (M - 1);

    VECTOR *vertices = (VECTOR *)malloc(sizeof(VECTOR) * v_count);

    int index = 0;
    float z, x;
    for (int i = 0; i < M; i++)
    {
        z = halfDepth - dy * i;
        for (int j = 0; j < N; j++)
        {
            x = -halfWidth + dx * j;
            index = i * N + j;
            vertices[index][0] = x;
            vertices[index][1] = 0.0f;
            vertices[index][2] = z;
            vertices[index][3] = 1.0f;
        }
    }

    // go->vertices = (VECTOR*)malloc(sizeof(VECTOR) * index_count);

    for (int i = 0; i < buffer->vertexCount; i++)
    {
        int index = buffer->indices[i];
        VectorCopy(buffer->vertices[i], vertices[index]);
        //  VectorCopy(go->texCoords[i], uvs[index]);
        // DEBUGLOG("here %d\n", index);
    }

    free(vertices);

    // return go->vertices;
}

void CreateGridUVS(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int v_count = N * M;

    float du = 1.0f / (N - 1);
    float dv = 1.0f / (M - 1);

    VECTOR *uvs = (VECTOR *)malloc(sizeof(VECTOR) * v_count);

    int index = 0;
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            index = i * N + j;
            uvs[index][0] = j * du;
            uvs[index][1] = ((M - 1) - i) * dv;
            uvs[index][2] = 1.0f;
            uvs[index][3] = 0.0f;
        }
    }

    for (int i = 0; i < buffer->vertexCount; i++)
    {
        int index = buffer->indices[i];
        VectorCopy(buffer->texCoords[i], uvs[index]);
    }

    free(uvs);
}

MeshBuffers *CreateGrid(int N, int M, float depth, float width, MeshBuffers *buffer)
{

    int faceCount = 2 * (N - 1) * (M - 1);

    int index_count = faceCount * 3;

    buffer->vertexCount = index_count;

    DEBUGLOG("Grid indices count %d", buffer->vertexCount);

    buffer->indices = (u32 *)malloc(sizeof(int) * index_count);
    buffer->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
    buffer->vertices = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
    CreateGridIndices(N, M, width, depth, buffer);
    CreateGridVectors(N, M, width, depth, buffer);
    CreateGridUVS(N, M, width, depth, buffer);
    return buffer;
}

void CreateGridIndices(int N, int M, float depth, float width, MeshBuffers *buffer)
{
    int k = 0;
    for (int j = 0; j < M - 1; j++)
    {
        for (int i = 0; i < N - 1; i++)
        {
            buffer->indices[k] = j * N + i;
            buffer->indices[k + 1] = j * N + i + 1;
            buffer->indices[k + 2] = (j + 1) * N + i;
            buffer->indices[k + 3] = (j + 1) * N + i;
            buffer->indices[k + 4] = j * N + i + 1;
            buffer->indices[k + 5] = (j + 1) * N + i + 1;
            k += 6;
        }
    }
}



void Pathify(const char *name, char *file)
{
    int len = strlen(name);
    file[0] = 92;
    for (int i = 1; i <= len; i++)
    {
        file[i] = name[i - 1];
    }
    file[len + 1] = 59;
    file[len + 2] = 49;
    file[len + 3] = 0;
}

// append characters from input2 to input1 and store in output
void AppendString(const char *input1, const char *input2, char *output, u32 max)
{
    const char *iter = input1;
    char *outIter = output;

    u32 len = 0;

    while (*iter != 0 && len < max)
    {
        *outIter = *iter;
        outIter++;
        iter++;
        len++;
    }

    iter = input2;

    while (*iter != 0 && len < max)
    {
        *outIter = *iter;
        outIter++;
        iter++;
        len++;
    }

    if (len >= max)
    {
        ERRORLOG("error appending strings. too long given input size");
    }

    *outIter = 0;
}
