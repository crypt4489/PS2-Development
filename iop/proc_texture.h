#ifndef __PROC_TEX_H__
#define __PROC_TEX_H__

#define INIT_PROC_TEX 0
#define CHECKER_BOARD_FUNCTION 5
#define PERLIN_NOISE_FUNCTION 4
#define READ_FROM_IOP 3

#define BEGIN_PROCESSING 0x10
#define	PROC_TEXTURE_IRX 0x9811789
// Let's have a prototype for our export!


void read_from_iop(unsigned char* iop_addr, int bytes, unsigned char* ee_buffer);

void SetupTimer();


int initialize_process_thread();

#endif

