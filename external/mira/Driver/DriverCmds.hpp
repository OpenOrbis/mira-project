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
    // 0 - 20 = General Use
    CMD_GENERAL_START = 0,
    CMD_None = 0,
    CMD_GENERAL_END = 20,
    
    // 21-40 = Trainers
    CMD_TRAINERS_START = 21,
    CMD_CreateTrainerShm,           // TODO: Implement
    CMD_GetTrainerShm,              // TODO: Implement
    CMD_LoadTrainers,               // TODO: Implement
    CMD_GetOriginalEntryPoint,      // Gets the original entry point by process id
    
    CMD_TRAINERS_END = 40,
    // 41-60 = Debugger
    CMD_DEBUGGER_START = 41,
    CMD_ProcessList,                // Get process list
    CMD_ProcessInformation,         // Get process information
    CMD_ReadProcessMemory,          // Read process memory
    CMD_WriteProcessMemory,         // Write process memory
    CMD_ThreadCredentials,          // Get/Set thread credentials
    CMD_DEBUGGER_END = 60,

    // 61-90 = Mira Reserved
    CMD_MIRA_START = 61,
    CMD_GetConfig,                  // Get mira's kernel configuration
    CMD_SetConfig,                  // Set mira's kernel configuration

    CMD_MountInSandbox,             // Mount full unescaped sandbox path into the current sandbox
    CMD_MIRA_END = 90,
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
#define MIRA_TRAINERS_LOAD _IOC(IOC_VOID, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_LoadTrainers), 0)
#define MIRA_TRAINERS_ORIG_EP _IOC(IOC_OUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetOriginalEntryPoint), sizeof(uint64_t))

// Read/Write process memory
#define MIRA_READ_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ReadProcessMemory), sizeof(MiraReadProcessMemory))
#define MIRA_WRITE_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_WriteProcessMemory), sizeof(MiraWriteProcessMemory))

// Configuration
#define MIRA_GET_CONFIG _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetConfig), 0)
#define MIRA_SET_CONFIG _IOC(IOC_OUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SetConfig), sizeof(MiraConfig))