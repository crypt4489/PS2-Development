#include "audio/ps_sound.h"
#include "audio/ps_soundtypes.h"

#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>
#include <stdlib.h>

#include <audsrv.h>

#include "log/ps_log.h"
#include "io/ps_file_io.h"
#include "util/ps_misc.h"
#include "math/ps_fast_maths.h"

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

	if (!addr)
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

	if (!iop_addr)
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

	while (!(id = SifSetDma(&sifdma, 1)))
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

	if (!bufferLoad)
	{
		ERRORLOG("Cannot open wav file");
		return NULL;
	}

	WavFile *wavFile = (WavFile *)malloc(sizeof(WavFile));

	if (!wavFile)
	{
		ERRORLOG("Cannot create wav file holder");
		free(bufferLoad);
		return wavFile;
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

	buffer = bufferLoad + 20 + wavFile->header.fmt_length;

	char list[4];

	memcpy(&list, buffer, 4);

	if (!strncmp("data", list, 4))
	{
		memcpy(&wavFile->header.data_tag, list, 4);
	}
	else 
	{
		ERRORLOG("No DATA tag in the header file %s", list);
		free(bufferLoad);
		free(wavFile);
		return NULL;
	}

	// DEBUGLOG("%c%c%c%c", wavFile->header.data_tag[0], wavFile->header.data_tag[1], wavFile->header.data_tag[2], wavFile->header.data_tag[3]);

	buffer += 4;

	memcpy(&wavFile->header.data_length, buffer, 4);

	int dataSize = wavFile->header.data_length;

	// DEBUGLOG("%d", dataSize);

	buffer += 4;

	wavFile->samples = (u8 *)malloc(dataSize);

	if (!wavFile->samples)
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

	if (!bufferLoad)
	{
		ERRORLOG("Cannot open Vag file");
		return NULL;
	}

	VagFile *vagFile = (VagFile *)malloc(sizeof(VagFile));

	if (!vagFile)
	{
		ERRORLOG("Cannot create vag file holder");
		free(bufferLoad);
		return vagFile;
	}

	u8 *buffer = bufferLoad;

	u32 dataSize;

	memcpy(&vagFile->header, buffer, 48);

	buffer += 64;

	vagFile->header.version = SWAP_ENDIAN(vagFile->header.version);
	vagFile->header.sampleRate = SWAP_ENDIAN(vagFile->header.sampleRate);
	vagFile->header.dataLength = SWAP_ENDIAN(vagFile->header.dataLength);

	dataSize = vagFile->header.dataLength;

	vagFile->samples = (u8 *)malloc(dataSize + 16);

	if (!vagFile->samples)
	{
		ERRORLOG("Failed to allocate samples buffer");
		free(vagFile);
		free(bufferLoad);
		return NULL;
	}

	CreateVagSamplesBuffer(vagFile, buffer);

	free(bufferLoad);

	return vagFile;
}

void CreateVagSamplesBuffer(VagFile *vagFile, u8 *buffer)
{
	u32 pitch = (vagFile->header.sampleRate * 4096) * vagFile->header.channels / 48000;

	vagFile->samples[4] = 0x00;
	vagFile->samples[5] = 0x00;
	vagFile->samples[6] = 0x00;
	vagFile->samples[7] = 0x00;

	memcpy(vagFile->samples + 8, &pitch, 4);

	memcpy(vagFile->samples + 16, buffer, vagFile->header.dataLength);
}

s16* Convert16PCMToShortSamples(u8 *buffer, u32 size)
{
	s16 *samples = (s16*)malloc(sizeof(s16) * (size));

	if (!samples)
	{
		ERRORLOG("Failed to allocate samples");
		return samples;
	}

	for (u32 i = 0; i<size; i++)
	{
		u16 topByte = ((u16)(buffer[i * 2]));
		u16 lowerByte = ((u16)buffer[(i * 2) + 1]) << 8;
		samples[i] = topByte | lowerByte;
	}

	return samples;
}

s16* Convert8PCMToShortSamples(u8 *buffer, u32 size)
{
	s16 *samples = (s16*)malloc(sizeof(s16) * (size));

	if (!samples)
	{
		ERRORLOG("Failed to allocate samples");
		return samples;
	}

	for (u32 i = 0; i<size; i++)
	{
		samples[i] = (s16)(buffer[i] - 0x80) << 8;
	}

	return samples;
}


u8* CreateVagSamples(s16* samples, u32 len, u32* outSize, u32 loopStart, u32 loopEnd, bool loopFlag)
{
	float _hist_1 = 0.0, _hist_2 = 0.0;
	float hist_1 = 0.0, hist_2 = 0.0;

	int fullChunks = len / 28;
	int remaining = len % 28;

	if (remaining)
	{
		fullChunks++;
	}

	int sizeOfOut = fullChunks * 16;

	u8* outBuffer = (u8*)malloc(sizeOfOut);

	u8* ret = outBuffer;

	u32 bytesRead = 0;
	EncBlock block;

	for (int i = 0; i < fullChunks; i++)
	{

		int chunkSize = 28;
		if (i == fullChunks - 1)
		{
			chunkSize = remaining;
		}
		int predict = 0, shift;
		float min = 1e10;
		float s_1 = 0.0, s_2 = 0.0;
		float predictBuf[28][5];
		for (int j = 0; j < 5; j++)
		{
			float max = 0.0;

			s_1 = _hist_1;
			s_2 = _hist_2;

			for (int k = 0; k < chunkSize; k++)
			{
				float sample = samples[k];
				if (sample > 30719.0)
				{
					sample = 30719.0;
				}
				if (sample < -30720.0)
				{
					sample = -30720.0;
				}

				float ds = sample + s_1 * vaglut[j][0] + s_2 * vaglut[j][1];

				predictBuf[k][j] = ds;

				if (Abs(ds) > max)
				{
					max = Abs(ds);
				}

				s_2 = s_1;
				s_1 = sample;
			}
			if (max < min)
			{
				min = max;
				predict = j;
			}
			if (min <= 7)
			{
				predict = 0;
				break;
			}
		}
		
		_hist_1 = s_1;
		_hist_2 = s_2;

		float d_samples[28];
		for (int i = 0; i < 28; i++)
		{
			d_samples[i] = predictBuf[i][predict];
		}

		int min2 = (int)min;
		int shift_mask = 0x4000;
		shift = 0;

		while (shift < 12)
		{
			if (shift_mask & (min2 + (shift_mask >> 3)))
			{
				break;
			}
			shift++;
			shift_mask >>= 1;
		}

		block.predict = predict;
		block.shift = shift;

		if (len - bytesRead > 28)
		{
			block.flags = VAGF_NOTHING;
			if (loopFlag)
			{
				block.flags = VAGF_LOOP_REGION;
				if (i == loopStart)
				{
					block.flags = VAGF_LOOP_START;
				}
				if (i == loopEnd)
				{
					block.flags = VAGF_LOOP_END;
				}
			}
		}
		else
		{
			block.flags = VAGF_LOOP_LAST_BLOCK;
			if (loopFlag)
			{
				block.flags = VAGF_LOOP_END;
			}
		}

		s16 outBuf[28];
		memset(outBuf, '\0', 56);
		for (int k = 0; k < 28; k++)
		{
			float s_double_trans = d_samples[k] + hist_1 * vaglut[predict][0] + hist_2 * vaglut[predict][1];
			float s_double = s_double_trans * (1 << shift);
			int sample = (int)(((int)s_double + 0x800) & 0xFFFFF000);

			if (sample > 32767)
			{
				sample = 32767;
			}
			if (sample < -32768)
			{
				sample = -32768;
			}

			outBuf[k] = (short)sample;

			sample >>= shift;
			hist_2 = hist_1;
			hist_1 = sample - s_double_trans;
		}

		for (int k = 0; k < 14; k++)
		{
			block.sample[k] = (u8)(((outBuf[(k * 2) + 1] >> 8) & 0xf0) | ((outBuf[k * 2] >> 12) & 0xf));
		}

		samples += 28;
		
		s8 lastPredictAndShift = (((block.predict << 4) & 0xF0) | (block.shift & 0x0F));

		*outBuffer++ = lastPredictAndShift;
		*outBuffer++ = block.flags;
		for (int h = 0; h < 14; h++)
			*outBuffer++ = block.sample[h];

		bytesRead += chunkSize;
	}

	*outSize = sizeOfOut;
	
	return ret;
}

VagFile *ConvertRawPCMToVag(u8 *buffer, u32 size, u32 sampleRate, u32 channels, u32 bitDepth)
{
	VagFile *vagFile = (VagFile *)malloc(sizeof(VagFile));

	if (!vagFile)
	{
		ERRORLOG("Cannot create vag file holder");
		return vagFile;
	}
	
	s16 *pcmSamples = NULL;
	u32 conversionSize = size;

	switch(bitDepth)
	{
		case 8:
			pcmSamples = Convert8PCMToShortSamples(buffer, conversionSize);
			break;
		case 16:
			conversionSize >>= 1;
			pcmSamples = Convert16PCMToShortSamples(buffer, conversionSize);
			break;
		default:
			ERRORLOG("Incorrect bitDepth for PCM");
			break;
	}

	if (!pcmSamples)
	{
		free(vagFile);
		ERRORLOG("PCM sample creation failed");
		return NULL;
	}

	u32 outVagSize = 0;

	u8 *vagSamples = CreateVagSamples(pcmSamples, conversionSize, &outVagSize, 0, 0, 0);
	
	vagFile->header.sampleRate = sampleRate;
	vagFile->header.channels = channels;
	vagFile->header.dataLength = outVagSize;
	vagFile->samples = (u8 *)malloc(outVagSize + 16);

	CreateVagSamplesBuffer(vagFile, vagSamples);
	
	free(pcmSamples);
	free(vagSamples);

	return vagFile;
}


void DestroyVAGFile(VagFile *file)
{
	if (file)
	{
		free(file->samples);
		free(file);
	}
}

void DestroyWAVFile(WavFile *file)
{
	if (file)
	{
		free(file->samples);
		free(file);
	}
}

void AudioInitialization()
{
	int ret;
	
    ret = SifLoadModule("cdrom0:\\LIBSD.IRX", 0, NULL);

    ret = SifLoadModule("cdrom0:\\AUDSRV.IRX", 0, NULL);

    ret = audsrv_init();

    audsrv_adpcm_init();
}