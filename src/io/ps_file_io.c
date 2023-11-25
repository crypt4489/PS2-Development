#include "io/ps_file_io.h"

#include <stdlib.h>
#include <string.h>

#include "gameobject/ps_gameobject.h"
#include "textures/ps_texture.h"
#include "math/ps_misc.h"
#include "log/ps_log.h"
#include "compression/ps_huffman.h"
#include "animation/ps_animation.h"
#include "gamemanager/ps_manager.h"
#include "util/ps_linkedlist.h"
#include "system/ps_timer.h"
#include "math/ps_vector.h"
#include "math/ps_matrix.h"

sceCdRMode sStreamMode;

extern GameManager g_Manager;

int IsFileCompressed(const char *filename)
{
    int len = strnlen(filename, MAX_FILE_NAME);
    //  DEBUGLOG("Print isCompressed %c %c %c %c", filename[len-6], filename[len-5], filename[len-4], filename[len-3]);
    if (filename[len - 6] == 0x43 && filename[len - 5] == 0x42 && filename[len - 4] == 0x49 && filename[len - 3] == 0x4E)
    {
        return 1;
    }

    return 0;
}

void InitDVDDrive()
{
    sceCdInit(SCECdINIT);
}

u8 *ReadSector(u32 sector, u32 numOfSecs, u8 *buffer)
{
    while (1)
    {
        if (sceCdRead(sector, numOfSecs, buffer, &sStreamMode) == 0)
        {
            ERRORLOG("read failed");
            break;
        }

        sceCdSync(0);

        if (sceCdGetError() == 0)
        {
            break;
        }
        ERRORLOG("Error: cannot read file");
    }

    return buffer;
}

int FileExist(const char *filename) {

    sceCdlFILE file_struct; 
     if (!(sceCdSearchFile(&file_struct, filename)))
    {
        ERRORLOG("FINDEXISTS: Not found %s", filename);
        
        return 0;
    }

    return 1;
}

sceCdlFILE *FindFileByName(const char *filename)
{
    sceCdlFILE *file_struct = (sceCdlFILE *)malloc(sizeof(sceCdlFILE)); //(sceCdlFILE *)malloc(sizeof(sceCdlFILE));

    if (!(sceCdSearchFile(file_struct, filename)))
    {
        ERRORLOG("FINDFILEBYNAME: Not found %s", filename);
        free(file_struct);
        return NULL;
    }

    return file_struct;
}

u32 ReadFileBytes(sceCdlFILE *loc_file_struct,
                  u8 *outBuffer,
                  u32 offset, u32 readSize)
{
    u32 starting_sec = loc_file_struct->lsn;

    float divisor = 1.f / SECTOR_SIZE;

    u32 sectorOffset = offset * divisor;

    starting_sec += sectorOffset;

    u32 i = 0;

    u32 sectors = readSize * divisor;

    u32 remaining = readSize % SECTOR_SIZE;

    if (remaining)
    {
        sectors++;
    }

    u32 bytesLeft = readSize;

    u8 *head_of_copy = outBuffer;
    u32 totalBytesRead = 0;
    while (i < sectors)
    {
        u32 bytesRead = SECTOR_SIZE;
        if (SECTOR_SIZE > bytesLeft)
        {
            u8 readBuffer[SECTOR_SIZE];
            bytesRead = bytesLeft;
            ReadSector(starting_sec + i, 1, readBuffer);
            memcpy(head_of_copy, readBuffer, bytesRead);
        }
        else
        {
            ReadSector(starting_sec + i, 1, head_of_copy);
        }
        i += 1;
        head_of_copy += bytesRead;
        bytesLeft -= bytesRead;
        totalBytesRead += bytesRead;
    }

    return totalBytesRead;
}

u8 *ReadFileInFull(const char *filename, u32 *outSize)
{
    int compressed = IsFileCompressed(filename);

    // DEBUGLOG("Compressed file ? %d", compressed);

    sceCdlFILE *loc_file_struct = FindFileByName(filename);

    if (loc_file_struct == NULL)
    {
        return NULL;
    }

    u32 starting_sec = loc_file_struct->lsn;
    u32 sectors = loc_file_struct->size / SECTOR_SIZE;

    u32 remaining = loc_file_struct->size % SECTOR_SIZE;

    u32 bufferSize = loc_file_struct->size;

    if (remaining)
    {
        sectors++;
    }

    u32 bytesLeft = bufferSize;

    u8 *buffer = (u8 *)malloc(bufferSize);

    u8 *head_of_copy = buffer;

    memset(head_of_copy, 0, bufferSize);

    u32 i = 0;

    while (i < sectors)
    {

        u32 bytesRead = SECTOR_SIZE;
        if (SECTOR_SIZE > bytesLeft)
        {
            u8 readBuffer[SECTOR_SIZE];
            bytesRead = bytesLeft;
            ReadSector(starting_sec + i, 1, readBuffer);
            memcpy(head_of_copy, readBuffer, bytesRead);
        }
        else
        {
            ReadSector(starting_sec + i, 1, head_of_copy);
        }

        i++;
        head_of_copy += bytesRead;
        bytesLeft -= bytesRead;
    }

    if (compressed)
    {
        u8 *old = buffer;
        float time1 = getTicks(g_Manager.timer);
        buffer = decompress(buffer, bufferSize, &bufferSize);
        float time2 = getTicks(g_Manager.timer);
        DEBUGLOG("DECOMPRESSION TIME %f", time2 - time1);
        free(old);
    }

    *outSize = bufferSize;

    free(loc_file_struct);

    return buffer;
}

MeshBuffers *AllocateMeshBuffersFromCode(MeshBuffers *buffers, u16 code, u32 size)
{
    buffers->meshData[MESHTRIANGLES]->vertexCount = size;
    buffers->meshAnimationData = NULL;
    if ((code & 0x01) != 0)
    {
        buffers->meshData[MESHTRIANGLES]->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x04) != 0)
    {
        buffers->meshData[MESHTRIANGLES]->normals = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x02) != 0)
    {
        buffers->meshData[MESHTRIANGLES]->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x08) != 0)
    {
        buffers->indices = (u32 *)malloc(sizeof(u32) * size);
    }

    if ((code & 0x20) != 0)
    {
        buffers->meshData[MESHTRIANGLES]->weights = (VECTOR *)malloc(sizeof(VECTOR) * size);
        buffers->meshData[MESHTRIANGLES]->bones = (VectorInt *)malloc(sizeof(VectorInt) * size);
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

    u32 id = GetTextureIDByName(name, g_Manager.texManager);

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
    Bin2Float copy;

    for (int i = 0; i < 16; i++)
    {
        copy.int_x = *input++;

        m[i] = copy.float_x;
    }

    return input;
}

static u32 LoadJoints(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 size = *input_int;

    buffers->meshAnimationData->jointsCount = size;
    buffers->meshAnimationData->joints = malloc(sizeof(Joint *) * size);

    input_int++;

    for (int i = 0; i < size; i++)
    {
        u8 *bytePtr = (u8 *)input_int;
        Joint *joint = (Joint *)malloc(sizeof(Joint));
        joint->id = (*bytePtr++ & 0xff);
        // DEBUGLOG("%d %d", joint->id, size);
        u32 nameSize = (*bytePtr++ & 0xff);
        for (u8 iter = 0; iter < nameSize; iter++)
        {
            joint->name[iter] = *bytePtr++;
        }

        joint->name[nameSize] = 0;
        // DEBUGLOG("%s", joint->name);
        input_int = (u32 *)bytePtr;
        input_int = LoadMatrix(joint->offset, input_int);
        // DumpMatrix(joint->offset);
        buffers->meshAnimationData->joints[i] = joint;
    }

    return (input_int - begin) * 4;
}


static AnimationNode *ReadAnimationNode(u32 **input_ptr)
{
    AnimationNode *node = (AnimationNode *)malloc(sizeof(AnimationNode));

    u32 *input = *input_ptr;

    int nodeNameLength = *input++;;

    u8 *bytePtr = (u8 *)input;

    for (u8 iter = 0; iter < nodeNameLength; iter++)
    {
        node->name[iter] = *bytePtr++;
    }

    node->name[nodeNameLength] = 0;

    u32 count = node->childrenCount = *bytePtr++;

    input = (u32 *)bytePtr;

    input = LoadMatrix(node->transformation, input);

    node->children = malloc(sizeof(AnimationNode *) * count);

    for (int child = 0; child < count; child++)
    {
        node->children[child] = ReadAnimationNode(&input);
    }

    *input_ptr = input;
    return node;
}

static u32 LoadAnimationData(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    AnimationData *data = (AnimationData *)malloc(sizeof(AnimationData));

    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 nameSize = *input_int++;

    u8 *bytePtr = (u8 *)input_int;

    for (u8 iter = 0; iter < nameSize; iter++)
    {
        data->name[iter] = *bytePtr++;
    }

    data->name[nameSize] = 0;

    input_int = (u32 *)bytePtr;

    Bin2Float copy;

    copy.int_x = *input_int++;

    data->duration = copy.float_x;

    copy.int_x = *input_int++;

    data->ticksPerSecond = copy.float_x;

    data->root = NULL;

    data->root = ReadAnimationNode(&input_int);

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

    Bin2Float copy;
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
            copy.int_x = *input++;
            key->timeStamp = copy.float_x;

            copy.int_x = *input++;
            key->key[0] = copy.float_x;

            copy.int_x = *input++;
            key->key[1] = copy.float_x;

            copy.int_x = *input++;
            key->key[2] = copy.float_x;

            copy.int_x = *input++;
            key->key[3] = copy.float_x;

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

            copy.int_x = *input++;
            key->timeStamp = copy.float_x;

            copy.int_x = *input++;
            key->key[1] = copy.float_x;

            copy.int_x = *input++;
            key->key[2] = copy.float_x;

            copy.int_x = *input++;
            key->key[3] = copy.float_x;

            copy.int_x = *input++;
            key->key[0] = copy.float_x;

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
            copy.int_x = *input++;
            key->timeStamp = copy.float_x;

            copy.int_x = *input++;
            key->key[0] = copy.float_x;

            copy.int_x = *input++;
            key->key[1] = copy.float_x;

            copy.int_x = *input++;
            key->key[2] = copy.float_x;

            copy.int_x = *input++;
            key->key[3] = copy.float_x;

            keyH->keys[j] = key;
        }
        data->keyScalings[i] = keyH;
    }

    return (input - ret) * 4;
}

static LoadFunc_Array loadFuncArray[11] = {NULL, LoadVertices, LoadIndices, LoadTexCoords,
                                           LoadNormals, LoadBones, LoadWeights, LoadMaterial, LoadJoints, LoadAnimationData, LoadAnimationSRTs};

static void CreateVerticesBuffer(MeshBuffers *buffers, u16 code, u32 vertSize, u32 indicesSize)
{
    float divisor = 1.f / 8.f;
    u32 sizeOfBuffer = vertSize * divisor;
    if ((vertSize & 0x7) != 0)
    {
        sizeOfBuffer++;
    }

    u8 *binaryBuffer = (u8 *)malloc(sizeOfBuffer);
    if (binaryBuffer == NULL)
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
        u8 mask = index & 0x07;
        u8 present = (binaryBuffer[indexInBuffer] & mask);
        if (present)
        {
            continue;
        }
        else
        {
            binaryBuffer[indexInBuffer] |= mask;
        }

        if ((code & 0x01) != 0)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->vertices[index], buffers->meshData[MESHTRIANGLES]->vertices[i]);
        }

        if ((code & 0x04) != 0)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->normals[index], buffers->meshData[MESHTRIANGLES]->normals[i]);
        }

        if ((code & 0x02) != 0)
        {
            VectorCopy(buffers->meshData[MESHVERTICES]->texCoords[index], buffers->meshData[MESHTRIANGLES]->texCoords[i]);
        }

        if ((code & 0x20) != 0)
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
    if ((code & 0x01) != 0)
    {
        buffers->meshData[MESHVERTICES]->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x04) != 0)
    {
        buffers->meshData[MESHVERTICES]->normals = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x02) != 0)
    {
        buffers->meshData[MESHVERTICES]->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x20) != 0)
    {

        buffers->meshData[MESHVERTICES]->weights = (VECTOR *)malloc(sizeof(VECTOR) * size);
        buffers->meshData[MESHVERTICES]->bones = (VectorInt *)malloc(sizeof(VectorInt) * size);
    }
}

void CreateMeshBuffersFromFile(void *object, void *params, u8 *buffer, u32 bufferLen)
{
    MeshBuffers *buffers = (MeshBuffers *)object;

    u8 *iter = buffer;

    u32 *input_int; //= (u32*)malloc(4);

    // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);

    if (iter[0] == 0xDF && iter[1] == 0x01)
    {
        iter += 2;

        u16 meshCode = (u16)(0xFF00 & (((u16)iter[0]) << 8)) | (0x00FF & (u16)iter[1]);

       //DEBUGLOG("what's going on? %x", meshCode);

        iter += 2;
        // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);
        input_int = (u32 *)iter;
        u32 indicesSize = *input_int;
        iter += 4;
        // DEBUGLOG("%d", indicesSize);
        // DEBUGLOG("%x %x %x %x", iter[0], iter[1], iter[2], iter[3]);
        input_int = (u32 *)iter;
        u32 verticesSize =*input_int;
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
                u8 code = iter[0];
                //  DEBUGLOG("%x", code);
                iter += 1;
                input_int = (u32 *)iter;

                if (code <= 11)
                {
                    // DEBUGLOG("%x", code);
                    iter += loadFuncArray[code - 1](input_int, buffers, &index, &end);
                }
                else
                {
                    ERRORLOG("Unsupported load mesh code %x", code);
                }
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
    if (buffer == NULL)
    {
        ERRORLOG("File Buffer for mesh %s was empty", filename);
        return;
    }
    //  float time1  = getTicks(g_Manager.timer);

    CreateMeshBuffersFromFile(buffers, NULL, buffer, fSize);

    // float time2 = getTicks(g_Manager.timer);

    // DEBUGLOG("Time for Create : %f", time2-time1);
    free(buffer);
    // free(copy);
}
