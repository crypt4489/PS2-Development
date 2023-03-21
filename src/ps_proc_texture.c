#include "ps_proc_texture.h"
#include "ps_log.h"
#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>
extern void *_gp;

static struct t_SifRpcClientData cd0;
static unsigned int sbuff[1024] __attribute__((aligned (64))); //buffer between rpc and ee
static struct t_SifRpcDataQueue cb_queue;
static struct t_SifRpcServerData cb_srv;
static unsigned char rpc_server_stack[0x1800] __attribute__((aligned (16))); //stack for ee_thread;


static int rpc_server_thread_id;

static int completion_sema;

static int textureMutex;


static void *my_ee_rpc_handler(int fnum, void *buffer, int len)
{
	switch(fnum){
        case 1:

	}

	return buffer;
}


static void rpc_server_thread(void *arg)
{
    static unsigned char cb_rpc_buffer[64] __attribute__((aligned(64)));
    SifSetRpcQueue(&cb_queue, GetThreadId());
	SifRegisterRpc(&cb_srv, PROC_TEXTURE_IRX, &my_ee_rpc_handler, cb_rpc_buffer, NULL, NULL, &cb_queue);
	SifRpcLoop(&cb_queue);
}

void init_rpc()
{

    ee_sema_t compSema, texSema;
    ee_thread_t rpcClientThread;


    int ret = 0;
    memset(&cd0, '\0', sizeof(cd0));

    while (1)
	{
		if (SifBindRpc(&cd0, PROC_TEXTURE_IRX, 0) < 0)
		{
			ERRORLOG("error error connecting ee to iop proc tex rpc thread");
			return;
		}

 		if (cd0.server != 0)
		{
			break;
		}

		nopdelay();

	}

	texSema.init_count = 1;
	texSema.max_count = 1;
	texSema.option = 0;

	textureMutex = CreateSema(&texSema);



    compSema.init_count = 1;
	compSema.max_count = 1;
	compSema.option = 0;
	completion_sema = CreateSema(&compSema);

    if (completion_sema < 0)
	{
        ERRORLOG("no sema for completed");
		return;
	}

	rpcClientThread.attr = 0;
	rpcClientThread.option = 0;
	rpcClientThread.func = &rpc_server_thread;
	rpcClientThread.stack = rpc_server_stack;
	rpcClientThread.stack_size = sizeof(rpc_server_stack);
    rpcClientThread.gp_reg = &_gp;
	rpcClientThread.initial_priority = 0x60;
	rpc_server_thread_id = CreateThread(&rpcClientThread);
	StartThread(rpc_server_thread_id, NULL);


    SifCallRpc(&cd0, INIT_PROC_TEX, 0, sbuff, 64, sbuff, 64, NULL, NULL);
    ret = sbuff[0];
    if (ret != 0)
	{
        DEBUGLOG("%d", ret);
	}

	SifInitIopHeap();
}

void copy_data_to_iop(void* buffer, int bytes, void* iop_addr)
{

    SifDmaTransfer_t sifdma;
	int id;


    sifdma.src = (void*)buffer;
	sifdma.dest = (void*)iop_addr;
	sifdma.size = bytes;
	sifdma.attr = 0;

	while((id = SifSetDma(&sifdma, 1)) == 0);
	while(SifDmaStat(id) >= 0);

	WaitSema(completion_sema);

	sbuff[0] = (int)iop_addr;
	sbuff[1] = bytes;
	sbuff[2] = (int)buffer;

    SifCallRpc(&cd0, WRITE_TO_IOP, 0, sbuff, 12, sbuff, 4, NULL, NULL);

    SignalSema(completion_sema);
}

void *allocIopSpace(int bytes)
{
    void *addr = SifAllocIopHeap(bytes);

    if (addr == 0)
	{
		return NULL;
	}

	return addr;
}


void generate_iop_checker_tex(proc_texture *proc, void* iop_addr, int size, void* ee_addr, int context, int angle)
{

    sbuff[0] = context;
    sbuff[1] = angle;
    memcpy(&sbuff[2], proc, sizeof(proc_texture));


    SifCallRpc(&cd0, CHECKER_BOARD_FUNCTION, 0, sbuff, sizeof(proc_texture)+8, sbuff, 4, NULL, NULL);
}


void generate_iop_perlin_tex(proc_texture *proc, void* iop_addr, int size, void* ee_addr, int context)
{

    sbuff[0] = context;
    memcpy(&sbuff[1], proc, sizeof(proc_texture));

    SifCallRpc(&cd0, PERLIN_NOISE_FUNCTION, 0, sbuff, sizeof(proc_texture)+4, sbuff, 4, NULL, NULL);
}

void copy_data_from_iop(void* buffer, int bytes, void* iop_addr)
{
    sbuff[0] = (u32)iop_addr;
    sbuff[1] = bytes;
    sbuff[2] = (u32)buffer;

    WaitSema(completion_sema);


    SifCallRpc(&cd0, READ_FROM_IOP, 0, sbuff, 12, sbuff, 4, NULL, NULL);

    SignalSema(completion_sema);

    FlushCache(0);
}
