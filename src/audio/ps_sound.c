#include "audio/ps_sound.h"

#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>
#include <stdlib.h>

#include "log/ps_log.h"
#include "io/ps_file_io.h"
#include "math/ps_misc.h"

extern void *_gp;

static struct t_SifRpcClientData cd0;
static unsigned int sbuff[4096] __attribute__((aligned(64))); // buffer between rpc and ee
static struct t_SifRpcDataQueue cb_queue;
static struct t_SifRpcServerData cb_srv;
static unsigned char rpc_server_stack[0x1800] __attribute__((aligned(16))); // stack for ee_thread;

static int rpc_server_thread_id;

static int completionSema;

static int soundMutex;

void *iop_addr;

static void *my_ee_rpc_handler(int fnum, void *buffer, int len)
{
	switch (fnum)
	{
	case 1:
	}

	return buffer;
}

static void rpc_server_thread(void *arg)
{
	static unsigned char cb_rpc_buffer[64] __attribute__((aligned(64)));
	SifSetRpcQueue(&cb_queue, GetThreadId());
	SifRegisterRpc(&cb_srv, SOUND_IRX, &my_ee_rpc_handler, cb_rpc_buffer, NULL, NULL, &cb_queue);
	SifRpcLoop(&cb_queue);
}

static void *AllocIopSpace(int bytes)
{
	void *addr = SifAllocIopHeap(bytes);

	if (addr == 0)
	{
		return NULL;
	}

	return addr;
}

int WaitAudio(int buflen)
{
	sbuff[0] = buflen;
	SifCallRpc(&cd0, 1, 0, sbuff, 4, sbuff, 4, NULL, NULL);
	return sbuff[0];
}

void LoadSample(unsigned char *wav, int size)
{

	iop_addr = AllocIopSpace(size);

	if (iop_addr == NULL)
	{
		ERRORLOG("Cannot allocate sample buffer on IOP");
		return;
	}

	SifDmaTransfer_t sifdma;
	int id;

	sifdma.src = wav;
	sifdma.dest = iop_addr;
	sifdma.size = size;
	sifdma.attr = 0;

	while ((id = SifSetDma(&sifdma, 1)) == 0)
		;
	while (SifDmaStat(id) >= 0)
		;

	WaitSema(completionSema);

	sbuff[0] = (int)iop_addr;
	sbuff[1] = size;

	SifCallRpc(&cd0, 2, 0, sbuff, 8, sbuff, 4, NULL, NULL);

	SignalSema(completionSema);
}

void InitSound()
{
	int ret;

	ret = SifLoadModule("cdrom0:\\LIBSD.IRX", 0, NULL);

	if (ret < 0)
	{
		ERRORLOG("Cannot load lidsd\n");
		return;
	}

	ret = SifLoadModule("cdrom0:\\SOUND.IRX", 0, NULL);

	if (ret < 0)
	{
		ERRORLOG("Cannot load sound.irx\n");
		return;
	}

	ee_sema_t compSema, soundSema;
	ee_thread_t rpcClientThread;

	ret = 0;
	memset(&cd0, '\0', sizeof(cd0));

	while (1)
	{
		if (SifBindRpc(&cd0, SOUND_IRX, 0) < 0)
		{
			ERRORLOG("error error connecting ee to iop sound rpc thread");
			return;
		}

		if (cd0.server != 0)
		{
			break;
		}

		nopdelay();
	}

	soundSema.init_count = 1;
	soundSema.max_count = 1;
	soundSema.option = 0;

	soundMutex = CreateSema(&soundSema);

	if (soundMutex < 0)
	{
		ERRORLOG("no sema for completed");
		return;
	}

	compSema.init_count = 1;
	compSema.max_count = 1;
	compSema.option = 0;

	completionSema = CreateSema(&compSema);

	if (completionSema < 0)
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

	SifCallRpc(&cd0, 0, 0, sbuff, 64, sbuff, 64, NULL, NULL);
	ret = sbuff[0];

	if (ret != 0)
	{
		ERRORLOG("error from iop sound init %d", ret);
	}

	SifInitIopHeap();
}

WavFile *LoadWavFile(const char *name)
{
	char path[MAX_FILE_NAME];
	Pathify(name, path);

	u32 fileSize;

	u8 *bufferLoad = ReadFileInFull(path, &fileSize);

	if (bufferLoad == NULL)
	{
		ERRORLOG("Cannot open wav file");
		return NULL;
	}

	WavFile *wavFile = (WavFile *)malloc(sizeof(WavFile));

	if (wavFile == NULL)
	{
		ERRORLOG("Cannot create wav file holder");
		free(bufferLoad);
		return NULL;
	}

	u8 *buffer = bufferLoad;

	memcpy(&wavFile->header, buffer, 36);

	buffer += 36;

	// DEBUGLOG("%c%c%c%c", wavFile->header.riff_tag[0], wavFile->header.riff_tag[1], wavFile->header.riff_tag[2], wavFile->header.riff_tag[3]);

	// DEBUGLOG("%d", wavFile->header.riff_length);

	// DEBUGLOG("%c%c%c%c", wavFile->header.wav_tag[0], wavFile->header.wav_tag[1], wavFile->header.wav_tag[2], wavFile->header.wav_tag[3]);

	// DEBUGLOG("%c%c%c%c", wavFile->header.fmt_tag[0], wavFile->header.fmt_tag[1], wavFile->header.fmt_tag[2],  wavFile->header.fmt_tag[3] );

	// DEBUGLOG("%d", wavFile->header.fmt_length);

	// DEBUGLOG("%d", wavFile->header.audio_format);

	// DEBUGLOG("CHANNELS %d", wavFile->header.num_channels);

	// DEBUGLOG("SAMPLE RATE %d", wavFile->header.sample_rate);

	// DEBUGLOG("%d", wavFile->header.byte_rate);

	// DEBUGLOG("%d", wavFile->header.block_align);

	// DEBUGLOG("BPS %d", wavFile->header.bits_per_sample);

	char list[4];

	memcpy(&list, buffer, 4);

	if (strncmp("data", list, 4) == 0)
	{
		memcpy(&wavFile->header.data_tag, list, 4);
	}
	else if (strncmp("list", list, 4) == 0)
	{
		// DEBUGLOG("%x %x %x %x", list[0], list[1], list[2], list[3]);
		int skip;
		buffer += 4;
		memcpy(&skip, buffer, 4);
		////DEBUGLOG("SKIP %d", skip);
		buffer += (skip + 4);
		memcpy(&wavFile->header.data_tag, buffer, 4);
	}
	else if (list[0] == 0 && list[1] == 0)
	{
		buffer += 6;
		int skip;
		memcpy(&skip, buffer, 4);
		////DEBUGLOG("SKIP %d", skip);
		buffer += (skip + 4);
		memcpy(&wavFile->header.data_tag, buffer, 4);
	}

	// DEBUGLOG("%c%c%c%c", wavFile->header.data_tag[0], wavFile->header.data_tag[1], wavFile->header.data_tag[2], wavFile->header.data_tag[3]);

	buffer += 4;

	memcpy(&wavFile->header.data_length, buffer, 4);

	int dataSize = wavFile->header.data_length;

	// DEBUGLOG("%d", dataSize);

	buffer += 4;

	wavFile->samples = (u8 *)malloc(dataSize);

	if (wavFile->samples == NULL)
	{
		ERRORLOG("Failed to allocate samples buffer");
		free(wavFile);
		free(bufferLoad);
		return NULL;
	}

	memcpy(wavFile->samples, buffer, dataSize);

	free(bufferLoad);

	return wavFile;
}

VagFile *LoadVagFile(const char *name)
{
	char path[MAX_FILE_NAME];

	Pathify(name, path);

	u32 fileSize;

	u8 *bufferLoad = ReadFileInFull(path, &fileSize);

	if (bufferLoad == NULL)
	{
		ERRORLOG("Cannot open Vag file");
		return NULL;
	}

	VagFile *vagFile = (VagFile *)malloc(sizeof(VagFile));

	if (vagFile == NULL)
	{
		ERRORLOG("Cannot create wav file holder");
		free(bufferLoad);
		return NULL;
	}

	u8 *buffer = bufferLoad;

	u32 dataSize;

	memcpy(&vagFile->header, buffer, 48);

	buffer += 64;

	dataSize = vagFile->header.dataLength;

	vagFile->samples = (u8 *)malloc(dataSize + 16);

	if (vagFile->samples == NULL)
	{
		ERRORLOG("Failed to allocate samples buffer");
		free(vagFile);
		free(bufferLoad);
		return NULL;
	}

	u32 pitch = (vagFile->header.sampleRate * vagFile->header.channels * 4096) / 48000;

	vagFile->samples[4] = vagFile->samples[5] = vagFile->samples[6] = vagFile->samples[7] = 0x02;

	memcpy(vagFile->samples + 8, &pitch, 4);

	memcpy(vagFile->samples + 16, buffer, dataSize);

	free(bufferLoad);

	return vagFile;
}