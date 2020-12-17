#pragma once
#include "DriverStructs.hpp"

#if defined(__cplusplus)
extern "C" {
#endif
#include <sys/ioccom.h>
#if defined(__cplusplus)
};
#endif

#define MIRA_IOCTL_BASE 'M'

// List of commands, this way everything keeps the same ID's
// NOTE: DO NOT REMOVE/ADD ANYTHING FROM THIS LIST
// ADDING NEW ENTRIES SHOULD ONLY BE DONE BEFORE THE MAX ENTRY, DO NOT RE-ORDER THIS SHIT OR ILL SLAP YOU IRL
#if defined(__cplusplus)
typedef enum class _MiraIoctlCmds : uint32_t
#else
enum MiraIoctlCmds
#endif
{
    CMD_None = 0,
    CMD_ThreadCredentials,
    CMD_ProcessList,
    CMD_ProcessInformation,
    CMD_MountInSandbox,
    CMD_CreateTrainerShm,
    CMD_GetTrainerShm,
    CMD_LoadTrainers,
    CMD_ReadProcessMemory,
    CMD_WriteProcessMemory,
    CMD_GetConfig,
    CMD_SetConfig,
    CMD_GetOriginalEntryPoint,
    CMD_MAX
#if defined(__cplusplus)
} MiraIoctlCmds;
#else
};
#endif


// Get/set the thread credentials
#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ThreadCredentials), sizeof(MiraThreadCredentials))

// Get a process id list
#define MIRA_GET_PID_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessList), sizeof(MiraProcessList))

// Get process information
#define MIRA_GET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessInformation), sizeof(MiraProcessInformation))

// Mount a path within sandbox
#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_MountInSandbox), sizeof(MiraMountInSandbox))

// Create new Shm
#define MIRA_TRAINERS_CREATE_SHM _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_CreateTrainerShm), sizeof(MiraCreateTrainerShm))

// Get the currently loaded shm's
#define MIRA_TRAINERS_GET_SHM _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetTrainerShm), sizeof(MiraGetTrainersShm))

// Trainers
#define MIRA_TRAINERS_LOAD _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_LoadTrainers), 0)
#define MIRA_TRAINERS_ORIG_EP _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetOriginalEntryPoint), 0)

// Read/Write process memory
#define MIRA_READ_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ReadProcessMemory), sizeof(MiraReadProcessMemory))
#define MIRA_WRITE_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_WriteProcessMemory), sizeof(MiraWriteProcessMemory))

// Configuration
#define MIRA_GET_CONFIG _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetConfig), 0)
#define MIRA_SET_CONFIG _IOC(IOC_OUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SetConfig), sizeof(MiraConfig))