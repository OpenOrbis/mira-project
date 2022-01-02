#pragma once
#include "../Ioc.hpp"

typedef struct _TrainerLoad
{
    uint8_t Padding[0];
} TrainerLoad;
#define MIRA_TRAINER_LOAD _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_TrainerLoad), sizeof(TrainerLoad))


typedef struct _TrainerGetEntryPoint
{
    // Original entry point that had been stored before
    uint64_t OriginalEntryPoint;
} TrainerGetEntryPoint;
#define MIRA_TRAINER_GET_ENTRY_POINT _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_TrainerGetEntryPoint), sizeof(TrainerEntryPoint))
