#pragma once
#include <Utils/Types.hpp>
#include <sys/conf.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>

#include <Plugins/Substitute/Substitute.hpp>

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
    typedef enum class _MiraGetThreadCredentialsPrison : uint32_t
    {
        // Non-root prison
        Default,

        // Switch prison to root vnode
        Root,

        // Total options count
        COUNT
    } MiraGetThreadCredentialsPrison;

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
    MiraGetThreadCredentialsPrison Prison;
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
    char Path[_MAX_PATH];
} MiraMountInSandbox;

#define MIRA_IOCTL_BASE 'M'

#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 1, sizeof(MiraThreadCredentials))

#define MIRA_GET_PID_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 2, sizeof(MiraProcessList))

#define MIRA_GET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 3, sizeof(MiraProcessInformation))

#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, 4, sizeof(MiraMountInSandbox))

namespace Mira
{
    namespace Driver
    {
        class CtrlDriver
        {
        private:
            eventhandler_entry* m_processStartHandler;
            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

        public:
            CtrlDriver();
            ~CtrlDriver();

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        
        protected:
            static void OnProcessStart(void *arg, struct proc *p);

            // Callback functions
            static int32_t OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetProcList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            // Helper functions
            static bool GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result);
            static bool GetProcessList(MiraProcessList*& p_List);
        };
    }
}