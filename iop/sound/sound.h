#ifndef IOP_SOUND_H
#define IOP_SOUND_H
#define	SOUND_IRX 0x14410998
void read_from_iop(unsigned char* iop_addr, int bytes, unsigned char* ee_buffer);
void InitSoundIOP();
void LoadSampleToSPU(char *addr, unsigned int buflen);
#endif