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
    // Based on the process type we access one of the variables below
    MiraProcessInformationType Type;
    union
    {
        // Name of the process
        char Name[_MAX_PATH];

        // Address of the process
        uint64_t Address;

        // Process ID
        uint32_t ProcessId;
    };
} MiraProcessInformation;

typedef struct _MiraProcessInformationResult
{
    typedef struct _MiraProcessInformationThreadResult
    {

    } MiraGetProcessInformationThreadResult;

    uint32_t ProcessId;
    uint32_t OpPid;
    uint32_t DebugChild;
    uint32_t ExitThreads;
    uint32_t SigParent;
    uint32_t Signal;
    uint32_t Code;
    uint32_t Stops;
    uint32_t SType;
    char Name[32];
    char ElfPath[1024];
    char RandomizedPath[256];

} MiraProcessInformationResult;

#define MIRA_IOCTL_BASE 'M'

#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 1, sizeof(MiraThreadCredentials))
#define MIRA_SET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 2, sizeof(MiraThreadCredentials))

#define MIRA_GET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 3, sizeof(MiraProcessInformation))
#define MIRA_SET_PROC_INFORMATION _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 4, sizeof(MiraProcessInformation))


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
        };
    }
}