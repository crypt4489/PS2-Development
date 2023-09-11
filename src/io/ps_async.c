#include "io/ps_async.h"

void LoadASync(const char *name, 
                void* object, 
                void *params, 
                handle_file_loaded* loaderCB,
                finish_async_callback* finish)

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