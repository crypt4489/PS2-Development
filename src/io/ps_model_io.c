#include "io/ps_model_io.h"
#include "io/ps_file_io.h"
#include "util/ps_misc.h"
#include "log/ps_log.h"

#include "gameobject/ps_gameobject.h"
#include "textures/ps_texture.h"
#include "util/ps_linkedlist.h"
#include "animation/ps_animation.h"
#include "math/ps_vector.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"
#include "textures/ps_texturemanager.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

MeshBuffers *AllocateMeshBuffersFromCode(MeshBuffers *buffers, u16 code, u32 size)
{
    buffers->meshData[MESHTRIANGLES]->vertexCount = size;
    buffers->meshAnimationData = NULL;
    if (code & 0x01)
    {
        buffers->meshData[MESHTRIANGLES]->vertices = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
    }

    if (code & 0x04)
    {
        buffers->meshData[MESHTRIANGLES]->normals = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
    }

    if (code & 0x02)
    {
        buffers->meshData[MESHTRIANGLES]->texCoords = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
    }

    if (code & 0x08)
    {
        buffers->indices = (u32 *)malloc(sizeof(u32) * size);
    }

    if ((code & 0x20))
    {
        buffers->meshData[MESHTRIANGLES]->weights = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
        buffers->meshData[MESHTRIANGLES]->bones = (VectorInt *)memalign(16,sizeof(VectorInt) * size);
        buffers->meshAnimationData = (AnimationMesh *)malloc(sizeof(AnimationMesh));
        buffers->meshAnimationData->animationsCount = buffers->meshAnimationData->jointsCount = 0;
        buffers->meshAnimationData->animations = NULL;
        buffers->meshAnimationData->joints = NULL;
    }

    buffers->materials = NULL;
    buffers->matCount = 0;

    return buffers;
}

typedef u32 (*LoadFunc_Array)(u32 *, MeshBuffers *, u32 *, u32 *);

static u32 LoadMaterial(u32 *ptr, MeshBuffers *buff, u32 *start, u32 *end)
{
    // DEBUGLOG("HERE!");
    int _start, _end, _ret;
    _start = *start = *ptr;
    ptr++;
    _end = *ptr;
    *end = _end + 1;
    ptr++;
    // DEBUGLOG("%d %d", _start, _end);
    char name[MAX_CHAR_TEXTURE_NAME];
    char *bytePtr = (char *)ptr;
    u8 sizeOfName = *bytePtr++;

    for (u8 i = 0; i < sizeOfName; i++)
    {
        name[i] = *bytePtr++;
    }

    name[sizeOfName] = 0;

    u32 id = GetTextureIDByName(g_Manager.texManager, name);

    buff = CreateMaterial(buff, _start, _end, id);

    _ret = 9 + sizeOfName;

    return _ret;
}

static u32 LoadVertices(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {
        memcpy(buffers->meshData[MESHTRIANGLES]->vertices[i], input_int, 12);
        buffers->meshData[MESHTRIANGLES]->vertices[i][3] = 1.0f;
        input_int+=3;
    }

    return 4 + (12 * size);
}

static u32 LoadTexCoords(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {
         memcpy(buffers->meshData[MESHTRIANGLES]->texCoords[i], input_int, 8);

        buffers->meshData[MESHTRIANGLES]->texCoords[i][2] = 1.0f;

        buffers->meshData[MESHTRIANGLES]->texCoords[i][3] = 0.0f;

        input_int += 2;
    }

    return 4 + (8 * size);
}

static u32 LoadNormals(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        memcpy(buffers->meshData[MESHTRIANGLES]->normals[i], input_int, 12);

        buffers->meshData[MESHTRIANGLES]->normals[i][3] = 0.0f;

        input_int+=3;
    }

    return 4 + (12 * size);
}

static u32 LoadIndices(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {
        buffers->indices[i] = *input_int++;
    }

    return ((size + 1) * 4);
}

static u32 LoadWeights(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {
        memcpy(buffers->meshData[MESHTRIANGLES]->weights[i], input_int, 16);

        input_int+=4;
    }

    return 4 + (16 * size);
}

static u32 LoadBones(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = *input_int;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        s32 innerx = (*input_int & 0x000000ff);
        if (innerx == 255)
            innerx = -1;
        buffers->meshData[MESHTRIANGLES]->bones[i][0] = innerx;

        s32 innery = ((*input_int >> 8) & 0x000000ff);
        if (innery == 255)
            innery = -1;
        buffers->meshData[MESHTRIANGLES]->bones[i][1] = innery;

        s32 innerz = ((*input_int >> 16) & 0x000000ff);
        if (innerz == 255)
            innerz = -1;
        buffers->meshData[MESHTRIANGLES]->bones[i][2] = innerz;

        s32 innerw = ((*input_int >> 24) & 0x000000ff);
        if (innerw == 255)
            innerw = -1;
        buffers->meshData[MESHTRIANGLES]->bones[i][3] = innerw;

        input_int++;
    }

    return ((size + 1) * 4);
}

static u32 *LoadMatrix(MATRIX m, u32 *input)
{
    memcpy(m, input, 4*16);

    input += 16;

    return input;
}

static u32 LoadJoints(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 size = *input_int;

    buffers->meshAnimationData->jointsCount = size;
    buffers->meshAnimationData->joints = malloc(sizeof(Joint *) * size);
    buffers->meshAnimationData->finalBones = (VECTOR*)memalign(16,sizeof(VECTOR) * 3 * size);
    input_int++;

    for (int i = 0; i < size; i++)
    {
        Joint *joint = (Joint *)malloc(sizeof(Joint));
        joint->id = *input_int++;
        // DEBUGLOG("%d %d", joint->id, size);
        u32 nameSize = *input_int++;
        memcpy(joint->name, input_int, nameSize);
        joint->name[nameSize] = 0;
        int skip = nameSize >> 2;
        if (nameSize % 4) skip++;
        input_int+=skip; 
        // DEBUGLOG("%s %d %d", joint->name, skip, nameSize);
        input_int = LoadMatrix(joint->offset, input_int);
        // DumpMatrix(joint->offset);
        buffers->meshAnimationData->joints[i] = joint;
    }

    return (input_int - begin) * 4;
} 


static u32* ReadAnimationNodes(AnimationNode *node, u32* input_ptr)
{
    u32 nodeNameLength  = *input_ptr++;
    
    memcpy(node->name, input_ptr, nodeNameLength);

    node->name[nodeNameLength] = 0;

    int skip = nodeNameLength  >> 2;

    if (nodeNameLength  % 4) skip++;

    input_ptr += skip;

    u32 count = node->childrenCount = *input_ptr++;

    input_ptr = LoadMatrix(node->transformation, input_ptr);

    node->children = (u32*)malloc(sizeof(u32) * count);

    return input_ptr;
}

static void AssignChildren(AnimationNode *node, u32 nodeCount)
{
    u32 *nodes = (u32*)malloc(sizeof(u32) * nodeCount);

    u32 *child = (u32*)malloc(sizeof(u32) * nodeCount);

    int stackptr = 0;

    child[stackptr] = 0;

    nodes[stackptr] = 0;

    u32 i = 1;

    while (true)
    {
        u32 ni = nodes[stackptr];
        u32 ci = child[stackptr];
        AnimationNode *eval = &node[ni];
        while(!eval->childrenCount || ci == eval->childrenCount)
        {
            child[stackptr] = 0;
            stackptr--;
            if (stackptr < 0) goto end;
            ni = nodes[stackptr];
            ci = child[stackptr];
            eval = &node[ni];
        }
        eval->children[ci] = i;
        child[stackptr]++;
        nodes[++stackptr] = i;
        child[stackptr] = 0;
        i++;
    }
end:
    free(nodes);
    free(child);
}

static u32 LoadAnimationData(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    AnimationData *data = (AnimationData *)malloc(sizeof(AnimationData));

    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 nameSize = *input_int++;

    memcpy(data->name, input_int, nameSize);

    data->name[nameSize] = 0;

    int skip = nameSize >> 2;

    if (nameSize % 4) skip++;

    input_int += skip;

    memcpy(&data->duration, input_int, 4);

    input_int++;

    memcpy(&data->ticksPerSecond, input_int, 4);

    input_int++;

    LinkedList *newAnim = CreateLinkedListItem(data);

    buffers->meshAnimationData->animations = AddToLinkedList(buffers->meshAnimationData->animations, newAnim);

    buffers->meshAnimationData->animationsCount++;

    return (input_int - begin) * 4;
}

static u32 LoadAnimationSRTs(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    u32 *input = ptr;

    u32 *ret = ptr;

    u32 posSize = *input++;

    AnimationData *data = GetAnimationByIndex(buffers->meshAnimationData->animations,
                                              buffers->meshAnimationData->animationsCount);

    data->numPositionKeys = posSize;
    data->keyPositions = malloc(sizeof(AnimationKeyHolder *) * posSize);
    
    for (int i = 0; i < posSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder *)malloc(sizeof(AnimationKeyHolder));
        keyH->id = *input++;
        u32 size = *input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey *) * size);
        for (int j = 0; j < size; j++)
        {
            AnimationKey *key = (AnimationKey *)malloc(sizeof(AnimationKey));

            memcpy(&key->timeStamp, input, 4);
            input++;
            memcpy(key->key, input, 16);
            input+=4;

            keyH->keys[j] = key;
        }
        data->keyPositions[i] = keyH;
    }

    u32 rotSize = *input++;

    data->numRotationKeys = rotSize;
    data->keyRotations = malloc(sizeof(AnimationKeyHolder *) * rotSize);

    for (int i = 0; i < rotSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder *)malloc(sizeof(AnimationKeyHolder));
        keyH->id = *input++;

        u32 size = *input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey *) * size);
        for (int j = 0; j < size; j++)
        {
            AnimationKey *key = (AnimationKey *)malloc(sizeof(AnimationKey));

            memcpy(&key->timeStamp, input, 4);
            input++;

            memcpy(&key->key[1], input, 12);
            

            memcpy(&key->key[0], input+3, 4);
            input+=4;

            keyH->keys[j] = key;
        }
        data->keyRotations[i] = keyH;
    }

    u32 scalSize = *input++;

    data->numScalingKeys = scalSize;
    data->keyScalings = malloc(sizeof(AnimationKeyHolder *) * scalSize);

    for (int i = 0; i < rotSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder *)malloc(sizeof(AnimationKeyHolder));
        keyH->id = *input++;

        u32 size = *input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey *) * size);
        for (int j = 0; j < size; j++)
        {
            AnimationKey *key = (AnimationKey *)malloc(sizeof(AnimationKey));
            memcpy(&key->timeStamp, input, 4);
            input++;
            memcpy(key->key, input, 16);
            input+=4;

            keyH->keys[j] = key;
        }
        data->keyScalings[i] = keyH;
    }

    return (input - ret) * 4;
}

static u32 LoadAnimationNodes(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 nodeCount = buffers->meshAnimationData->nodeCount = *input_int++;

    buffers->meshAnimationData->root = (AnimationNode *)malloc(nodeCount* sizeof(AnimationNode));
   
    for(u32 i = 0; i<nodeCount; i++)
    {
        input_int = ReadAnimationNodes(&(buffers->meshAnimationData->root[i]), input_int);
    }

    AssignChildren(buffers->meshAnimationData->root, nodeCount);

    return (input_int - begin) * 4;
}

static LoadFunc_Array loadFuncArray[12] = {NULL, LoadVertices, LoadIndices, LoadTexCoords,
                                           LoadNormals, LoadBones, LoadWeights, LoadMaterial, LoadJoints, LoadAnimationData, LoadAnimationSRTs, LoadAnimationNodes};

static void CreateVerticesBuffer(MeshBuffers *buffers, u16 code, u32 vertSize, u32 indicesSize)
{
    float divisor = 1.f / 8.f;
    u32 sizeOfBuffer = vertSize * divisor;
    if ((vertSize & 0x7))
    {
        sizeOfBuffer++;
    }

    u8 *binaryBuffer = (u8 *)malloc(sizeOfBuffer);
    if (!binaryBuffer)
    {
        ERRORLOG("ERROR creating binaryBuffer");
        return;
    }

    memset(binaryBuffer, 0, sizeOfBuffer);

    u32 *indices = buffers->indices;

    for (int i = 0; i < indicesSize; i++)
    {
        u32 index = indices[i];

        u32 indexInBuffer = index * divisor;
        u8 mask =  1 << (index & 0x07);
        u8 present = (binaryBuffer[indexInBuffer] & mask);
        if (present)
        {
            continue;
        }
        else
        {
            binaryBuffer[indexInBuffer] |= mask;
        }

        if (code & 0x01)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->vertices[index], buffers->meshData[MESHTRIANGLES]->vertices[i]);
        }

        if (code & 0x04)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->normals[index], buffers->meshData[MESHTRIANGLES]->normals[i]);
        }

        if (code & 0x02)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->texCoords[index], buffers->meshData[MESHTRIANGLES]->texCoords[i]);
        }

        if (code & 0x20)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->weights[index], buffers->meshData[MESHTRIANGLES]->weights[i]);
            VectorIntCopy(buffers->meshData[MESHVERTICES]->bones[index], buffers->meshData[MESHTRIANGLES]->bones[i]);
        }
    }

    free(binaryBuffer);

}

static void AllocateVerticesBufferFromCode(MeshBuffers *buffers, u16 code, u32 size)
{
    buffers->meshData[MESHVERTICES]->vertexCount = size;
    if (code & 0x01)
    {
        buffers->meshData[MESHVERTICES]->vertices = (VECTOR *)memalign(16, sizeof(VECTOR) * size);
    }

    if (code & 0x04)
    {
        buffers->meshData[MESHVERTICES]->normals = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
    }

    if (code & 0x02)
    {
        buffers->meshData[MESHVERTICES]->texCoords = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
    }

    if (code & 0x20)
    {

        buffers->meshData[MESHVERTICES]->weights = (VECTOR *)memalign(16,sizeof(VECTOR) * size);
        buffers->meshData[MESHVERTICES]->bones = (VectorInt *)memalign(16,sizeof(VectorInt) * size);
    }
}

void CreateMeshBuffersFromFile(void *object, void *params, u8 *buffer, u32 bufferLen)
{
    MeshBuffers *buffers = (MeshBuffers *)object;

    u8 *iter = buffer;

    u32 *input_int; //= (u32*)malloc(4);

    // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);

    if (iter[1] == 0xDF && iter[0] == 0x01)
    {
        iter += 4;

        u16 meshCode = iter[0] | ((iter[1] << 8) & 0xFF00);

       //DEBUGLOG("what's going on? %x", meshCode);
        
        iter += 4;
        // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);
        input_int = (u32 *)iter;
        u32 indicesSize = *input_int;
        iter += 4;
        // DEBUGLOG("%d", indicesSize);
        // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);
        input_int = (u32 *)iter;
        u32 verticesSize = *input_int;
        iter += 4;
        // DEBUGLOG("%d", verticesSize);

        buffers = AllocateMeshBuffersFromCode(buffers, meshCode, indicesSize);
        u32 index = 0;
        u32 end = indicesSize;
    
        while (iter[0] != 0xFF && iter[1] != 0xFF && iter[2] != 0x41 && iter[3] != 0x14)
        {
            if (iter[0] == 0xAB && iter[1] == 0xAD && iter[2] == 0xBE && iter[3] == 0xEF)
            {
                //  DEBUGLOG("%x", iter[0]);

                // while(1);
                iter += 4;
                input_int = (u32 *)iter;
                u32 code = *input_int;
                input_int++;
                iter += 4;

                if (code <= 12)
                {
                    DEBUGLOG("%x", code);
                    iter += loadFuncArray[code - 1](input_int, buffers, &index, &end);
                }
                else
                {
                    ERRORLOG("Unsupported load mesh code %x", code);
                    break;
                }
            } else {
                break;
            }
            // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);
            // break;
        }
        AllocateVerticesBufferFromCode(buffers, meshCode, verticesSize);
        CreateVerticesBuffer(buffers, meshCode, verticesSize, indicesSize);
    }
}

void ReadModelFile(const char *filename, MeshBuffers *buffers)
{
    char _file[MAX_FILE_NAME];
    Pathify(filename, _file);
    u32 fSize;
    u8 *buffer = ReadFileInFull(_file, &fSize);
    if (!buffer)
    {
        ERRORLOG("File Buffer for mesh %s was empty", filename);
        return;
    }

    CreateMeshBuffersFromFile(buffers, NULL, buffer, fSize);

    free(buffer);
}

static void DeleteAnimationNode(AnimationNode *root, u32 nodeCount)
{
    for (u32 i = 0; i<nodeCount; i++)
    {
        free(root[i].children);
    }
    free(root);
}

static void DeleteAnimationData(AnimationData *data)
{
    if (data)
    {
        int i;
        for (i = 0; i<data->numPositionKeys; i++)
        {
            AnimationKeyHolder *key = data->keyPositions[i];
            for (int j = 0; j<key->count; j++)
            {
                free(key->keys[j]);
            }
            free(key->keys);
            free(key);
        }

        free(data->keyPositions);

        for (i = 0; i<data->numRotationKeys; i++)
        {
            AnimationKeyHolder *key = data->keyRotations[i];
            for (int j = 0; j<key->count; j++)
            {
                free(key->keys[j]);
            }
            free(key->keys);
            free(key);
        }

        free(data->keyRotations);

        for (i = 0; i<data->numRotationKeys; i++)
        {
            AnimationKeyHolder *key = data->keyScalings[i];
            for (int j = 0; j<key->count; j++)
            {
                free(key->keys[j]);
            }
            free(key->keys);
            free(key);
        }

        free(data->keyScalings);

       

        free(data);
    }
}

void DestroyAnimationMesh(AnimationMesh *meshAnimationData)
{
    if (meshAnimationData)
    {
        for (int i = 0; i<meshAnimationData->jointsCount; i++)
        {
            free(meshAnimationData->joints[i]);
        }

        DeleteAnimationNode(meshAnimationData->root, meshAnimationData->nodeCount);

        free(meshAnimationData->joints);

        free(meshAnimationData->finalBones);

        if (meshAnimationData->animations)
        {
            LinkedList *list = meshAnimationData->animations;
            while(list)
            {
                DeleteAnimationData(list->data);
                free(list->data);
                list = CleanLinkedListNode(list);
            }
        }
        
        free(meshAnimationData);
    }
}