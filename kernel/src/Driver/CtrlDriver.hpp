#pragma once
#include <Utils/Types.hpp>
#include <sys/conf.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>

#include <Plugins/Substitute/Substitute.hpp>
#include <mira/MiraConfig.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>
    #include <sys/proc.h>
    #include <sys/ioccom.h>
};

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
    int64_t ProcessId;
    void* EntryPoint;
    struct _MiraTrainerProcessInfo* Previous;
    struct _MiraTrainerProcessInfo* Next;
} MiraTrainerProcessInfo;


#define MIRA_IOCTL_BASE 'M'

// List of commands, this way everything keeps the same ID's
// NOTE: DO NOT REMOVE/ADD ANYTHING FROM THIS LIST
// ADDING NEW ENTRIES SHOULD ONLY BE DONE BEFORE THE MAX ENTRY, DO NOT RE-ORDER THIS SHIT OR ILL SLAP YOU IRL
typedef enum class _MiraIoctlCmds : uint32_t
{
    None = 0,
    ThreadCredentials,
    ProcessList,
    ProcessInformation,
    MountInSandbox,
    CreateTrainerShm,
    GetTrainerShm,
    LoadTrainers,
    ReadProcessMemory,
    WriteProcessMemory,
    GetConfig,
    SetConfig,
    GetOriginalEntryPoint,
    MAX
} MiraIoctlCmds;

// Get/set the thread credentials
#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::ThreadCredentials), sizeof(MiraThreadCredentials))

// Get a process id list
#define MIRA_GET_PID_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::ProcessList), sizeof(MiraProcessList))

// Get process information
#define MIRA_GET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::ProcessInformation), sizeof(MiraProcessInformation))

// Mount a path within sandbox
#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::MountInSandbox), sizeof(MiraMountInSandbox))

// Create new Shm
#define MIRA_TRAINERS_CREATE_SHM _IOC(IOC_IN, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::CreateTrainerShm), sizeof(MiraCreateTrainerShm))

// Get the currently loaded shm's
#define MIRA_TRAINERS_GET_SHM _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::GetTrainerShm), sizeof(MiraGetTrainersShm))

// Trainers
#define MIRA_TRAINERS_LOAD _IOC(IOC_IN, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::LoadTrainers), 0)
#define MIRA_TRAINERS_ORIG_EP _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::GetOriginalEntryPoint), 0)

// Read/Write process memory
#define MIRA_READ_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::ReadProcessMemory), sizeof(MiraReadProcessMemory))
#define MIRA_WRITE_PROCESS_MEMORY _IOC(IOC_INOUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::WriteProcessMemory), sizeof(MiraWriteProcessMemory))

// Configuration
#define MIRA_GET_CONFIG _IOC(IOC_IN, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::GetConfig), 0)
#define MIRA_SET_CONFIG _IOC(IOC_OUT, MIRA_IOCTL_BASE, static_cast<uint32_t>(MiraIoctlCmds::SetConfig), sizeof(MiraConfig))

namespace Mira
{
    namespace Driver
    {
        class CtrlDriver
        {
        private:
            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

            struct mtx m_Mutex;
            MiraTrainerProcessInfo* m_Head;

        public:
            CtrlDriver();
            ~CtrlDriver();

            void AddOrUpdateEntryPoint(int32_t p_ProcessId, void* p_EntryPoint);
            void RemoveEntryPoint(int32_t p_ProcessId);
            void* GetEntryPoint(int32_t p_ProcessId);

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        
            static void OnProcessExec(void*, struct proc *p);
        protected:
            // Callback functions
            static int32_t OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetProcList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraThreadCredentials(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraSetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            // Helper functions
            static bool GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result);
            static bool GetProcessList(MiraProcessList*& p_List);

            static bool GetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials*& p_Output);
            static bool SetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials& p_Input);
        };
    }
}