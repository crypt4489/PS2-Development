#include "system/ps_vumanager.h"

#include <kernel.h>
#include <ee_regs.h>

#include <stdlib.h>

#include "system/ps_vif.h"
#include "log/ps_log.h"

volatile u32 *vu1_data_address = (volatile u32 *)0x1100c000;

volatile u32 *vu0_data_address = (volatile u32 *)0x11004000;

volatile u32 *vif1_top = (volatile u32 *)0x10003ce0;

volatile u32 *vif1_tops = (volatile u32 *)0x10003cc0;

#define VU1DATASIZE (1 << 14)
#define VU0DATASIZE (1 << 12)

int VU1CompleteHandler(s32 cause, void *arg, void *addr)
{
    GameManager *local_manager = (GameManager *)arg;
    local_manager->vu1DoneProcessing = true;
    *R_EE_VIF1_FBRST |= 1 << 3;
    return 0;
}

void SetupVU1INTEHandler()
{
    s32 id = AddIntcHandler2(5, VU1CompleteHandler, 0, &g_Manager);
    if (id <= 0)
    {
        ERRORLOG("failed to reg vif1 int handler!");
        return;
    }
    EnableIntc(5);
}

u32 SizeOfProgramInstructions(u32 *codeStart, u32 *codeEnd)
{
    u32 count = (codeEnd - codeStart) / 2;
    if (count & 1)
        count++;
    return count;
}

u32 SizeOfProgramPacket(u32 *codeStart, u32 *codeEnd)
{
    int count = SizeOfProgramInstructions(codeStart, codeEnd);
    return (count / 128) + 1;
}

void ReadFromVU(volatile u32 *start, int printOutSize, bool usefloatornot)
{
    int qWordCount = 0;
    volatile u32 *ptr = start;
    Bin2Float bf_val;
    printf("%d : ", qWordCount);
    for (int i = 1; i <= printOutSize; i++)
    {
        bf_val.int_x = *ptr;

        if (usefloatornot)
        {
            printf("%f ", bf_val.float_x);
        }
        else
        {
            printf("%x ", bf_val.int_x);
        }

        ptr++;

        if ((i % 4) == 0 && i != printOutSize)
        {
            qWordCount++;
            printf("\n");
            printf("%d : ", qWordCount);
        }
    }
    printf("\n");
}

/* VU1Manager Functions */

VU1Manager *CreateVU1Manager(u32 size)
{
    VU1Manager *manager = (VU1Manager *)malloc(sizeof(VU1Manager));

    if (!manager)
    {
        ERRORLOG("VU1Manager failed to allocate!");
        return manager;
    }

    manager->basePointer = 0;
    manager->numPrograms = size;
    manager->programIdentifier = 0;
    manager->programsInVU1 = 0;
    manager->programs = malloc(sizeof(VU1Program *) * size);

    if (!manager->programs)
    {
        ERRORLOG("VU1Manager programs array failed to allocate!");
        free(manager);
        return NULL;
    }

    return manager;
}

void DestroyVU1Manager(VU1Manager *manager)
{
    u32 size = manager->numPrograms;
    for (u32 i = 0; i < size; i++)
    {
        DestroyVU1Program(manager->programs[i]);
    }
    free(manager->programs);
    free(manager);
}

#define MAX_PROGRAMS 25
#define PROGRAM_STEP 5

void AddProgramToManager(VU1Manager *manager, VU1Program *program)
{
    if (program->size + manager->basePointer >= MAX_VU1_CODE_ADDRESS)
    {
        ERRORLOG("VU1 Code memory is full ");
        return;
    }

    if (manager->programsInVU1 > MAX_PROGRAMS)
    {
        ERRORLOG("Max programs in VU1 reached. Failed to add program");
        return;
    }

    if (manager->programsInVU1 > manager->numPrograms)
    {
        u32 size = manager->numPrograms + PROGRAM_STEP;
        manager->programs = realloc(manager->programs, sizeof(VU1Program *) * size);
        if (!manager->programs)
        {
            ERRORLOG("VU1Manager programs array failed to reallocate when adding!");
            return;
        }
        manager->numPrograms = size;
    }

    program->address = manager->basePointer;

    program->programId = manager->programIdentifier;

    manager->programs[program->programId] = program;

    u32 packetSize = SizeOfProgramPacket(program->codeStart, program->codeEnd);

    UploadProgramToVU1(program->codeStart, program->codeEnd, manager->basePointer, packetSize, program->size);

    manager->programIdentifier++;
    manager->basePointer += program->size;

    INFOLOG("%d %d %d %d", manager->basePointer, packetSize, program->size, manager->programIdentifier);

    manager->programsInVU1++;
}

u32 GetProgramAddressVU1Manager(VU1Manager *manager, u32 index)
{
    u16 _cache = manager->programsInVU1;
    if (index > _cache)
    {
        ERRORLOG("Cannot access VU1 Program at index %u with list size %u", index, _cache);
        return MAX_VU1_CODE_ADDRESS;
    }
    return GetProgramAddressVU1Program(manager->programs[index]);
}

/* VU1 Program Functions */

VU1Program *CreateVU1Program(u32 *codeStart, u32 *codeEnd, u32 stage)
{
    VU1Program *prog = (VU1Program *)malloc(sizeof(VU1Program));
    prog->address = 0;
    prog->programId = 0;
    prog->stage = stage;
    prog->codeEnd = codeEnd;
    prog->codeStart = codeStart;
    prog->size = SizeOfProgramInstructions(codeStart, codeEnd);
    return prog;
}

void DestroyVU1Program(VU1Program *prog)
{
    if (prog)
    {
        free(prog);
    }
}

u32 GetProgramAddressVU1Program(VU1Program *prog)
{
    return (u32)prog->address;
}
