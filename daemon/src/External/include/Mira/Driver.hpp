#pragma once
#include <cstdint>

extern "C" 
{
    #define COMM_LEN 19 + 13
    #define NAME_LEN 19 + 17
    #define _MAX_PATH 1024
    
    typedef enum SceAuthenticationId_t : uint64_t
    {
        SceVdecProxy = 0x3800000000000003ULL,
        SceVencProxy = 0x3800000000000004ULL,
        Orbis_audiod = 0x3800000000000005ULL,
        Coredump = 0x3800000000000006ULL,
        SceSysCore = 0x3800000000000007ULL,
        Orbis_setip = 0x3800000000000008ULL,
        GnmCompositor = 0x3800000000000009ULL,
        SceShellUI = 0x380000000000000fULL, // NPXS20001
        SceShellCore = 0x3800000000000010ULL,
        NPXS20103 = 0x3800000000000011ULL,
        NPXS21000 = 0x3800000000000012ULL,
        // TODO: Fill in the rest
        Decid = 0x3800000000010003,
    } SceAuthenticationId;

    typedef enum SceCapabilities_t : uint64_t
    {
        Max = 0xFFFFFFFFFFFFFFFFULL,
    } SceCapabilites;

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
        char Path[_MAX_PATH];
    } MiraMountInSandbox;

};