#include "ps_file_io.h"

#include <stdlib.h>
#include <string.h>

#include "ps_gameobject.h"
#include "ps_texture.h"
#include "ps_misc.h"
#include "ps_log.h"
#include "ps_huffman.h"
#include "ps_animation.h"
#include "ps_manager.h"

sceCdRMode sStreamMode;

#define SECTOR_SIZE 2048

static int IsFileCompressed(const char* filename)
{
    int len = strlen(filename);
  //  DEBUGLOG("Print isCompressed %c %c %c %c", filename[len-6], filename[len-5], filename[len-4], filename[len-3]);
    if (filename[len-6] == 0x43 && filename[len-5] == 0x42
    && filename[len-4] == 0x49 && filename[len-3] == 0x4E)
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
        ERRORLOG("Fuck");
    }

    return buffer;
}
sceCdlFILE *FindFileByName(const char *filename)
{
    sceCdlFILE *file_struct = (sceCdlFILE*)malloc(sizeof(sceCdlFILE)); //(sceCdlFILE *)malloc(sizeof(sceCdlFILE));
    if (!(sceCdSearchFile(file_struct, filename)))
    {
        ERRORLOG("FINDFILEBYNAME: Not found %s", filename);
        free(file_struct);
        return NULL;
    }
    // free(file_struct);

    return file_struct;
}

u8 *ReadFileInFull(const char *filename, u32 *outSize)
{
    int compressed = IsFileCompressed(filename);

    //DEBUGLOG("Compressed file ? %d", compressed);

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
        bufferSize += (SECTOR_SIZE - remaining); // will have a dead bytes, but it allows for the copy to go through
        sectors++;
    }

    u8 *buffer = (u8 *)malloc(bufferSize);

    u8 *head_of_copy = buffer;

    u32 i = 0;

    while (i < sectors)
    {
        ReadSector(starting_sec + i, 1, head_of_copy);
        i++;
        head_of_copy += SECTOR_SIZE;
    }

    if (compressed)
    {
        u8 *old = buffer;
        buffer = decompress(buffer, bufferSize, &bufferSize);
        free(old);
    }


    *outSize = bufferSize;

    return buffer;
}

MeshBuffers *AllocateMeshBuffersFromCode(MeshBuffers *buffers, u16 code, u32 size)
{
    buffers->vertexCount = size;
    buffers->meshAnimationData = NULL;
    if ((code & 0x01) != 0)
    {
        buffers->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x04) != 0)
    {
        buffers->normals = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x02) != 0)
    {
        buffers->texCoords = (VECTOR *)malloc(sizeof(VECTOR) * size);
    }

    if ((code & 0x08) != 0)
    {
        buffers->indices = (u32 *)malloc(sizeof(u32) * size);
    }

    if ((code & 0x10) != 0)
    {
        // buffers->indices = (u32 *)malloc(sizeof(u32) * size);
    }

    if ((code & 0x20) != 0)
    {
        //DEBUGLOG("AM I HERE WITH CREATING WEIGHTS and BONES");
        buffers->weights = (VECTOR *)malloc(sizeof(VECTOR) * size);
        buffers->bones = (VectorInt *)malloc(sizeof(VectorInt) * size);
        buffers->meshAnimationData = (AnimationMesh*)malloc(sizeof(AnimationMesh));
        buffers->meshAnimationData->animationsCount = buffers->meshAnimationData->jointsCount = 0;
        buffers->meshAnimationData->animations = NULL;
        buffers->meshAnimationData->joints = NULL;
    }

    buffers->materials = NULL;
    buffers->matCount = 0;

    return buffers;
}

typedef u32 (*LoadFunc_Array)(u32 *, MeshBuffers *, u32 *, u32 *);

// bytes read returned
static u32 LoadMaterial(u32 *ptr, MeshBuffers *buff, u32 *start, u32 *end)
{
    // DEBUGLOG("HERE!");
    int _start, _end, _ret;
    _start = *start = SWAP_ENDIAN(*ptr);
    ptr++;
    _end = SWAP_ENDIAN(*ptr);
    *end = _end + 1;
    ptr++;
   // DEBUGLOG("%d %d", _start, _end);
    char name[MAX_CHAR_TEXTURE_NAME];
    char *bytePtr = (char *)ptr;
    u8 sizeOfName = *bytePtr;
    bytePtr++;

    for (u8 i = 0; i < sizeOfName; i++)
    {
        name[i] = *bytePtr;
        bytePtr++;
    }

    name[sizeOfName] = 0;

    u32 id = GetTextureIDByName(name, g_Manager.texManager);

    buff = CreateMaterial(buff, _start, _end, id);

    _ret = 9 + sizeOfName;

    return _ret;
}

static u32 LoadVertices(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    float x, y, z;

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    Bin2Float copy;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        copy.int_x = SWAP_ENDIAN(*input_int);

        x = copy.float_x;

        buffers->vertices[i][0] = x;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        y = copy.float_x;

        buffers->vertices[i][1] = y;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        z = copy.float_x;

        buffers->vertices[i][2] = z;

        input_int++;
       // copy.int_x = SWAP_ENDIAN(*input_int);

       // w = copy.float_x;

        buffers->vertices[i][3] = 1.0f;

       // input_int++;

        //  DumpVector(buffers->vertices[i]);
    }

   // DEBUGLOG("%x", *input_int);

    return 4 + (12 * size);
}

static u32 LoadTexCoords(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    float x, y;

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    Bin2Float copy;
    // DEBUGLOG("siz ein func%d", size);

    // buffers->vertexCount = size;
    // buffers->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    input_int += 1;

    // DEBUGLOG("indy and end : %d %d", index, end);

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        copy.int_x = SWAP_ENDIAN(*input_int);

        x = copy.float_x;

        buffers->texCoords[i][0] = x;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        y = copy.float_x;

        buffers->texCoords[i][1] = y;

        input_int++;

        buffers->texCoords[i][2] = 1.0f;

        buffers->texCoords[i][3] = 0.0f;
    }

    return 4 + (8 * size);
}

static u32 LoadNormals(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    float x, y, z;

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    Bin2Float copy;
    // DEBUGLOG("siz ein func%d", size);

    // buffers->vertexCount = size;
    // buffers->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    input_int += 1;

    // DEBUGLOG("indy and end : %d %d", index, end);

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        copy.int_x = SWAP_ENDIAN(*input_int);

        x = copy.float_x;

        buffers->normals[i][0] = x;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        y = copy.float_x;

        buffers->normals[i][1] = y;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        z = copy.float_x;

        buffers->normals[i][2] = z;

        input_int++;
       // copy.int_x = SWAP_ENDIAN(*input_int);

        buffers->normals[i][3] = 0.0f;



       // input_int++;
    }

    return 4 + (12 * size);
}

static u32 LoadIndices(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);


    // buffers->vertexCount = size;
    // buffers->vertices = (VECTOR *)malloc(sizeof(VECTOR) * size);
    input_int += 1;

    // DEBUGLOG("indy and end : %d %d", index, end);

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        u32 inner = SWAP_ENDIAN(*input_int);
        buffers->indices[i] = inner;
        input_int++;
         // DEBUGLOG("%d : %d", buffers->indices[i], inner);
    }

    return ((size+1) * 4);
}

static u32 LoadWeights(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    float x, y, z, w;

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    Bin2Float copy;

    input_int += 1;

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        copy.int_x = SWAP_ENDIAN(*input_int);

        x = copy.float_x;

        buffers->weights[i][0] = x;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        y = copy.float_x;

        buffers->weights[i][1] = y;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        z = copy.float_x;

        buffers->weights[i][2] = z;

        input_int++;
        copy.int_x = SWAP_ENDIAN(*input_int);

        w = copy.float_x;

        buffers->weights[i][3] = w;

        input_int++;

       // VECTOR temp = *(buffers->weights[i]);
    }

   // DEBUGLOG("%x", *input_int);

    return 4 + (16 * size);
}

static u32 LoadBones(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    input_int += 1;

    // DEBUGLOG("indy and end : %d %d", index, end);

    u32 _start = *start;
    u32 _end = *end;

    for (int i = _start; i < _end; i++)
    {

        s32 innerx = (*input_int & 0x000000ff);
         if (innerx == 255)
            innerx = -1;
        buffers->bones[i][0] = innerx;


        s32 innery = ((*input_int >> 8) & 0x000000ff);
        if (innery == 255)
            innery = -1;
        buffers->bones[i][1] = innery;


        s32 innerz = ((*input_int >> 16) & 0x000000ff);
         if (innerz == 255)
            innerz = -1;
        buffers->bones[i][2] = innerz;


        s32 innerw = ((*input_int >> 24) & 0x000000ff);
        if (innerw == 255)
            innerw = -1;
        buffers->bones[i][3] = innerw;


        input_int++;
        //DumpVectorInt(buffers->bones[i]);
    }

    return ((size+1) * 4);
}

static u32* LoadMatrix(MATRIX m, u32 *input)
{
    Bin2Float copy;

    for (int i = 0; i<16; i++)
    {
        copy.int_x = SWAP_ENDIAN(*input);

        m[i] = copy.float_x;

        input++;
    }

    return input;
}

static u32 LoadJoints(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 size = SWAP_ENDIAN(*input_int);

    buffers->meshAnimationData->jointsCount = size;
    buffers->meshAnimationData->joints = malloc(sizeof(Joint*) * size);

    input_int++;

    for (int i = 0; i<size; i++)
    {
        u8 *bytePtr = (u8 *)input_int;
        Joint *joint = (Joint*)malloc(sizeof(Joint));
        joint->id = (*bytePtr++ & 0xff);
        //DEBUGLOG("%d %d", joint->id, size);
        u32 nameSize = (*bytePtr++ & 0xff);
        for (u8 iter = 0; iter < nameSize; iter++)
        {
            joint->name[iter] = *bytePtr;
            bytePtr++;
        }

        joint->name[nameSize] = 0;
       // DEBUGLOG("%s", joint->name);
        input_int = (u32*)bytePtr;
        input_int = LoadMatrix(joint->offset, input_int);
        //DumpMatrix(joint->offset);
        buffers->meshAnimationData->joints[i] = joint;
    }

    u32 ret_val = input_int - begin;

    return ret_val * 4;
}

static AnimationNode* ReadAnimationNode(u32 **input_ptr)
{
    AnimationNode *node = (AnimationNode*)malloc(sizeof(AnimationNode));

    u32 *input = *input_ptr;

    int nodeNameLength = SWAP_ENDIAN(*input);

    input++;

    u8 *bytePtr = (u8*)input;

    for (u8 iter = 0; iter < nodeNameLength; iter++)
    {
        node->name[iter] = *bytePtr;
        bytePtr++;
    }

    node->name[nodeNameLength] = 0;

    u32 count = node->childrenCount = *bytePtr;

    //DEBUGLOG("%s %d", node->name, count);

    bytePtr++;

    input = (u32*)bytePtr;

    input = LoadMatrix(node->transformation, input);

   // DumpMatrix(node->transformation);

    node->children = malloc(sizeof(AnimationNode*) * count);

    for (int child = 0; child<count; child++)
    {
       // DEBUGLOG("----------------");
      // node->children[child] = (AnimationNode*)malloc(sizeof(AnimationNode));
       node->children[child] = ReadAnimationNode(&input);
    }
    *input_ptr = input;
    return node;
}

static u32 LoadAnimationData(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{

    AnimationData *data = (AnimationData*)malloc(sizeof(AnimationData));

    u32 *input_int = ptr;

    u32 *begin = input_int;

    u32 nameSize =  SWAP_ENDIAN(*input_int);

    input_int++;

    u8 *bytePtr = (u8*)input_int;

    for (u8 iter = 0; iter < nameSize; iter++)
    {
        data->name[iter] = *bytePtr;
        bytePtr++;
    }

    data->name[nameSize] = 0;

   // DEBUGLOG("%s", data->name);

    input_int = (u32*)bytePtr;

    Bin2Float copy;

    copy.int_x = SWAP_ENDIAN(*input_int);

    data->duration = copy.float_x;

    input_int++;

    copy.int_x = SWAP_ENDIAN(*input_int);

    data->ticksPerSecond = copy.float_x;

    input_int++;

    //DEBUGLOG("%f %f", data->ticksPerSecond, data->duration);
    data->root = NULL;

    data->root = ReadAnimationNode( &input_int);

  //  DEBUGLOG("%s", data->root->name);


    LinkedList *newAnim = CreateLinkedListItem(data);

    buffers->meshAnimationData->animations = AddToLinkedList(buffers->meshAnimationData->animations, newAnim);

    buffers->meshAnimationData->animationsCount++;

    u32 ret_val = input_int - begin;

    return ret_val * 4;
}

static u32 LoadAnimationSRTs(u32 *ptr, MeshBuffers *buffers, u32 *start, u32 *end)
{
    //DEBUGLOG("WE ARE HERE IN SRT");

    u32 *input = ptr;

    u32 *ret = ptr;

    u32 posSize = SWAP_ENDIAN(*input);

    AnimationData *data = GetAnimationByIndex(buffers->meshAnimationData->animations,
                        buffers->meshAnimationData->animationsCount);

    //DEBUGLOG("%d %s", posSize, data->name);



    data->numPositionKeys = posSize;
    data->keyPositions = malloc(sizeof(AnimationKeyHolder*) * posSize);
    input++;
    Bin2Float copy;
    for (int i = 0; i<posSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder*)malloc(sizeof(AnimationKeyHolder));
        keyH->id = SWAP_ENDIAN(*input);
        input++;
        u32 size = SWAP_ENDIAN(*input);
        input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey*) * size);
        for (int j = 0; j<size; j++)
        {
            AnimationKey *key = (AnimationKey*)malloc(sizeof(AnimationKey));
            copy.int_x = SWAP_ENDIAN(*input);
            key->timeStamp = copy.float_x;
            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[0] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[1] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[2] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[3] = copy.float_x;
            input++;


            keyH->keys[j] = key;
        }
        data->keyPositions[i] = keyH;
    }


    u32 rotSize = SWAP_ENDIAN(*input);

    data->numRotationKeys = rotSize;
    data->keyRotations = malloc(sizeof(AnimationKeyHolder*) * rotSize);
    input++;


    for (int i = 0; i<rotSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder*)malloc(sizeof(AnimationKeyHolder));
        keyH->id = SWAP_ENDIAN(*input);
        input++;
        u32 size = SWAP_ENDIAN(*input);
        input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey*) * size);
        for (int j = 0; j<size; j++)
        {
            AnimationKey *key = (AnimationKey*)malloc(sizeof(AnimationKey));
            copy.int_x = SWAP_ENDIAN(*input);
            key->timeStamp = copy.float_x;
            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[1] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[2] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[3] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[0] = copy.float_x;
            input++;


            keyH->keys[j] = key;
        }
        data->keyRotations[i] = keyH;
    }


    u32 scalSize = SWAP_ENDIAN(*input);

    data->numScalingKeys = scalSize;
    data->keyScalings = malloc(sizeof(AnimationKeyHolder*) * scalSize);
    input++;


    for (int i = 0; i<rotSize; i++)
    {
        AnimationKeyHolder *keyH = (AnimationKeyHolder*)malloc(sizeof(AnimationKeyHolder));
        keyH->id = SWAP_ENDIAN(*input);
        input++;

        u32 size = SWAP_ENDIAN(*input);

        input++;

        keyH->count = size;
        keyH->keys = malloc(sizeof(AnimationKey*) * size);
        for (int j = 0; j<size; j++)
        {
            AnimationKey *key = (AnimationKey*)malloc(sizeof(AnimationKey));
            copy.int_x = SWAP_ENDIAN(*input);
            key->timeStamp = copy.float_x;
            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[0] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[1] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[2] = copy.float_x;

            input++;
            copy.int_x = SWAP_ENDIAN(*input);
            key->key[3] = copy.float_x;
            input++;



            keyH->keys[j] = key;
        }
        data->keyScalings[i] = keyH;
    }

    return (input - ret) * 4;
}


static LoadFunc_Array loadFuncArray[11] = {NULL, LoadVertices, LoadIndices, LoadTexCoords,
                                          LoadNormals, LoadBones, LoadWeights, LoadMaterial, LoadJoints, LoadAnimationData, LoadAnimationSRTs};

void ReadModelFile(const char *filename, MeshBuffers *buffers)
{
    char _file[MAX_FILE_NAME];
    Pathify(filename, _file);
    u32 fSize;
    u8 *buffer = ReadFileInFull(_file, &fSize);
    u8 *iter = buffer;

    u32 *input_int; //= (u32*)malloc(4);

    if (iter[0] == 0xDF && iter[1] == 0x01)
    {
        iter += 2;

        u16 code = (u16)(0xFF00 & (((u16)iter[0]) << 8)) | (0x00FF & (u16)iter[1]);

      //  DEBUGLOG("what's going on? %x", code);

        iter += 2;

        input_int = (u32 *)iter;
        u32 vertSize = SWAP_ENDIAN(*input_int);
        iter += 4;

      //  DEBUGLOG("%x", code);

        buffers = AllocateMeshBuffersFromCode(buffers, code, vertSize);
        u32 index = 0;
        u32 end = vertSize;

        while (iter[0] != 0xFF && iter[1] != 0xFF && iter[2] != 0x41 && iter[3] != 0x14)
        {
            if (iter[0] == 0xAB && iter[1] == 0xAD && iter[2] == 0xBE && iter[3] == 0xEF)
            {
                //  DEBUGLOG("%x", iter[0]);

               // while(1);
                iter += 4;
                u8 code = iter[0];
                iter += 1;
                input_int = (u32 *)iter;



                if (code <= 11)
                {
                    //DEBUGLOG("%x", code);
                    iter += loadFuncArray[code-1](input_int, buffers, &index, &end);
                }
                else
                {
                    ERRORLOG("Unsupported load mesh code");
                }
            }
        }
    }

    free(buffer);
    // free(copy);
}
