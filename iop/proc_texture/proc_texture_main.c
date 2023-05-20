// This include will allow us to avoid reincluding other headers
#include "irx_imports.h"

#define MODNAME "proc texture"
IRX_ID("PROC TEXTURE", 1, 1);

extern struct irx_export_table _exp_hello;


// This contain our prototype for our export, and we need it to call it inside our _start
#include "proc_texture.h"
#include "rpc_server.h"
#include "rpc_client.h"


static void process_thread(void *arg);

static int processing_thread_id;





int initialize_process_thread()
{

    /*iop_event_t proc_event;

    proc_event.attr = EA_MULTI;
    proc_event.bits = 0;

    processing_event_flag = CreateEventFlag(&proc_event);

    //ClearEventFlag(processing_event_flag, ~BEGIN_PROCESSING);

    printf("my event flag: %d\n", processing_event_flag);

    int ret = 0;
    iop_thread_t proc_thread;
    proc_thread.attr = TH_C;
    proc_thread.priority = 39;
    proc_thread.option = 0;
    proc_thread.stacksize = 0x800;
    proc_thread.thread = (void *)process_thread;



    processing_thread_id = CreateThread(&proc_thread);

    printf("proc thread id: %d\n", processing_thread_id);

    ret = StartThread(processing_thread_id, NULL);

    printf("proc thread started: %d\n", ret);
*/
    return 0;
}

// This is a bit like a "main" for IRX files.
int _start(int argc, char *argv[])
{
    if (RegisterLibraryEntries(&_exp_hello) != 0)
        return 1;

    initialize_rpc_thread();

    return 0;
}
