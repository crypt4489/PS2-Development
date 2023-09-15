#include "io/ps_async.h"

#include <kernel.h>

#include "log/ps_log.h"


static unsigned char asyncStack[0x1800] __attribute__((aligned(16))); // stack for ee_thread;

static int asyncThreadID;

static void IOThreadFunction(void *arg)
{

}

void LoadASync(const char *name,
                void* object,
                void *params,
                handle_file_loaded loaderCB,
                finish_async_callback finish)

{

    u8 *buffer;

    int compressed = IsFileCompressed(name);

    sceCdlFILE *loc_file_struct = FindFileByName(name);

    if (loc_file_struct == NULL)
    {
        ERRORLOG("Cannot find file %s in async loader", name);
        return;
    }

    loaderCB(object, params, buffer);

    if (finish != NULL)
    {
        finish(object);
    }
}

void InitASyncIO()
{
    ee_thread_t IOThread;
    IOThread.attr = 0;
	IOThread.option = 0;
	IOThread.func = &IOThreadFunction;
	IOThread.stack = asyncStack;
	IOThread.stack_size = sizeof(asyncStack);
	IOThread.gp_reg = &_gp;
	IOThread.initial_priority = 0x60;
	asyncThreadID = CreateThread(&IOThread);
	StartThread(asyncThreadID, NULL);
}
