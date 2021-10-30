#pragma once
#include "DriverStructs.hpp"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
#endif

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
    CMD_LoadTrainers,               // Requests for the trainer sandbox directories to be mounted and to load trainers
    CMD_GetOriginalEntryPoint,      // Gets the original entry point by process id
    
    CMD_TRAINERS_END = 40,
    // 41-60 = Debugger
    CMD_DEBUGGER_START = 41,
    CMD_GetProcessInformation,      // Get process information
    CMD_GetThreadCredentials,       // Get thread credentials
    CMD_SetThreadCredentials,       // Set thread credentials
    CMD_DEBUGGER_END = 60,

    // 61-90 = Mira Reserved
    CMD_MIRA_START = 61,
    CMD_GetConfig,                  // Get mira's kernel configuration
    CMD_SetConfig,                  // Set mira's kernel configuration

    CMD_MountInSandbox,             // Mount full unescaped sandbox path into the current sandbox
    CMD_SetThreadPrivMask,          // Gets/Sets the priv mask
    
    // System
    CMD_SystemAllocateMemory,       // Allocate memory by pid
    CMD_SystemFreeMemory,           // Free memory by pid
    CMD_SystemReadProcessMemory,    // Read process memory by pid
    CMD_SystemWriteProcessMemory,   // Write process memory by pid
    CMD_SystemGetProcessList,       // Get process list

    CMD_MIRA_END = 90,
    CMD_MAX
#if defined(__cplusplus)
} MiraIoctlCmds;
#else
};
#endif


// Get/set the thread credentials
#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetThreadCredentials), sizeof(MiraThreadCredentials))
#define MIRA_SET_PROC_THREAD_CREDENTIALS _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SetThreadCredentials), sizeof(MiraThreadCredentials))

// Get process detailed information
#define MIRA_GET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetProcessInformation), sizeof(MiraProcessInformation))

// Mount a path within sandbox
#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_MountInSandbox), sizeof(MiraMountInSandbox))

// Create new Shm
#define MIRA_TRAINERS_CREATE_SHM _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_CreateTrainerShm), sizeof(MiraCreateTrainerShm))

// Get the currently loaded shm's
#define MIRA_TRAINERS_GET_SHM _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetTrainerShm), sizeof(MiraGetTrainersShm))

// Trainers
#define MIRA_TRAINERS_LOAD _IOC(IOC_VOID, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_LoadTrainers), 0)
#define MIRA_TRAINERS_ORIG_EP _IOC(IOC_OUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetOriginalEntryPoint), sizeof(uint64_t))

/*
    System
*/

// Allocate/Free process memory
#define MIRA_ALLOCATE_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SystemAllocateMemory), sizeof(MiraAllocateMemory))
#define MIRA_FREE_PROCESS_MEMORY _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SystemFreeMemory), sizeof(MiraFreeMemory))

// Read/Write process memory
#define MIRA_READ_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SystemReadProcessMemory), sizeof(MiraReadProcessMemory))
#define MIRA_WRITE_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SystemWriteProcessMemory), sizeof(MiraWriteProcessMemory))

// Configuration
#define MIRA_GET_CONFIG _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_GetConfig), 0)
#define MIRA_SET_CONFIG _IOC(IOC_OUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SetConfig), sizeof(MiraConfig))

// Get process list
#define MIRA_GET_PID_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SystemGetProcessList), sizeof(MiraProcessList))

#define MIRA_SET_THREAD_PRIV_MASK _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_SetThreadPrivMask), sizeof(MiraSetThreadPrivMask))