#ifndef PS_FILE_IO_H
#define PS_FILE_IO_H
#include <libcdvd.h>
#include "ps_global.h"

#define SECTOR_SIZE 2048

bool FindFileByName(const char *filename, sceCdlFILE *loc_file_struct);

u32 ReadFileBytes(sceCdlFILE *loc_file_struct,
                    u8 *outBuffer,
                    u32 offset, u32 readSize);

u8 *ReadFileInFull(const char *filename, u32 *outSize);

void InitDVDDrive();

bool IsFileCompressed(const char *filename);

u8* ReadSector(u32 sector, u32 numOfSecs, u8* buffer);

bool FileExist(const char *filename);

#endif
