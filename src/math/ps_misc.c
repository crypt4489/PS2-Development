#include "math/ps_misc.h"

#include <stdlib.h>
#include <string.h>

#include "math/ps_fast_maths.h"
#include "math/ps_quat.h"
#include "math/ps_matrix.h"
#include "log/ps_log.h"
#include "math/ps_vector.h"
#include "gameobject/ps_ltm.h"
#include <kernel.h>
typedef struct zsort_pair_t
{
    float dist;
    u32 index;
} ZSortPair;
/*
static inline u32 BinarySearchZSort(float *dists, float element, u32 count)
{
    int left = 0, right = count - 1;
    int current = 0;
    while (left < right)
    {
        int middle = (right + left) >> 1;

        if (dists[middle] == element)
            return middle + 1;

        if (dists[middle] > element)
            right = middle - 1;

        if (dists[middle] < element)
            left = middle + 1;
    }

    return (element > dists[left]) ? (left + 1) : (left);
}

void InsertElementZSort(float *dists, u32 *indices, float element, u32 count)
{
    if (count == 0)
    {
        dists[0] = element;
        return;
    }

    int iter = count;

    int index = BinarySearchZSort(dists, element, count);

    while (iter >= index)
    {
        dists[iter + 1] = dists[iter];
        indices[iter + 1] = indices[iter];
        iter--;
    }

    dists[iter + 1] = element;
    indices[iter + 1] = count;
}
*/
int cmpfunc(const void *a, const void *b)
{
    ZSortPair *_a = (ZSortPair *)a;
    ZSortPair *_b = (ZSortPair *)b;

    if (_a->dist > _b->dist)
        return 1;

    if (_a->dist < _b->dist)
        return -1;

    return 0;
}
/*
static inline int QSortPartition(float *arr1, u32 *arr2, int low, int high)
{
    float pivot = arr1[high];

    int i = low - 1;
    float temp;
    for (int j = low; j <= high - 1; j++)
    {
        if (arr1[j] < pivot)
        {
            i++;
            temp = arr1[i];
            arr1[i] = arr1[j];
            arr1[j] = temp;
        }
    }
    temp = arr1[i + 1];
    arr1[i + 1] = arr1[high];
    arr1[high] = temp;
    return (i + 1);
}

static void ZSortQSort(float *arr1, u32 *arr2, int low, int high)
{
    if (low >= high)
        return;

    int part = QSortPartition(arr1, arr2, low, high);
    ZSortQSort(arr1, arr2, low, part - 1);
    ZSortQSort(arr1, arr2, part + 1, high);
}

static void ZSortMerge(float *arr1, u32* arr2, int left, int middle, int right)
{
    int i = left;
    int j = middle + 1;
    int k = 0;
    float temp[right - left + 1];
    while((i <= middle) && (j <= right))
    {
        if (arr1[i] < arr1[j])
            temp[k++] = arr1[i++];
        else 
            temp[k++] = arr1[j++];
    }

    while(j <= right)
    {
        temp[k++] = arr1[j++];
    }

    while(i <= middle)
    {
        temp[k++] = arr1[i++];
    }

    for (i = left, k = 0; i <= right; i++, k++)
        arr1[i] = temp[k];
}

void ZSortMergeSort(float *arr1, u32 *arr2, int left, int right)
{
    if (left < right)
    {
        int middle = (left + right) >> 1;
        ZSortMergeSort(arr1, arr2, left, middle);
        ZSortMergeSort(arr1, arr2, middle + 1, right);

        if (arr1[middle] > arr1[middle+1])
        {
            ZSortMerge(arr1, arr2, left, middle, right);
        }
    }
}
*/
void ZSort(GameObject *obj, Camera *cam)
{

    
    u32 triangleCount = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertexCount / 3;
   // float *distances = (float *)malloc(sizeof(float) * triangleCount);
   // u32 *indices = (u32 *)malloc(sizeof(u32) * triangleCount);

    ZSortPair *pairs = (ZSortPair*)malloc(sizeof(ZSortPair) * triangleCount);
    VECTOR *triangles = obj->vertexBuffer.meshData[MESHTRIANGLES]->vertices;
    const float onethird = 0.333333;
    VECTOR camPos;
    GetPositionVectorCopyLTM(cam->ltm, camPos);
    MATRIX m;
    CreateWorldMatrixLTM(obj->ltm, m);
    asm __volatile__(
        "qmtc2 %0, $vf5\n"
        "lqc2 $vf6, 0x00(%1)\n"
        "lqc2 $vf8, 0x00(%2)\n"
        "lqc2 $vf9, 0x10(%2)\n"
        "lqc2 $vf10, 0x20(%2)\n"
        "lqc2 $vf11, 0x30(%2)\n"
        :
        : "r"(onethird), "r"(camPos), "r"(m)
        : "memory");
    for (u32 i = 0; i < triangleCount; i++)
    {
        float dist;
        u32 index = i * 3;
        ZSortPair *pair = &pairs[i];
       asm __volatile__(
            "lqc2 $vf1, 0x00(%1)\n"
            "lqc2 $vf2, 0x00(%2)\n"
            "lqc2 $vf3, 0x00(%3)\n"
            "vmulax.xyzw		$ACC, $vf8, $vf1\n"
            "vmadday.xyzw	$ACC, $vf9, $vf1\n"
            "vmaddaz.xyzw	$ACC, $vf10, $vf1\n"
            "vmaddw.xyzw		$vf1, $vf11, $vf1\n"
            "vmulax.xyzw		$ACC, $vf8, $vf2\n"
            "vmadday.xyzw	$ACC, $vf9, $vf2\n"
            "vmaddaz.xyzw	$ACC, $vf10, $vf2\n"
            "vmaddw.xyzw		$vf2, $vf11, $vf2\n"
            "vmulax.xyzw		$ACC, $vf8, $vf3\n"
            "vmadday.xyzw	$ACC, $vf9, $vf3\n"
            "vmaddaz.xyzw	$ACC, $vf10, $vf3\n"
            "vmaddw.xyzw		$vf3, $vf11, $vf3\n"
            "vadd.xyz $vf4, $vf1, $vf2\n"
            "vadd.xyz $vf4, $vf4, $vf3\n"
            "vmul.xyz $vf4, $vf4, $vf5\n"
            "vsub.xyz $vf7, $vf6, $vf4\n"
            "vmul.xyz $vf7, $vf7, $vf7\n"
            "vaddx.y $vf7, $vf7, $vf7\n"
            "vaddx.z $vf7, $vf7, $vf7\n"
            "vsqrt $Q, $vf7x\n"
            "vaddq.x $vf7, $vf0, $Q\n"
            "qmfc2 %0, $vf7\n"
            : "=r"(dist)
            : "r"(triangles[index]), "r"(triangles[index + 1]), "r"(triangles[index + 2])
            : "memory"); 
       
        pair->dist = dist;
        pair->index = i;

    }
    //FlushCache(0);
    qsort(pairs, triangleCount, sizeof(ZSortPair), cmpfunc);

    free(pairs);
}

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

    for (int i = 0; i < buffer->meshData[MESHTRIANGLES]->vertexCount; i++)
    {
        int index = buffer->indices[i];
        VectorCopy(buffer->meshData[MESHTRIANGLES]->vertices[i], vertices[index]);
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

    for (int i = 0; i < buffer->meshData[MESHTRIANGLES]->vertexCount; i++)
    {
        int index = buffer->indices[i];
        VectorCopy(buffer->meshData[MESHTRIANGLES]->texCoords[i], uvs[index]);
    }

    free(uvs);
}

MeshBuffers *CreateGrid(int N, int M, float depth, float width, MeshBuffers *buffer)
{

    int faceCount = 2 * (N - 1) * (M - 1);

    int index_count = faceCount * 3;

    buffer->meshData[MESHTRIANGLES]->vertexCount = index_count;

    DEBUGLOG("Grid indices count %d", buffer->meshData[MESHTRIANGLES]->vertexCount);

    buffer->indices = (u32 *)malloc(sizeof(int) * index_count);
    buffer->meshData[MESHTRIANGLES]->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
    buffer->meshData[MESHTRIANGLES]->vertices = (VECTOR *)malloc(sizeof(VECTOR) * index_count);
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
    int len = strnlen(name, MAX_FILE_NAME);
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
        ERRORLOG("error appending strings. given input size too long ");
    }

    *outIter = 0;
}
