#include "irx_imports.h"
#include "rpc_server.h"
#include "rpc_client.h"
#include "sound.h"
#include "sound_log.h"
static int rpc_buffer[18000 / 4];

static SifRpcDataQueue_t qd;

static SifRpcServerData_t sd0;

static void *rpc_command(int func, unsigned *data, int size)
{
    int ret, buffer_size;

    switch (func)
    {
    case 0:
        initialize_rpc_client();
        InitSoundIOP();
        ret = 0;
        break;
    case 1:
        ret = -1;
        break;
    case 2:
        LoadSampleToSPU((char*)data[0], data[1]);
        ret = 0;
        break;
    default:
        ret = -1;
        break;
    }
    data[0] = ret;
    return data;
}

static void rpc_server_thread(void *arg)
{
    SifInitRpc(0);

    LOGGER("Creating rpc server");

    SifSetRpcQueue(&qd, GetThreadId());
    SifRegisterRpc(&sd0, SOUND_IRX, (void *)rpc_command, rpc_buffer, NULL, NULL, &qd);
    SifRpcLoop(&qd);
}


int initialize_rpc_thread()
{
    int servid = 0;
    int rpc_tid = 0;
    int ret;
    iop_thread_t serv_thread;

    serv_thread.attr = TH_C;
    serv_thread.priority = 40;
    serv_thread.option = 0;
    serv_thread.stacksize = 0x800;
    serv_thread.thread = (void *)rpc_server_thread;

    rpc_tid = CreateThread(&serv_thread);

    LOGGER("RPC server thread 0x%x started", rpc_tid);

    StartThread(rpc_tid, NULL);

    return 0;
}
