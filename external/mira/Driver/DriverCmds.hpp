#pragma once

/*
   97 
   98  prison0 describes what is "real" about the system. 
   99 struct prison prison0 = {
  100         .pr_id          = 0,
  101         .pr_name        = "",
  102         .pr_ref         = 1,
  103         .pr_uref        = 1,
  104         .pr_path        = "/",
  105         .pr_securelevel = -1,
  106         .pr_childmax    = JAIL_MAX,
  107         .pr_hostuuid    = DEFAULT_HOSTUUID,
  108         .pr_children    = LIST_HEAD_INITIALIZER(prison0.pr_children),
  109 #ifdef VIMAGE
  110         .pr_flags       = PR_HOST|PR_VNET|_PR_IP_SADDRSEL,
  111 #else
  112         .pr_flags       = PR_HOST|_PR_IP_SADDRSEL,
  113 #endif
  114         .pr_allow       = PR_ALLOW_ALL,
  115 };
  116 MTX_SYSINIT(prison0, &prison0.pr_mtx, "jail mutex", MTX_DEF);
*/

/*
# bare minimum.
- alloc/read/write/protect own process  -> (read/write doesn't have to be in kernel)
-- ability to see mem map of own proc
- miraVersion
- mounting stuff
- jailbreak stuff

# extra (you don't strictly need this but would eventually make it easier.)
- request pfi of prx to hook imports without hardcoding offsets. 
-- mira maintained lib to interpret pfi.

# don't really need this probably? (does anything use this?)
- alloc/read/write/protect other processes
-- ability to see all procs ^
-- ability to see memory map of other procs ^*/

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
    CMD_GENERAL_END,
    
    // 21-40 = Trainers
    CMD_TRAINERS_START,

    // TODO: Implement, Trainer create shared memory
    CMD_TrainerCreateShm,

    // TODO: Implement, Trainer get shared memory
    CMD_TrainerGetShm,

    // Requests for the trainer sandbox directories to be mounted and to load trainers
    CMD_TrainerLoad,

    // Gets the original entry point by process id    
    CMD_TrainerGetEntryPoint,
    
    CMD_TRAINERS_END,

    // 41-50 Processes

    // Allocate memory by process id
    CMD_ProcessAllocateMemory,

    // Free memory by process id
    CMD_ProcessFreeMemory,

    // Read memory by process id
    CMD_ProcessReadMemory,

    // Write memory by process id
    CMD_ProcessWriteMemory,

    // Protect memory by process id
    CMD_ProcessProtectMemory,

    // List all process ids on system
    CMD_ProcessList,

    // Get detailed process information
    CMD_ProcessInfo,

    // Get process prison
    CMD_ProcessReadPrison,

    // Set process prison
    CMD_ProcessWritePrison,

    // Get detailed thread information
    CMD_ProcessThreadInfo,

    // Get thread process credentials
    CMD_ProcessThreadReadCredentials,

    // Set thread process credentials
    CMD_ProcessThreadWriteCredentials,

    // TODO: Implement
    CMD_ProcessThreadReadPrivilegeMask,

    // TODO: Implement
    CMD_ProcessThreadWritePrivilegeMask,

    // Dynamic Libraries

    // Find the offset of a function on a I(mport) A(ddress) T(able)
    CMD_ProcessFindImportAddress,

    // List all dynamically loaded modules
    CMD_ProcessModuleList,

    // Load a module from within the sandbox
    CMD_ProcessModuleLoad,

    // Inject a new posix thread to load a module within the sandbox
    CMD_ProcessModuleInject,

    // 51-60 = Debugger
    CMD_DEBUGGER_START,

    // Jails/Prisons
    CMD_PrisonList,
    CMD_PrisonInfo,

    //CMD_GetProcessThreadList,       // Gets the process thread list

    // Credentials
    //CMD_GetThreadCredentials,       // Get thread credentials
    //CMD_SetThreadCredentials,       // Set thread credentials

    /*// Registers
    CMD_GetThreadRegisters,         // Gets the process thread registers
    CMD_SetThreadRegisters,         // Sets the process thread registers

    // Breakpoints
    CMD_GetBreakpointList,          // Get all breakpoints (software, hardware)
    CMD_GetBreakpoint,              // Get breakpoint information
    CMD_SetBreakpoint,              // Set breakpoint information*/

    // Process
    //CMD_FindJmpslot,                

    CMD_DEBUGGER_END,

    // 61-90 = Mira Reserved
    CMD_MIRA_START,
    CMD_MiraReadConfig,                  // Get mira's kernel configuration
    CMD_MiraWriteConfig,                  // Set mira's kernel configuration

    CMD_MiraMountInSandbox,             // Mount full unescaped sandbox path into the current sandbox
    

    CMD_MIRA_END,
    CMD_MAX
#if defined(__cplusplus)
} MiraIoctlCmds;
#else
};
#endif
