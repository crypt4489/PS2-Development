#ifndef PS_FILE_IO_H
#define PS_FILE_IO_H
#include <libcdvd.h>
#include "ps_global.h"



sceCdlFILE *FindFileByName(const char *filename);
u8 *ReadFileInFull(const char *filename, u32 *outSize);
void InitDVDDrive();

u8* ReadSector(u32 sector, u32 numOfSecs, u8* buffer);

void ReadModelFile(const char *filename, MeshBuffers *buffers);

#endif