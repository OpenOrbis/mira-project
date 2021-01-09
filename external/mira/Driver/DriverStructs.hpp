#pragma once
extern "C"
{
    #include <sys/types.h>
    #include <sys/param.h>
    #include <sys/proc.h>
};

#include <Utils/Kernel.hpp>

#if !defined(_MAX_PATH)
#define _MAX_PATH 260
#endif

// Process Handler Information
// "safe" way in order to modify kernel ucred externally
typedef struct _MiraThreadCredentials {
    typedef enum class _MiraThreadCredentialsPrison : uint32_t
    {
        // Non-root prison
        Default,

        // Switch prison to root vnode
        Root,

        // Total options count
        COUNT
    } MiraThreadCredentialsPrison;

    typedef enum class _State : uint32_t
    {
        Get,
        Set,
        COUNT
    } GSState;

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
typedef struct _MiraLoadPlugin 
{
    // The plugin type to load
    MiraLoadPluginType PluginType;

    // The path on filesystem of the plugin
    char PluginPath[_MAX_PATH];
} MiraLoadPlugin;

typedef struct _MiraUnloadPlugin 
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

typedef struct _MiraProcessInformation
{
    typedef struct _ThreadResult
    {
        int32_t ThreadId;
        int32_t ErrNo;
        int64_t RetVal;
        char Name[sizeof(((struct thread*)0)->td_name)];
    } ThreadResult;

    // Structure size
    uint32_t Size;
    int32_t ProcessId;
    int32_t OpPid;
    int32_t DebugChild;
    int32_t ExitThreads;
    int32_t SigParent;
    int32_t Signal;
    uint32_t Code;
    uint32_t Stops;
    uint32_t SType;
    char Name[sizeof(((struct proc*)0)->p_comm)];
    char ElfPath[sizeof(((struct proc*)0)->p_elfpath)];
    char RandomizedPath[sizeof(((struct proc*)0)->p_randomized_path)];
    ThreadResult Threads[];
} MiraProcessInformation;

typedef struct _MiraProcessList
{
    // Structure size
    uint32_t Size;

    // Pid array
    int32_t Pids[];
} MiraProcessList;

typedef struct _MiraMountInSandbox
{
    int32_t Permissions;
    char HostPath[_MAX_PATH];
    char SandboxPath[_MAX_PATH];
} MiraMountInSandbox;

typedef struct _MiraCreateTrainerShm
{
    struct thread* Thread;
    void* TrainerHeader;
    uint64_t TrainerHeaderMapSize; // 1MB default
} MiraCreateTrainerShm;

typedef struct _MiraTrainerShm
{
    // shm name
    char ShmName[16];
} MiraTrainerShm;

typedef struct _MiraGetTrainersShm
{
    // Size of this structure including Shms[]
    uint32_t StructureSize;

    // Output ShmCount
    uint32_t ShmCount;

    // Array of shm names
    MiraTrainerShm Shms[];
} MiraGetTrainersShm;

typedef struct _MiraReadProcessMemory
{
    // Size of the structure
    uint32_t StructureSize;

    // -1 for calling process
    int32_t ProcessId;

    // Address to read from in process
    uint64_t Address;

    // Size of data to read from
    uint64_t Size;

    // Array of returned data
    uint8_t Data[];
} MiraReadProcessMemory;

typedef struct _MiraWriteProcessMemory
{
    // Size of the structure
    uint32_t StructureSize;

    // -1 for calling process
    int32_t ProcessId;

    // Address to write to in process
    uint64_t Address;

    // Size of the data to write
    uint64_t Size;

    // Data to write
    uint8_t Data[];
} MiraWriteProcessMemory;

typedef struct _MiraTrainerProcessInfo
{
    // this is 64 bit for alignment
    pid_t ProcessId;
    void* EntryPoint;
} MiraTrainerProcessInfo;