#pragma once
#include "../Ioc.hpp"

typedef struct _ProcessAllocateMemory
{
    // Process id to allocate memory in (<= 0 for current process)
    int32_t ProcessId;

    // Allocation size
    uint32_t Size;

    // Protection to set (RWX allowed)
    int32_t Protection;

    // Output pointer (should be set to nullptr on request, will be filled or nullptr on return)
    uint8_t* Pointer;
} ProcessAllocateMemory;
#define MIRA_PROCESS_ALLOCATE_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessAllocateMemory), sizeof(ProcessAllocateMemory))

typedef struct _ProcessFreeMemory
{
    // Process id to free memory previously allocated with ProcessAllocateMemory (<= 0 for current process)
    int32_t ProcessId;

    // Size of the allocation
    uint32_t Size;

    // Pointer from ProcessAllocateMemory
    void* Pointer;
} ProcessFreeMemory;
#define MIRA_PROCESS_FREE_MEMORY _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessFreeMemory), sizeof(ProcessFreeMemory))

typedef struct _ProcessReadMemory
{
    // <= 0 for calling process
    int32_t ProcessId;

    // Address to read from process
    void* Address;

    // Size of data to read
    uint32_t DataSize;

    // Read data output
    uint8_t Data[];
} ProcessReadMemory;
#define MIRA_PROCESS_READ_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessReadMemory), sizeof(ProcessReadMemory))

typedef struct _ProcessWriteMemory
{
    // <= 0 for calling process
    int32_t ProcessId;

    // Address to write to process
    void* Address;

    // Size of the data to write
    uint32_t DataSize;

    // Data to write
    uint8_t Data[];
} ProcessWriteMemory;
#define MIRA_PROCESS_WRITE_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessWriteMemory), sizeof(ProcessWriteMemory))

typedef struct _ProcessProtectMemory
{
    // <= 0 for calling process
    int32_t ProcessId;

    // Protection to set
    int32_t Protection;

    // Size to protect
    uint32_t Size;

    // Address to protect
    void* Address;
} ProcessProtectMemory;
#define MIRA_PROCESS_PROTECT_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessProtectMemory), sizeof(ProcessProtectMemory))

typedef struct _ProcessList
{
    // Number of process id's
    uint32_t ProcessCount;

    // Process id list
    int32_t ProcessIds[0];
} ProcessList;
#define MIRA_PROCESS_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessList), sizeof(ProcessList))

typedef struct _ProcessInfo
{
    // Process id
    int32_t ProcessId;

    // Name of process
    char Name[32];

    // Elf path of process
    char ElfPath[1024];

    // Randomized path
    char RandomizedPath[256];

    // Thread count
    uint32_t ThreadCount;

    // Thread ids
    int32_t ThreadIds[0];
} ProcessInfo;
#define MIRA_PROCESS_INFO _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessInfo), sizeof(ProcessInfo))

typedef struct _ProcessReadPrison
{
    // Currently set process prison
    void* Prison;
} ProcessReadPrison;
#define MIRA_PROCESS_READ_PRISON _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessReadPrison), sizeof(ProcessReadPrison))

typedef struct _ProcessWritePrison
{
    // Prison to set
    void* Prison;
} ProcessWritePrison;
#define MIRA_PROCESS_WRITE_PRISON _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessWritePrison), sizeof(ProcessWritePrison))

typedef struct _ProcessThreadInfo
{
    // TODO: Implement
} ProcessThreadInfo;
#define MIRA_PROCESS_THREAD_INFO _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessThreadInfo), sizeof(ProcessThreadInfo))

typedef struct _ProcessThreadReadCredentials
{
    // TODO: Implement
} ProcessThreadReadCredentials;
#define MIRA_PROCESS_THREAD_READ_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessThreadReadCredentials), sizeof(ProcessThreadReadCredentials))

typedef struct _ProcessThreadWriteCredentials
{
    // TODO: Implement
} ProcessThreadWriteCredentials;
#define MIRA_PROCESS_THREAD_WRITE_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessThreadWriteCredentials), sizeof(ProcessThreadWriteCredentals))

typedef struct _ProcessThreadReadPrivilegeMask
{
    // TODO: Implement
} ProcessThreadReadPrivilegeMask;
#define MIRA_PROCESS_THREAD_READ_PRIVILEGE_MASK _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessThreadReadPrivilegeMask), sizeof(ProcessThreadReadPrivilegeMask))

typedef struct _ProcessThreadWritePrivilegeMask
{

} ProcessThreadWritePrivilegeMask;
#define MIRA_PROCESS_THREAD_WRITE_PRIVILEGE_MASK _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessThreadWritePrivilegeMask), sizeof(ProcessThreadWritePrivilegeMask))

typedef struct _ProcessFindImportAddress
{
    // Library name
    char ModuleName[_MAX_PATH];

    // Function name or nid
    char FunctionName[_MAX_PATH];

    // Is the FunctionName field a NID?
    bool IsNID;

    // Returned address on success
    void* ImportAddress;
} ProcessFindImportAddress;
#define MIRA_PROCESS_FIND_IMPORT_ADDRESS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessFindImportAddress), sizeof(ProcessFindImportAddress))

typedef struct _ProcessModuleList
{
    // TODO: Implement
} ProcessModuleList;
#define MIRA_PROCESS_MODULE_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessModuleList), sizeof(ProcessModuleList))

typedef struct _ProcessModuleLoad
{
    // TODO: Implement
} ProcessModuleLoad;
#define MIRA_PROCESS_MODULE_LOAD _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessModuleLoad), sizeof(ProcessModuleLoad))

typedef struct _ProcessModuleInject
{
    // TODO: Implement
} ProcessModuleInject;
#define MIRA_PROCESS_MODULE_INJECT _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_ProcessModuleInject), sizeof(ProcessModuleInject))
