#include "ps_file_io.h"

#include "ps_gameobject.h"
#include "ps_texture.h"
#include "ps_misc.h"
#include "ps_log.h"

#include <stdlib.h>

sceCdRMode sStreamMode;
const u32 SectorSize = 2048;

#define SECTOR_SIZE 2048

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
    sceCdlFILE *loc_file_struct = FindFileByName(filename);

    if (loc_file_struct == NULL)
    {
        return NULL;
    }

    *outSize = loc_file_struct->size;

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

    return buffer;
}

MeshBuffers *AllocateMeshBuffersFromCode(MeshBuffers *buffers, u16 code, u32 size)
{
    buffers->vertexCount = size;
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
    float x, y, z, w;

    u32 *input_int = ptr;

    u32 size = SWAP_ENDIAN(*input_int);

    Bin2Float copy;
     

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

        buffers->texCoords[i][0] = x;

        input_int++;

        copy.int_x = SWAP_ENDIAN(*input_int);

        y = copy.float_x;

        buffers->texCoords[i][1] = y;

        input_int++;

      //  copy.int_x = SWAP_ENDIAN(*input_int);

      //  z = copy.float_x;

        buffers->texCoords[i][2] = 1.0f;

       // input_int++;
      //  copy.int_x = SWAP_ENDIAN(*input_int);

        buffers->texCoords[i][3] = 0.0f;

      //  input_int++;

        //  DumpVector(buffers->vertices[i]);
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

         // DumpVector(buffers->normals[i]);
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

static LoadFunc_Array loadFuncArray[8] = {NULL, LoadVertices, LoadIndices, LoadTexCoords,
                                          LoadNormals, NULL, NULL, LoadMaterial};

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
            // DEBUGLOG("%x", iter[0]);
            if (iter[0] == 0xAB && iter[1] == 0xAD && iter[2] == 0xBE && iter[3] == 0xEF)
            {
                //  DEBUGLOG("%x", iter[0]);
                iter += 4;
                u8 code = iter[0];
                iter += 1;
                u32 size = 0;
                input_int = (u32 *)iter;
                //DEBUGLOG("%x", code);
                iter += loadFuncArray[code-1](input_int, buffers, &index, &end);
            }
        }
    }
   // DEBUGLOG("finished?");

    free(buffer);
    // free(copy);
}
