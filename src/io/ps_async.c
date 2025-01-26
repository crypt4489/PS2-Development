#include "io/ps_async.h"

#include <kernel.h>
#include <stdlib.h>

#include "log/ps_log.h"
#include "util/ps_queue.h"
#include "util/ps_misc.h"
#include "system/ps_timer.h"
typedef struct io_file_info_t
{
    u32 totalSize;
    u32 totalRead;
    u32 currentSector;
    u32 totalSectors;
} FileInfo;
typedef struct io_queue_item_t
{
    u32 done;
    bool compressed;
    sceCdlFILE *fileStruct;
    FileInfo *info;
    handle_file_loaded loaderCB;
    finish_async_callback finish;
    u8 *buffer;
    void *params;
    void *object;
} IOQueueItem;

static unsigned char asyncStack[0x18000] __attribute__((aligned(16))); // stack for ee_thread;

static int asyncThreadID = 0;
static u32 count = 0;
static float timeToLoad = 15.0f;

static Queue *asyncQueueWaiting = NULL;
static Queue *asyncQueueDone = NULL;

extern GameManager g_Manager;

static void IOThreadFunction(void *arg)
{
    while (1)
    {
        if (asyncQueueWaiting->count > 0)
        {
            IOQueueItem *item = (IOQueueItem *)PeekQueue(asyncQueueWaiting);
            float time1, time2;
            time1 = time2 = getTicks(g_Manager.timer);
            u32 currentSector = item->info->currentSector;
            u32 totalRead = item->info->totalRead;
            u32 totalToRead = item->info->totalSize;
            u8 *bufferLocation = item->buffer + totalRead;
            u32 bytesToRead = SECTOR_SIZE;
            while ((time2 - time1) < timeToLoad && totalRead < totalToRead)
            {
                u32 diff;
                if ((diff = (totalToRead - totalRead)) < SECTOR_SIZE)
                {
                    bytesToRead = diff;
                }
                u32 offset = currentSector * SECTOR_SIZE;
                u32 step = ReadFileBytes(item->fileStruct, bufferLocation, offset, bytesToRead);

                totalRead += step;
                bufferLocation += step;
                currentSector += 1;
                offset += step;

                time2 = getTicks(g_Manager.timer);
            }

            if (totalRead >= totalToRead)
            {
                IOQueueItem *itemToRemove = (IOQueueItem*)PopQueue(asyncQueueWaiting);
                itemToRemove->done = 1;
                AddQueueElement(asyncQueueDone, itemToRemove);
            }
            else
            {
                item->info->currentSector = currentSector;
                item->info->totalRead = totalRead;
            }

        }
        count++;
        SleepThread();
    }
}

static FileInfo *CreateFileInfo(u32 totalSize, u32 totalSectors)
{
    FileInfo *info = (FileInfo *)malloc(sizeof(FileInfo));

    if (!info)
    {
        ERRORLOG("FileInfo failed to create!\n");
        return info;
    }

    info->currentSector = info->totalRead = 0;
    info->totalSize = totalSize;
    info->totalSectors = totalSectors;
    return info;
}

static IOQueueItem *CreateIOQueueItem(sceCdlFILE *file,
                                      handle_file_loaded cb1,
                                      finish_async_callback finish,
                                      bool compressed,
                                      FileInfo *info,
                                      u8 *buffer,
                                      void *params,
                                      void *object)
{
    IOQueueItem *item = (IOQueueItem *)malloc(sizeof(IOQueueItem));

    if (!item)
    {
        ERRORLOG("IOQueueItem failed to create!\n");
        return item;
    }

    item->compressed = compressed;
    item->done = 0;
    item->fileStruct = file;
    item->finish = finish;
    item->loaderCB = cb1;
    item->info = info;
    item->buffer = buffer;
    item->params = params;
    item->object = object;
    return item;
}

static void AddIOQueueItem(IOQueueItem *item)
{
    if (asyncQueueWaiting)
        AddQueueElement(asyncQueueWaiting, item);
    else
        ERRORLOG("Queue not created when adding");
}

static void DeleteQueueItem(IOQueueItem *item)
{
    if (item)
    {
        if (item->fileStruct)
            free(item->fileStruct);
        if (item->info)
            free(item->info);
        if (item->buffer)
            free(item->buffer);
        if (item->params)
            free(item->params);
        free(item);
    }
}

void LoadASync(const char *name,
               void *object,
               void *params,
               handle_file_loaded loaderCB,
               finish_async_callback finish)

{
    char _file[MAX_FILE_NAME];
    Pathify(name, _file);

    int compressed = IsFileCompressed(_file);

    sceCdlFILE loc_file_struct; 
    
    bool ret = FindFileByName(_file, &loc_file_struct);

    if (!ret)
    {
        ERRORLOG("Cannot find file %s in async loader", name);
        return;
    }

    u32 sectors = loc_file_struct.size / SECTOR_SIZE;

    u32 remaining = loc_file_struct.size % SECTOR_SIZE;

    u32 bufferSize = loc_file_struct.size;

    if (remaining)
    {
        sectors++;
    }

    u8 *buffer = (u8 *)malloc(bufferSize);

    FileInfo *info = CreateFileInfo(bufferSize, sectors);

    IOQueueItem *item = CreateIOQueueItem(&loc_file_struct, loaderCB, finish,
                                          compressed, info, buffer, params,
                                          object);

    AddIOQueueItem(item);

   // INFOLOG("We are here! %d", asyncQueueWaiting->count);

    //WakeupThread(asyncThreadID);
}

void DebugPrintIO()
{
    DEBUGLOG("%d", count);
}

void InitASyncIO(int queueCount, float time)
{

    timeToLoad = time;

    ee_thread_t IOThread;

    asyncQueueWaiting = CreateQueue(queueCount, FIFO);

    if (!asyncQueueWaiting)
    {
        ERRORLOG("Cannot create IO Queue");
        return;
    }

    asyncQueueDone = CreateQueue(queueCount, FIFO);

    if (!asyncQueueDone)
    {
        ERRORLOG("Cannot create IO Queue");
        return;
    }

    IOThread.attr = 0;
    IOThread.option = 0;
    IOThread.func = &IOThreadFunction;
    IOThread.stack = asyncStack;
    IOThread.stack_size = sizeof(asyncStack);
    IOThread.gp_reg = &_gp;
    IOThread.initial_priority = 0x00;
    asyncThreadID = CreateThread(&IOThread);

    if (asyncThreadID == -1)
    {
        ERRORLOG("Cannot create IO thread");
        free(asyncQueueWaiting);
    }

    StartThread(asyncThreadID, NULL);
}

void SuspendIOThread()
{
    SuspendThread(asyncThreadID);
}

void WakeupIOThread()
{
    WakeupThread(asyncThreadID);
}

void DeinitASyncIO()
{
    s32 ret = DeleteThread(asyncThreadID);

    if (ret)
        ERRORLOG("Problem destroying ASync IO Thread");

    if (asyncQueueWaiting)
        free(asyncQueueWaiting);
}

void HandleASyncIO()
{
    if (asyncQueueDone->count > 0)
    {
        IOQueueItem *item = (IOQueueItem *)PopQueue(asyncQueueDone);
        if (!item)
        {
            // ERRORLOG("ASYNCIO : Popped done queue without object");
            return;
        }
        if (item->loaderCB)
            item->loaderCB(item->object, item->params, item->buffer, item->info->totalSize);
        if (item->finish)
            item->finish(item->object);
        DeleteQueueItem(item);
    }
}
