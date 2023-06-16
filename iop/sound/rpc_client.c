#include "irx_imports.h"
#include "rpc_client.h"
#include "sound.h"
#include "sound_log.h"
static int rpc_sema;

static SifRpcClientData_t client;

void initialize_rpc_client(void)
{
    rpc_sema = CreateMutex(IOP_MUTEX_UNLOCKED);

    client.server = NULL;

    while (sceSifBindRpc(&client, SOUND_IRX, 0) < 0 || client.server == NULL)
        DelayThread(500);

    LOGGER("Created RPC Client");
}

void deinitialize_rpc_client(void)
{
    memset(&client, 0, sizeof(client));
    DeleteSema(rpc_sema);
}

static void call_end_callback(void *end_param)
{
    iSignalSema(rpc_sema);
}

void call_client_callback(int id)
{
    WaitSema(rpc_sema);
    sceSifCallRpc(&client, id, SIF_RPC_M_NOWAIT, NULL, 0, NULL, 0, &call_end_callback, NULL);
}
