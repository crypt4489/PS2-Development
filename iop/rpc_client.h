#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include "proc_texture.h"
#include "irx_imports.h"


void initialize_rpc_client(void);
void deinitialize_rpc_client(void);
void call_client_callback(int id);

#endif