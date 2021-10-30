#pragma once

#ifdef _KERNEL
#include <mira/Kernel/Utils/Kernel.hpp>

extern "C"
{
    #include <sys/types.h>
    #include <sys/param.h>
    #include <sys/proc.h>
};
#else

typedef uint64_t SceAuthenticationId;
typedef uint64_t SceCapabilites;
#endif

#if !defined(_MAX_PATH)
#define _MAX_PATH 260
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

typedef enum class _State : uint32_t
{
    Get,
    Set,
    COUNT
} GSState;

// Process Handler Information
// "safe" way in order to modify kernel ucred externally
typedef struct __attribute__((packed)) _MiraThreadCredentials {
    typedef enum class _MiraThreadCredentialsPrison : uint32_t
    {
        // Non-root prison
        Default,

        // Switch prison to root vnode
        Root,

        // Total options count
        COUNT
    } MiraThreadCredentialsPrison;

    // Is this a get or set operation
    GSState State;

    // Process ID to modify
    int32_t ProcessId;

    // Threaad ID to modify, or -1 for (all threads in process, USE WITH CAUTION)
    int32_t ThreadId;

    // Everything below is Get/Set
    int32_t EffectiveUserId;
    int32_t RealUserId;
    int32_t SavedUserId;
    int32_t NumGroups;
    int32_t RealGroupId;
    int32_t SavedGroupId;
    MiraThreadCredentialsPrison Prison;
    SceAuthenticationId SceAuthId;
    SceCapabilites Capabilities[4];
    uint64_t Attributes[4];
} MiraThreadCredentials;

typedef enum class _MiraLoadPluginType : uint8_t
{
    // Don't try and load any plugin type
    None,

    // Load a new kernel plugin
    Kernel,

    // Load a new userland daemon plugin
    Daemon,

    // Load a new game/process trainer
    Trainer,

    // Total entry count
    COUNT
} MiraLoadPluginType;

// This is the requesting structure provided to the ioctl, on return a u64 as ID is returned
typedef struct __attribute__((packed)) _MiraLoadPlugin 
{
    // The plugin type to load
    MiraLoadPluginType PluginType;

    // The path on filesystem of the plugin
    char PluginPath[_MAX_PATH];
} MiraLoadPlugin;

typedef struct __attribute__((packed)) _MiraUnloadPlugin 
{
    // The returned plugin id from the plugin manager
    uint64_t PluginId;
} MiraUnloadPlugin;

typedef enum class _MiraProcessInformationType : uint8_t
{
    // Do nothing
    None,

    // Get process information if you know the pid
    ProcessId,

    // Get process information by name (case sensitive)
    Name,

    // Get process information by kernel address
    Address,

    // Total option count
    COUNT
} MiraProcessInformationType;

typedef struct __attribute__((packed)) _MiraProcessInformation
{
    typedef struct __attribute__((packed)) _ThreadResult
    {
        int32_t ThreadId;
        int32_t ErrNo;
        int64_t RetVal;
        char Name[36];
    } ThreadResult;

    // Structure size
    uint32_t StructureSize;
    int32_t ProcessId;
    int32_t OpPid;
    int32_t DebugChild;
    int32_t ExitThreads;
    int32_t SigParent;
    int32_t Signal;
    uint32_t Code;
    uint32_t Stops;
    uint32_t SType;
    char Name[32];
    char ElfPath[1024];
    char RandomizedPath[256];
    uint64_t ThreadCount;
    ThreadResult Threads[];
} MiraProcessInformation;

typedef struct __attribute__((packed)) _MiraProcessList
{
    // Structure size
    uint32_t StructureSize;

    // Pid array
    int32_t Pids[];
} MiraProcessList;

typedef struct __attribute__((packed)) _MiraMountInSandbox
{
    int32_t Permissions;
    char HostPath[_MAX_PATH];
    char SandboxPath[_MAX_PATH];
} MiraMountInSandbox;

typedef struct __attribute__((packed)) _MiraCreateTrainerShm
{
    struct thread* Thread;
    void* TrainerHeader;
    uint64_t TrainerHeaderMapSize; // 1MB default
} MiraCreateTrainerShm;

typedef struct __attribute__((packed)) _MiraTrainerShm
{
    // shm name
    char ShmName[16];
} MiraTrainerShm;

typedef struct __attribute__((packed)) _MiraGetTrainersShm
{
    // Size of this structure including Shms[]
    uint32_t StructureSize;

    // Output ShmCount
    uint32_t ShmCount;

    // Array of shm names
    MiraTrainerShm Shms[];
} MiraGetTrainersShm;

typedef struct __attribute__((packed)) _MiraReadProcessMemory
{
    // Size of the structure
    uint32_t StructureSize;

    // -1 for calling process
    int32_t ProcessId;

    // Address to read from in process
    void* Address;

    uint8_t Data[];
} MiraReadProcessMemory;

typedef struct __attribute__((packed)) _MiraWriteProcessMemory
{
    // Size of the structure
    uint32_t StructureSize;

    // -1 for calling process
    int32_t ProcessId;

    // Address to write to in process
    void* Address;

    // Data to write
    uint8_t Data[];
} MiraWriteProcessMemory;

typedef struct __attribute__((packed)) _MiraPrivCheck
{
    // Thread id
    int32_t ThreadId;

    // Get or set the mask?
    uint32_t IsGet;

    // Override mask
    uint8_t Mask[1024]; // This must match MaskSizeInBytes in PrivCheckPlugin.hpp

    inline bool SetBit(uint32_t p_Index, bool p_Override)
    {
        if (p_Index >= ARRAYSIZE(Mask))
            return false;
        
        // eh?
        
        Mask[p_Index] = p_Override;

        return true;
    }
    inline bool GetBit(uint32_t p_Index, bool& p_Overridden)
    {
        if (p_Index >= ARRAYSIZE(Mask))
            return false;
        
        p_Overridden = Mask[p_Index] != 0;
        return true;
    }
    
} MiraSetThreadPrivMask;

typedef struct _MiraAllocateMemory
{
    // Process id to allocate memory in (<= 0 for current process)
    int32_t ProcessId;

    // Allocation size
    uint32_t Size;

    // Protection to set (RWX allowed)
    int32_t Protection;

    // Output pointer (should be set to nullptr on request, will be filled or nullptr on return)
    uint8_t* Pointer;
} MiraAllocateMemory;

typedef struct _MiraFreeMemory
{
    int32_t ProcessId;

    uint32_t Size;

    void* Pointer;
} MiraFreeMemory;