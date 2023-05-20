#include "rpc_server.h"


static int rpc_buffer[18000 / 4];

static SifRpcDataQueue_t qd;

static SifRpcServerData_t sd0;


static void *rpc_command(int func, unsigned *data, int size)
{
    int ret, buffer_size;

    switch (func)
    {
    case INIT_PROC_TEX:
        initialize_rpc_client();
        ret = 0;
        break;

    case CHECKER_BOARD_FUNCTION:
        generateBoard((unsigned char*)data[3], data[6], data[4], data[5], data[0], data[1]);
        read_from_iop((unsigned char *)data[3], data[6], (unsigned char *)data[2]);
        call_client_callback(1);
        ret = 1;
        break;
    case READ_FROM_IOP:
        read_from_iop((unsigned char *)data[0], data[1], (unsigned char *)data[2]);
        ret = data[1];
        break;
    case PERLIN_NOISE_FUNCTION:
        generatePerlin((unsigned char *)data[2], data[5], data[3], data[4], data[0]);
        read_from_iop((unsigned char *)data[2], data[5], (unsigned char *)data[1]);
        call_client_callback(1);
        ret = 1;
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

    printf("my iop mod: creating rpc server\n");

    SifSetRpcQueue(&qd, GetThreadId());
    SifRegisterRpc(&sd0, PROC_TEXTURE_IRX, (void *)rpc_command, rpc_buffer, NULL, NULL, &qd);
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
    printf("my irx: rpc server thread 0x%x started\n", rpc_tid);

    StartThread(rpc_tid, NULL);


    return 0;
}
