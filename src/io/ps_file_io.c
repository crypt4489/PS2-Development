#include "io/ps_file_io.h"

#include <stdlib.h>
#include <string.h>


#include "log/ps_log.h"
#include "compression/ps_huffman.h"



sceCdRMode sStreamMode;


bool IsFileCompressed(const char *filename)
{
    int len = strnlen(filename, MAX_FILE_NAME);
    //  DEBUGLOG("Print isCompressed %c %c %c %c", filename[len-6], filename[len-5], filename[len-4], filename[len-3]);
    if (filename[len - 6] == 0x43 && filename[len - 5] == 0x42 && filename[len - 4] == 0x49 && filename[len - 3] == 0x4E)
    {
        return true;
    }

    return false;
}

void InitDVDDrive()
{
    sceCdInit(SCECdINIT);
}

u8 *ReadSector(u32 sector, u32 numOfSecs, u8 *buffer)
{
    while (true)
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

bool FileExist(const char *filename) {

    sceCdlFILE file_struct;
     
    if (!(sceCdSearchFile(&file_struct, filename)))
    {
        ERRORLOG("FINDEXISTS: Not found %s", filename);
        
        return false;
    }

    return true;
}

bool FindFileByName(const char *filename, sceCdlFILE *file_struct)
{
    if (!(sceCdSearchFile(file_struct, filename)))
    {
        ERRORLOG("FINDFILEBYNAME: Not found %s", filename);
        return false;
    }

    return true;
}

u8 readbuf[2048] __attribute__((aligned(2048)));

u32 ReadFileBytes(sceCdlFILE *loc_file_struct,
                  u8 *outBuffer,
                  u32 offset, u32 readSize)
{
    u32 starting_sec = loc_file_struct->lsn;

    const float divisor = .000488;

    u32 sectorOffset = offset * divisor;

    starting_sec += sectorOffset;

    u32 i = 0;

    u32 sectors = readSize * divisor;

    u32 remaining = readSize & (SECTOR_SIZE-1);

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
        if (bytesRead > bytesLeft) bytesRead = bytesLeft;
        ReadSector(starting_sec + i, 1, readbuf);
        memcpy(head_of_copy, readbuf, bytesRead);
        i++;
        head_of_copy += bytesRead;
        bytesLeft -= bytesRead;
        totalBytesRead += bytesRead;
    }

    return totalBytesRead;
}

u8 *ReadFileInFull(const char *filename, u32 *outSize)
{
    bool compressed = IsFileCompressed(filename);

    // DEBUGLOG("Compressed file ? %d", compressed);

    sceCdlFILE loc_file_struct;

    bool ret = FindFileByName(filename, &loc_file_struct);

    if (!ret) return NULL;
    
    u32 starting_sec = loc_file_struct.lsn;
    u32 sectors = loc_file_struct.size / SECTOR_SIZE;

    u32 remaining = loc_file_struct.size & (SECTOR_SIZE-1);

    u32 bufferSize = loc_file_struct.size;

    if (remaining)  sectors++;

    // allocate enough to read all sectors and avoid memory issues

    u8 *buffer = (u8 *)malloc(bufferSize/* + (SECTOR_SIZE-remaining)*/);

    u8 *head_of_copy = buffer;

    u32 i = 0;

    u32 bytesLeft = bufferSize;

    while (i < sectors)
    {
        u32 bytesRead = SECTOR_SIZE;
        if (bytesLeft < bytesRead) bytesRead = bytesLeft;
        ReadSector(starting_sec + i, 1, readbuf);
        i++;
        memcpy(head_of_copy, readbuf, bytesRead);
        head_of_copy += bytesRead;
        bytesLeft -= bytesRead;
    }

    if (compressed)
    {
        u8 *old = buffer;
        //float time1 = getTicks(g_Manager.timer);
        buffer = decompress(buffer, bufferSize, &bufferSize);
       // float time2 = getTicks(g_Manager.timer);
       // DEBUGLOG("DECOMPRESSION TIME %f", time2 - time1);
        free(old);
    }

    *outSize = bufferSize;

    return buffer;
}

