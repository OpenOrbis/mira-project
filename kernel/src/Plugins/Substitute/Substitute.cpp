// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Substitute.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Hook.hpp>

#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

extern "C"
{
    #include <fcntl.h>
    #include <sys/dirent.h>
    #include <sys/stat.h>
    #include <sys/sx.h>
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
    #include <sys/unistd.h>
    #include <sys/uio.h>
    #include <sys/proc.h>
    #include <sys/errno.h>
    #include <vm/vm.h>
    #include <vm/pmap.h>
    #include <sys/kthread.h>
    #include <sys/filedesc.h>
    #include <unistd.h>
    #include <sys/fcntl.h>
    #include <sys/mman.h>
    #include <sys/mount.h>
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
    #include <sys/syscall.h>
    #include <sys/syslimits.h>
    #include <sys/param.h>
    #include <sys/dynlib.h>
};

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

//////////////////////////
// MIRA BASE PLUGIN
//////////////////////////

// Substitute : Constructor
Substitute::Substitute()
{
    // Cleanup hooks memory space
    memset(m_Hooks, 0, sizeof(SubstituteHook) * SUBSTITUTE_MAX_HOOKS);
}

// Substitute : Destructor
Substitute::~Substitute()
{
}

// Substitute : Plugin loaded
bool Substitute::OnLoad()
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    WriteLog(LL_Info, "Loading Substitute ...");

    mtx_init(&m_Mutex, "Sbste", NULL, MTX_SPIN);

    // Print substitute ioctl command
    WriteLog(LL_Debug, "IOCTL Command:");
    WriteLog(LL_Debug, "SUBSTITUTE_HOOK_IAT: 0x%08x", SUBSTITUTE_HOOK_IAT);
    WriteLog(LL_Debug, "SUBSTITUTE_HOOK_JMP: 0x%08x", SUBSTITUTE_HOOK_JMP);
    WriteLog(LL_Debug, "SUBSTITUTE_HOOK_STATE: 0x%08x", SUBSTITUTE_HOOK_STATE);

    return true;
}

// Substitute : Plugin unloaded
bool Substitute::OnUnload()
{
    WriteLog(LL_Error, "Unloading Substitute ...");

    // Cleanup substitute hook (PRX Loader)
    CleanupAllHook();
    return true;
}

// Substitute : Plugin suspended
bool Substitute::OnSuspend()
{
    WriteLog(LL_Error, "Suspending Substitute ...");
    return true;
}

// Substitute : Plugin resumed
bool Substitute::OnResume()
{
    WriteLog(LL_Error, "Resuming Substitute ...");
    return true;
}

// Substitute : Get substitute pointer from static function
Substitute* Substitute::GetPlugin()
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return nullptr;
    
    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
        return nullptr;
    
    auto s_SubstitutePlugin = static_cast<Substitute*>(s_PluginManager->GetSubstitute());
    if (s_SubstitutePlugin == nullptr)
        return nullptr;
    
    return s_SubstitutePlugin;
}

//////////////////////////////////////////
// HOOK MEMORY SYSTEM (Kernel Side) //
//////////////////////////////////////////

// Substitute : Return hook struct by this id (MUTEX NEEDED)
SubstituteHook* Substitute::GetHookByID(int p_HookId) 
{
    // Check if the hook_id is in the specs
    if (p_HookId < 0 || p_HookId > SUBSTITUTE_MAX_HOOKS) 
    {
        WriteLog(LL_Error, "Invalid hook id ! (%d)", p_HookId);
        return nullptr;
    }

    // If the hook is empty, it's an invalid hook !
    SubstituteHook s_Empty;
    memset(&s_Empty, 0, sizeof(SubstituteHook));
    if (memcmp(&m_Hooks[p_HookId], &s_Empty, sizeof(SubstituteHook)) == 0) 
        return nullptr;

    // Return the address
    return &m_Hooks[p_HookId];
}

// Substitute : Allocate a new hook to the list (MUTEX NEEDED)
SubstituteHook* Substitute::AllocateHook(int* p_HookId) 
{
    if (!p_HookId) 
    {
        WriteLog(LL_Error, "Invalid argument !");
        return nullptr;
    }

    SubstituteHook s_Empty;
    memset(&s_Empty, 0, sizeof(s_Empty));

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) 
    {
        if (memcmp(&m_Hooks[i], &s_Empty, sizeof(s_Empty)) == 0) 
        {
            *p_HookId = i;
            return &m_Hooks[i];
        }
    }

    *p_HookId = -1;

    return nullptr;
}

// Substitute : Free a hook from the list (MUTEX NEEDED)
void Substitute::FreeHook(int p_HookId) 
{
    SubstituteHook* s_Hook = GetHookByID(p_HookId);

    if (!s_Hook)
        return;
    
    // Free the chain list
    if (s_Hook->hook_type == HOOKTYPE_IAT && s_Hook->iat.uap_chains)
        delete (s_Hook->iat.uap_chains);

    // Set to 0, now everything is available
    memset(s_Hook, 0, sizeof(SubstituteHook));

    WriteLog(LL_Info, "The hook %i have been deleted.", p_HookId);
}


//////////////////////////////////////////
// HOOK MANAGEMENT SYSTEM (Kernel Side) //
//////////////////////////////////////////

// Substitute : Find a hook with similar process and jmpslot (MUTEX NEEDED)
bool Substitute::FindJmpslotSimilarity(struct proc* p_Process, void* p_JmpSlotAddr, int* p_HookId) 
{
    if (!p_HookId) 
    {
        WriteLog(LL_Error, "Invalid argument !");
        return false;
    }

    SubstituteHook s_Empty;
    memset(&s_Empty, 0, sizeof(s_Empty));

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) 
    {
        // If empty, don't continue, just break instead of do all the list
        if (memcmp(&m_Hooks[i], &s_Empty, sizeof(s_Empty)) == 0)
            break;

        if (m_Hooks[i].hook_type != HOOKTYPE_IAT) 
            continue;
        
        if (m_Hooks[i].process == p_Process && m_Hooks[i].iat.jmpslot_address == p_JmpSlotAddr) 
        {
            *p_HookId = i;
            return true;
        }
    }

    *p_HookId = -1;

    return false;
}

// Substitute : Find position by chains address
int Substitute::FindPositionByChain(uint64_t* p_Chains, uint64_t p_Chain) 
{
    if (!p_Chains) 
    {
        WriteLog(LL_Error, "Invalid argument !");
        return -1;
    }

    for (int i = 0; i < SUBSTITUTE_MAX_CHAINS; i++) 
    {
        if (p_Chains[i] == p_Chain) 
            return i;
    }

    return -1;
}

// Substitute : Get UAP Address of last occurence of a chains (MUTEX NEEDED)
void* Substitute::FindLastChainOccurence(uint64_t* chains, int* position) 
{
    if (!chains || !position) 
    {
        WriteLog(LL_Error, "Invalid argument !");
        return nullptr;
    }

    for (int i = 0; i < SUBSTITUTE_MAX_CHAINS; i++) 
    {
        if (chains[i] == 0) 
        {

            if ( (i-1) < 0 )
                return nullptr;
            
            *position = (i-1);

            return (void*)chains[i-1];
        }
    }

    return nullptr;
}

// Substitute : Hook the function in the process (With Import Address Table)
int Substitute::HookIAT(struct proc* p_Process, struct substitute_hook_uat* p_Chain, const char* p_ModuleName, const char* p_Name, int32_t p_Flags, void* p_HookFunction) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p_Process || !p_Name || !p_HookFunction || !p_Chain) 
    {
        WriteLog(LL_Error, "One of the parameter is incorrect !");
        WriteLog(LL_Error, "p: %p", p_Process);
        WriteLog(LL_Error, "chain: %p", p_Chain);
        WriteLog(LL_Error, "name: %p", p_Name);
        WriteLog(LL_Error, "hook_function: %p", p_HookFunction);
        WriteLog(LL_Error, "One of the parameter is incorrect !");
        return SUBSTITUTE_BAD_ARGS;
    }

    // Get the jmpslot offset for this nids (Work like an id for detect if hook already present)
    void* s_JmpSlotAddress = (void*)FindJmpslotAddress(p_Process, p_ModuleName, p_Name, p_Flags);
    if (!s_JmpSlotAddress) 
    {
        WriteLog(LL_Error, "Unable to find the jmpslot address !");
        return SUBSTITUTE_INVALID;
    }

    int s_Error = 0;
    int s_HookId = -1;

    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    do
    {
        // Find if something is similar for this process
        WriteLog(LL_Info, "jmpslot addr: %p", s_JmpSlotAddress);
        if (FindJmpslotSimilarity(p_Process, s_JmpSlotAddress, &s_HookId)) 
        {
            // Similarity : Rebuild the hook chain for handle new hook of same function/process
            WriteLog(LL_Info, "Similarity : Rebuild the hook chain");

            SubstituteHook* s_Hook = GetHookByID(s_HookId);
            if (s_Hook == nullptr) 
            {
                WriteLog(LL_Error, "Unable to get the hook !");
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // Build the current chain
            struct substitute_hook_uat s_CurrentChain;
            s_CurrentChain.hook_id = s_HookId;
            s_CurrentChain.hook_function = p_HookFunction;
            s_CurrentChain.original_function = s_Hook->iat.original_function;
            s_CurrentChain.next = nullptr;

            // Find lasted chain uap position and address
            int s_LastChainPosition = -1;
            void* s_LastChainAddress = FindLastChainOccurence(s_Hook->iat.uap_chains, &s_LastChainPosition);
            if (!s_LastChainAddress || s_LastChainPosition < 0) 
            {
                WriteLog(LL_Error, "Unable to get the lasted chain address.");
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // Determinate if we have space for write
            if ( (s_LastChainPosition + 1) > SUBSTITUTE_MAX_CHAINS ) 
            {
                WriteLog(LL_Error, "SUBSTITUTE_MAX_CHAINS reached !");
                s_Error = SUBSTITUTE_NOMEM;
                break;
            }

            // Write current chain in the process
            s_Error = proc_rw_mem(p_Process, p_Chain, sizeof(struct substitute_hook_uat), &s_CurrentChain, nullptr, true);
            if (s_Error != 0) 
            {
                WriteLog(LL_Error, "Unable to write chains structure: (%i)", s_Error);                
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // Got the last chains struct into kernel
            struct substitute_hook_uat s_LastChain;
            s_Error = proc_rw_mem(p_Process, s_LastChainAddress, sizeof(struct substitute_hook_uat), &s_LastChain, nullptr, false);
            if (s_Error != 0) 
            {
                WriteLog(LL_Error, "Unable to read the last chain: (%d)", s_Error);                
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // The next chain of last chain is the current chain
            s_LastChain.next = p_Chain;

            // Write the edited last chain in the process
            s_Error = proc_rw_mem(p_Process, s_LastChainAddress, sizeof(struct substitute_hook_uat), &s_LastChain, nullptr, true);
            if (s_Error != 0) 
            {
                WriteLog(LL_Error, "Unable to write chains structure: (%i)", s_Error);
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // Write the new chain address into the chain structure
            s_Hook->iat.uap_chains[(s_LastChainPosition + 1)] = (uint64_t)p_Chain;
        } 
        else 
        {
            // No similarity : Build a new hook block
            WriteLog(LL_Info, "No similarity : Build a new hook block ...");

            // Get the original value for this jmpslot
            void* s_OriginalFunction = FindOriginalAddress(p_Process, p_Name, p_Flags);
            if (!s_OriginalFunction) 
            {
                WriteLog(LL_Error, "Unable to get the original value from the jmpslot !");
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            s_HookId = -1;
            SubstituteHook* s_NewHook = AllocateHook(&s_HookId);
            if (!s_NewHook || s_HookId < 0) 
            {
                WriteLog(LL_Error, "Unable to allocate new hook (%p => %d) !", s_NewHook, s_HookId);
                s_Error = SUBSTITUTE_NOMEM;
                break;
            }

            // Initialize chains
            uint64_t* s_Chains = new uint64_t[SUBSTITUTE_MAX_CHAINS];
            memset(s_Chains, 0, sizeof(uint64_t) * SUBSTITUTE_MAX_CHAINS);

            // Setup uat values
            struct substitute_hook_uat s_Setup;
            s_Setup.hook_id = s_HookId;
            s_Setup.hook_function = p_HookFunction;
            s_Setup.original_function = s_OriginalFunction;
            s_Setup.next = nullptr;

            // Write uat values into the process
            s_Error = proc_rw_mem(p_Process, p_Chain, sizeof(struct substitute_hook_uat), &s_Setup, nullptr, true);
            if (s_Error != 0) 
            {
                WriteLog(LL_Error, "Unable to write chains structure: (%i)", s_Error);                
                // Free memory
                delete [] s_Chains;
                s_Error = SUBSTITUTE_INVALID;
                break;
            }

            // Write the first chains address
            s_Chains[0] = (uint64_t)p_Chain;

            // Set default value
            s_NewHook->process = p_Process;
            s_NewHook->hook_type = HOOKTYPE_IAT;
            s_NewHook->iat.uap_chains = s_Chains;
            s_NewHook->iat.jmpslot_address = s_JmpSlotAddress;
            s_NewHook->iat.original_function = s_OriginalFunction;

            // Enable first hook !
            // Overwrite the jmpslot slot address
            s_Error = proc_rw_mem(p_Process, s_JmpSlotAddress, sizeof(void*), &p_HookFunction, nullptr, true);
            if (s_Error != 0) 
            {
                WriteLog(LL_Error, "Unable to write to the jmpslot address: (%d)", s_Error);                
                delete[] s_Chains;
                s_Error = SUBSTITUTE_INVALID;
                break;
            }
        }
    } while (false);
    
    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "New hook is created (IAT) :  %i", s_HookId);
    return s_HookId;
}

// Substitute : Hook the function in the process (With longjmp)
int Substitute::HookJmp(struct proc* p_Process, void* p_OriginalAddress, void* hook_function) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p_Process || !p_OriginalAddress || !hook_function)
        return SUBSTITUTE_BAD_ARGS;    

    // Get buffer from original function for calculate size
    char s_Buffer[64];
    size_t s_ReadSize = 0;
    int s_Error = proc_rw_mem(p_Process, (void*)p_OriginalAddress, sizeof(s_Buffer), (void*)s_Buffer, &s_ReadSize, 0);
    if (s_Error) 
    {
        WriteLog(LL_Error, "Unable to get the buffer !");
        return SUBSTITUTE_INVALID;
    }

    // Set the original address
    int s_BackupSize = Utils::Hook::GetMinimumHookSize(s_Buffer);
    if (s_BackupSize <= 0) 
    {
        WriteLog(LL_Error, "Unable to get the minimum hook size.");
        return SUBSTITUTE_INVALID;
    }

    // Malloc data for the backup data
    char* s_BackupData = new char[s_BackupSize];
    if (!s_BackupData) 
    {
        WriteLog(LL_Error, "Unable to allocate memory for backup");
        return SUBSTITUTE_NOMEM;
    }

    // Get the backup data
    s_ReadSize = 0;
    s_Error = proc_rw_mem(p_Process, (void*)p_OriginalAddress, s_BackupSize, (void*)s_BackupData, &s_ReadSize, 0);
    if (s_Error) 
    {
        WriteLog(LL_Info, "Unable to get backupData (%d)", s_Error);
        delete[] (s_BackupData);
        return SUBSTITUTE_INVALID;
    }

    int s_HookId = -7;
    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    do
    {
        
        SubstituteHook* s_NewHook = AllocateHook(&s_HookId);
        if (!s_NewHook || s_HookId < 0) 
        {
            WriteLog(LL_Error, "Unable to allocate new hook (%p => %d) !", s_NewHook, s_HookId);
            s_Error = SUBSTITUTE_NOMEM;
            break;
        }

        // Set default value
        s_NewHook->process = p_Process;
        s_NewHook->hook_type = HOOKTYPE_JMP;
        s_NewHook->jmp.jmpto = hook_function;
        s_NewHook->jmp.orig_addr = p_OriginalAddress;
        s_NewHook->jmp.backupData = s_BackupData;
        s_NewHook->jmp.backupSize = s_BackupSize;
        s_NewHook->jmp.enable = false;

    } while (false);

    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "New hook is created (JMP) :  %i", s_HookId);
    return s_HookId;
}

// Substitute : Disable the hook
int Substitute::DisableHook(struct proc* p_Process, int p_HookId) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int32_t s_Error = SUBSTITUTE_OK;

    // Lock the process
    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    do
    {
        SubstituteHook* s_Hook = GetHookByID(p_HookId);
        if (!s_Hook) 
        {
            WriteLog(LL_Error, "The hook %i is not found !.", p_HookId);            
            s_Error = SUBSTITUTE_NOTFOUND;
            break;
        }

        if (s_Hook->process != p_Process) 
        {
            WriteLog(LL_Error, "Invalid handle : Another process trying to disable hook.");
            s_Error = SUBSTITUTE_INVALID;
            break;
        }

        switch (s_Hook->hook_type) 
        {
            case HOOKTYPE_IAT: 
            {
                WriteLog(LL_Error, "!!! WARNING !!! Not compatible with IAT Hook.");
                break;
            }

            case HOOKTYPE_JMP : 
            {
                if (s_Hook->process && s_Hook->jmp.enable && s_Hook->jmp.orig_addr && s_Hook->jmp.backupSize > 0 && s_Hook->jmp.backupData) 
                {
                    s_Error = proc_rw_mem(s_Hook->process, s_Hook->jmp.orig_addr, s_Hook->jmp.backupSize, s_Hook->jmp.backupData, nullptr, true);
                    if (s_Error != 0) 
                    {
                        WriteLog(LL_Error, "Unable to write original address: (%i)", s_Error);
                        s_Error = SUBSTITUTE_BADLOGIC;
                        break;
                    }

                    s_Hook->jmp.enable = false;
                } 
                else 
                {
                    WriteLog(LL_Error, "Invalid value was detected.");
                }

                break;
            }

            default: 
            {
                WriteLog(LL_Error, "Invalid type of hook was detected.");                
                s_Error = SUBSTITUTE_INVALID;
                break;
            }
        }

    } while (false);
    
    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);
    

    return s_Error;
}

// Substitute : Enable the hook
int Substitute::EnableHook(struct proc* p_Process, int p_HookId) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
  
    int s_Error = SUBSTITUTE_OK;

    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    do
    {
        SubstituteHook* s_Hook = GetHookByID(p_HookId);
        if (!s_Hook) 
        {
            s_Error = -1;
            break;
        }

        if (s_Hook->process != p_Process) 
        {
            WriteLog(LL_Error, "Invalid handle : Another process trying to enable hook.");
            s_Error = -2;
            break;
        }

        switch (s_Hook->hook_type) 
        {
            case HOOKTYPE_IAT: 
            {
                WriteLog(LL_Error, "!!! WARNING !!! Not compatible with IAT Hook.");
                break;
            }

            case HOOKTYPE_JMP : 
            {
                if (s_Hook->process && !s_Hook->jmp.enable && s_Hook->jmp.orig_addr && s_Hook->jmp.jmpto) 
                {

                    // Use the jmpBuffer from Hook.cpp
                    uint8_t s_JumpBuffer[] = {
                        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,             // # jmp    QWORD PTR [rip+0x0]
                        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, // # DQ: AbsoluteAddress
                    }; // Shit takes 14 bytes

                    uint64_t* s_JumpBufferAddress = (uint64_t*)(s_JumpBuffer + 6);

                    // Assign the address
                    *s_JumpBufferAddress = (uint64_t)s_Hook->jmp.jmpto;

                    s_Error = proc_rw_mem(s_Hook->process, s_Hook->jmp.orig_addr, sizeof(s_JumpBuffer), s_JumpBuffer, nullptr, true);
                    if (s_Error != 0) 
                    {
                        WriteLog(LL_Error, "Unable to write the jmp system: (%i)", s_Error);
                        s_Error = -5;
                        break;
                    }

                    s_Hook->jmp.enable = true;
                } 
                else 
                    WriteLog(LL_Error, "Invalid value was detected.");
                
                break;
            }

            default: 
            {
                WriteLog(LL_Error, "Invalid type of hook was detected.");
                s_Error = -6;
                break;
            }
        }
    } while (false);

    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    return s_Error;
}

// Substitute : Unhook the function
int Substitute::Unhook(struct proc* p_Process, int p_HookId, struct substitute_hook_uat* p_Chain) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int s_Error = SUBSTITUTE_OK;

    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    do
    {
        SubstituteHook* s_Hook = GetHookByID(p_HookId);
        if (!s_Hook) 
        {
            WriteLog(LL_Error, "The hook %i is not found !.", p_HookId);
            s_Error = -1;
            break;
        }

        if (s_Hook->process != p_Process) 
        {
            WriteLog(LL_Error, "Invalid handle : Another process trying to disable hook.");
            s_Error = -2;
            break;
        }

        switch (s_Hook->hook_type) 
        {
            case HOOKTYPE_IAT: 
            {
                // Need to fix the chains

                // Got the current chain
                struct substitute_hook_uat s_CurrentChain;
                s_Error = proc_rw_mem(p_Process, p_Chain, sizeof(struct substitute_hook_uat), &s_CurrentChain, nullptr, false);
                if (s_Error != 0) 
                {
                    WriteLog(LL_Error, "Unable to read the current chain: (%d)", s_Error);
                    s_Error = -3;
                    break;
                }

                // I get the current position on the chain
                int s_ChainPosition = FindPositionByChain(s_Hook->iat.uap_chains, (uint64_t)p_Chain);
                if (s_ChainPosition < 0) 
                {
                    WriteLog(LL_Error, "Unable to get the current chain position: (%d)", s_ChainPosition);
                    s_Error = -4;
                    break; 
                }

                // Before need to have the address of after
                if (s_ChainPosition > 0) 
                {
                    // If it's something inside, just need to do before->next = after
                    struct substitute_hook_uat s_BeforeChain;
                    void* s_BeforeChainAddress = (void*)s_Hook->iat.uap_chains[s_ChainPosition-1];

                    // Read the before chain
                    s_Error = proc_rw_mem(p_Process, s_BeforeChainAddress, sizeof(struct substitute_hook_uat), &s_BeforeChain, nullptr, false);
                    if (s_Error != 0) 
                    {
                        WriteLog(LL_Error, "Unable to read the before chain: (%d)", s_Error);
                        s_Error = -5;
                    }

                    uint64_t s_NextAddress = 0;
                    if ( (s_ChainPosition+1) < SUBSTITUTE_MAX_CHAINS )
                        s_NextAddress = s_Hook->iat.uap_chains[s_ChainPosition+1];

                    s_BeforeChain.next = (struct substitute_hook_uat*)s_NextAddress;
                } 
                else if (s_ChainPosition == 0) 
                {
                    // If it's 0, all the chain is okey, just need to edit the jmpslot

                    // Get the next function chain
                    struct substitute_hook_uat s_Next;
                    void* s_NextAddress = (void*)s_Hook->iat.uap_chains[1];

                    // Read the next chain
                    s_Error = proc_rw_mem(p_Process, s_NextAddress, sizeof(struct substitute_hook_uat), &s_Next, nullptr, false);
                    if (s_Error != 0) 
                    {
                        WriteLog(LL_Error, "Unable to read the next chain: (%d)", s_Error);
                        s_Error = -7;
                        break;
                    }

                    // Edit the jmpslot address for the current hook
                    s_Error = proc_rw_mem(p_Process, s_Hook->iat.jmpslot_address, sizeof(void*), &s_Next.hook_function, nullptr, true);
                    if (s_Error != 0) 
                    {
                        WriteLog(LL_Error, "Unable to write to the jmpslot address: (%d)", s_Error);
                        s_Error = -8;
                        break;
                    }
                } 
                else 
                {
                    WriteLog(LL_Error, "Very very very weird issues is here");
                    s_Error = -9;
                    break;
                }

                // Update the chains table !
                for (int i = s_ChainPosition; i < SUBSTITUTE_MAX_CHAINS; i++) 
                {
                    if (s_Hook->iat.uap_chains[i] == 0)
                        break;

                    if ( (i+1) < SUBSTITUTE_MAX_CHAINS)
                        s_Hook->iat.uap_chains[i] = s_Hook->iat.uap_chains[i+1];
                }
            }

            case HOOKTYPE_JMP: 
            {
                // Simply free the space :)
                FreeHook(p_HookId);
            }
        }
    } while(false);

    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    if (s_Error == SUBSTITUTE_OK)
        WriteLog(LL_Info, "%i is now unhooked.", p_HookId);

    return s_Error;
}

// Substitute : Cleanup all hook
void Substitute::CleanupAllHook() 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    //WriteLog(LL_Info, "Cleaning up all hook ...");

    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++)
        FreeHook(i);

    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);
}

// Substitute : Cleanup hook for a process
void Substitute::CleanupProcessHook(struct proc* p_Process) 
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p_Process)
    {
        WriteLog(LL_Error, "invalid process.");
        return;
    }

    // TODO: Implement in the proc structure
    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);

    _mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) 
    {
        SubstituteHook* hook = GetHookByID(i);
        if (hook && hook->process == p_Process) 
        {
            WriteLog(LL_Info, "Cleaning up hook for %s", s_TitleId);
            FreeHook(i);
        }
    }

    _mtx_unlock_flags(&m_Mutex, 0, __FILE__, __LINE__);
}

//////////////////////////
// IAT HOOK UTILITY
//////////////////////////

// Substitute : Find original function address by this name (or nids)
void* Substitute::FindOriginalAddress(struct proc* p_Process, const char* p_Name, int32_t p_Flags)
{
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto dynlib_do_dlsym = (void*(*)(void* dl, void* obj, const char* name, const char* libname, unsigned int flags))kdlsym(dynlib_do_dlsym);

    if (p_Process == nullptr)
        return nullptr;

    // TODO: Fix this structure within proc
    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);
    void* s_Address = nullptr;

    WriteLog(LL_Info, "TitleId: (%s).", s_TitleId);

    if (p_Process->p_dynlib) 
    {
        // Lock dynlib object
        struct sx* dynlib_bind_lock = &p_Process->p_dynlib->bind_lock;
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        auto main_dylib_obj = p_Process->p_dynlib->main_obj;

        if (main_dylib_obj) 
        {
            // Search in all library
            int total = 0;
            auto dynlib_obj = main_dylib_obj;
            for (;;) 
            {
                total++;

                /*
                char* lib_name = (char*)(*(uint64_t*)(dynlib_obj + 8));
                void* relocbase = (void*)(*(uint64_t*)(dynlib_obj + 0x70));
                uint64_t handle = *(uint64_t*)(dynlib_obj + 0x28);

                WriteLog(LL_Info, "[%s] search(%i): %p lib_name: %s handle: 0x%lx relocbase: %p ...", s_TitleId, total, (void*)dynlib_obj, lib_name, handle, relocbase);
                */

                // Doing a dlsym with nids or name
                if ( (p_Flags & SUBSTITUTE_IAT_NIDS) )
                    s_Address = dynlib_do_dlsym((void*)p_Process->p_dynlib, (void*)dynlib_obj, p_Name, NULL, 0x1); // name = nids
                else
                    s_Address = dynlib_do_dlsym((void*)p_Process->p_dynlib, (void*)dynlib_obj, p_Name, NULL, 0x0); // use name (dynlib_do_dlsym will calculate later)

                if (s_Address)
                    break;

                dynlib_obj = dynlib_obj->link.sle_next;
                if (!dynlib_obj)
                    break;
            }
        } 
        else
            WriteLog(LL_Error, "[%s] Unable to find main object !", s_TitleId);

        // Unlock dynlib object
        A_sx_xunlock_hard(dynlib_bind_lock);
    } 
    else
        WriteLog(LL_Error, "[%s] The process is not Dynamic Linkable", s_TitleId);

    return s_Address;
}

// Substitute : Find pre-offset from the name or nids
void* Substitute::FindJmpslotAddress(struct proc* p_Process, const char* p_ModuleName, const char* p_Name, int32_t p_Flags) 
{
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto name_to_nids = (void(*)(const char *name, const char *nids_out))kdlsym(name_to_nids);
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_Name == nullptr || p_Process == nullptr) 
    {
        WriteLog(LL_Error, "Invalid argument.");
        return 0;   
    }

    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);

    // Get the nids of the function
    char s_Nids[0xD] = { 0 };
    if ( (p_Flags & SUBSTITUTE_IAT_NIDS) )
        snprintf(s_Nids, sizeof(s_Nids), "%s", p_Name); // nids = name
    else
        name_to_nids(p_Name, s_Nids); // nids calculated by name

    caddr_t s_NidsOffsetFound = 0;

    // Determine if we need to lock the process, then do it if needed
    bool s_ProcessUnlockNeeded = false;
    auto s_ProcessLocked = PROC_LOCKED(p_Process);
    if (s_ProcessLocked == false)
    {
        _mtx_lock_flags(&p_Process->p_mtx, 0);
        s_ProcessUnlockNeeded = true;
    }

    do
    {
        auto s_DynlibObj = p_Process->p_dynlib->objs.slh_first;
        if (s_DynlibObj == nullptr)
        {
            WriteLog(LL_Error, "[%s] The process (%d) (%s) is not dynamically linkable.", s_TitleId, p_Process->p_pid, p_Process->p_comm);
            break;
        }

        // Lock dynlib object (Note: Locking will panic kernel sometime)
        // BUG: Determine if we need to lock at all, or if the lock is already held (unlocking and causing races later)
        struct sx* s_DynlibBindLock = &p_Process->p_dynlib->bind_lock; //(struct sx*)((uint64_t)p->p_dynlib + 0x70);
        A_sx_xlock_hard(s_DynlibBindLock, 0);

        do
        {
            // Get the main dynlib object
            auto s_MainDynlibObj = p_Process->p_dynlib->main_obj;
            if (s_MainDynlibObj == nullptr) 
            {
                WriteLog(LL_Error, "main dynlib object is nullptr.");
                break;
            }

            // Search in all library
            auto s_DynlibObj = s_MainDynlibObj;

            // Check if we not are in the main executable
            if (strncmp(p_ModuleName, SUBSTITUTE_MAIN_MODULE, SUBSTITUTE_MAX_NAME) == 0)
            {
                WriteLog(LL_Debug, "not the main executable bailing.");
                break;
            }

            for (;;) 
            {
                char* s_LibName = (char*)(*(uint64_t*)(s_DynlibObj + 8));

                // If the libname (a path) containt the module name, it's the good object, break it
                if (s_LibName && strstr(s_LibName, p_ModuleName)) {
                    break;
                }

                s_DynlibObj = s_DynlibObj->link.sle_next; //*(uint64_t*)(s_DynlibObj);
                if (s_DynlibObj == nullptr) 
                {
                    WriteLog(LL_Error, "Unable to find the library.");
                    break;
                }
            }

            // Get the main relocbase address, for calculation after
            caddr_t s_RelocBase = s_DynlibObj->realloc_base; //(uint64_t)(*(uint64_t*)(s_DynlibObj + 0x70));

            // Get the pfi
            auto s_Pfi = s_DynlibObj->pfi;
            if (s_Pfi == nullptr)
            {
                WriteLog(LL_Error, "pfi invalid.");
                break;
            }

            caddr_t s_StringTable = s_Pfi->strtab;

            size_t s_PltRelaSize = s_Pfi->pltrelasize;
            caddr_t s_PltRela = s_Pfi->pltrela;

            if (s_PltRela == nullptr || s_PltRelaSize == 0)
            {
                WriteLog(LL_Error, "pltrela (%p) or pltrelasize (%lx) invalid.", s_PltRela, s_PltRelaSize);
                break;
            }

            const Elf64_Rela* s_RelaStart = (const Elf64_Rela*)s_PltRela;
            const Elf64_Rela* s_RelaEnd = (const Elf64_Rela*)(s_PltRela + s_PltRelaSize);

            for (const Elf64_Rela* s_Current = s_RelaStart; s_Current != s_RelaEnd; ++s_Current)
            {
                uint64_t s_SymbolIndex = ELF64_R_SYM(s_Current->r_info); // (*(uint64_t*)(s_Current + 0x8)) >> 32;
                uint64_t s_SymbolOffset = s_SymbolIndex * sizeof(Elf64_Sym);

                // Get the nids offset by looking up the symbol offset
                size_t s_NidStringOffset = 0;

                // Check to make sure that the symbol offset is within the symbol table size
                if (s_Pfi->symtabsize <= s_SymbolOffset) 
                    s_NidStringOffset = 0;
                else 
                {
                    caddr_t s_NidsOffsetAddress = s_Pfi->symtab /* *(uint64_t*)(s_UnkObj + 0x28) */ + s_SymbolOffset;
                    //const Elf64_Sym* s_NidsSymbol = (Elf64_Sym*)s_NidsOffsetAddress;
                    //s_NidsSymbol->st_info;

                    // TODO: Ask TW what this is supposed to be doing ????
                    s_NidStringOffset = (size_t)(*(uint32_t*)(s_NidsOffsetAddress));
                }

                // Make sure the string table offset is within bounds
                size_t s_StringTableSize = s_Pfi->strsize;
                if (s_StringTableSize <= s_NidStringOffset) 
                {
                    WriteLog(LL_Error, "[%s] (%p <= %p) : Error", s_TitleId, (void*)s_StringTableSize, (void*)s_NidStringOffset);
                    continue;
                }

                s_NidStringOffset += (size_t)s_StringTable;

                Elf64_Addr s_Offset = s_Current->r_offset;
                

                char* s_Nidsf = (char*)s_NidStringOffset;

                // If the nids_offset is a valid address
                if (s_Nidsf == nullptr) 
                {
                    WriteLog(LL_Error, "could not get the nids strings.");
                    continue;
                }

                // Check if it's the good nids
                if (strncmp(s_Nidsf, s_Nids, 11) == 0) 
                {
                    s_NidsOffsetFound = s_RelocBase + s_Offset;
                    break;
                }
            }

        } while (false);

        // Unlock dynlib object
        A_sx_xunlock_hard(s_DynlibBindLock);

    } while (false);

    if (s_ProcessUnlockNeeded)
        _mtx_unlock_flags(&p_Process->p_mtx, 0);

    return s_NidsOffsetFound;
}

///////////////////////////////////////
// PROCESS MANAGEMENT (Mount & Load) //
///////////////////////////////////////


// Substitute : Mount Substitute folder and prepare prx for load
bool Substitute::OnProcessExecEnd(struct proc *p_Process)
{
    WriteLog(LL_Debug, "process ending (%p).", p_Process);
    return true;
}

// Substitute : Unmount Substitute folder
bool Substitute::OnProcessExit(struct proc *p_Process) 
{
    //auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    //auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    if (!p_Process)
    {
        WriteLog(LL_Error, "invalid process.");
        return false;
    }

    WriteLog(LL_Debug, "proc locked: (%d).", PROC_LOCKED(p_Process));

    // Get process information
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p_Process);
    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);

    Substitute* s_Substitute = GetPlugin();

    // Check if it's a valid process
    if ( !s_TitleId || s_TitleId[0] == 0 )
    {
        WriteLog(LL_Error, "title id blank or invalid.");
        return true;
    }

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);
        return false;
    }

    // Start by cleanup hook list
    s_Substitute->CleanupProcessHook(p_Process);

    /*

    // Getting jailed path for the process
    struct filedesc* s_FileDesc = p_Process->p_fd;

    char* s_SandboxPath = nullptr;
    char* s_Freepath = nullptr;
    vn_fullpath(s_MainThread, s_FileDesc->fd_jdir, &s_SandboxPath, &s_Freepath);

    if (s_SandboxPath == nullptr) 
    {
        if (s_Freepath)
            delete (s_Freepath);
        
        WriteLog(LL_Error, "vn_fullpath failed.");

        return false;
    }

    // Finding substitute folder
    char s_SubstituteFullMountPath[PATH_MAX];
    snprintf(s_SubstituteFullMountPath, PATH_MAX, "%s/_substitute", s_SandboxPath);

    auto s_DirectoryHandle = kopen_t(s_SubstituteFullMountPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        // Free the memory used by freepath
        if (s_Freepath)
            delete (s_Freepath);
        
        WriteLog(LL_Error, "could not open directory handle (%s) (%d).", s_SubstituteFullMountPath, s_DirectoryHandle);

        return false;
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    WriteLog(LL_Info, "[%s] Cleaning substitute ...", s_TitleId);

    // Unmount substitute folder if the folder exist
    s_Ret = kunmount_t(s_SubstituteFullMountPath, MNT_FORCE, s_MainThread);
    if (s_Ret < 0) 
        WriteLog(LL_Error, "could not unmount folder (%s) (%d), Trying to remove anyway ...", s_SubstituteFullMountPath, s_Ret);

    // Remove substitute folder
    s_Ret = krmdir_t(s_SubstituteFullMountPath, s_MainThread);
    if (s_Ret < 0) 
    {
        WriteLog(LL_Error, "could not remove substitute folder (%s) (%d).", s_SubstituteFullMountPath, s_Ret);

        // Free the memory used by freepath
        if (s_Freepath)
            delete (s_Freepath);

        return false;
    }

    if (s_Freepath)
        delete (s_Freepath);
*/
    WriteLog(LL_Info, "[%s] Substitute have been cleaned.", s_TitleId);
    return true;
}

//////////////////////////
// USERLAND IOCTL WRAPPER
//////////////////////////

// Substitute (IOCTL) : Do a IAT Hook for the specified thread
int Substitute::OnIoctl_HookIAT(struct thread* p_Thread, struct substitute_hook_iat* p_Uap) 
{
    // BUG: We do not copyin anything, we directly reference, this function needs to be rewritten

    int s_HookId = -1;
    if (!p_Thread || !p_Uap) 
    {
        WriteLog(LL_Error, "Invalid argument !");        
        return EINVAL;
    }

    Substitute* s_Substitute = GetPlugin();
    if (!s_Substitute) 
    {
        WriteLog(LL_Error, "Unable to got substitute object !");
        p_Uap->hook_id = -1;
        return 0;
    }

    s_HookId = s_Substitute->HookIAT(p_Thread->td_proc, p_Uap->chain, p_Uap->module_name, p_Uap->name, p_Uap->flags, p_Uap->hook_function);
    if (s_HookId >= 0) 
    {
        WriteLog(LL_Info, "New hook at %i", s_HookId);
    } 
    else 
    {
        WriteLog(LL_Error, "Unable to hook %s ! (%i)", p_Uap->name, s_HookId);
    }

    p_Uap->hook_id = s_HookId;
    return 0;
}

// Substitute (IOCTL) : Do a JMP Hook for the specified thread
int Substitute::OnIoctl_HookJMP(struct thread* p_Thread, struct substitute_hook_jmp* p_Uap) 
{
    // BUG: This function needs to be re-written, or ioctl needs to copyin first before trying to use uap

    int s_HookId = -1;
    if (!p_Thread || !p_Uap) 
    {
        WriteLog(LL_Error, "Invalid argument !");        
        return EINVAL;
    }

    Substitute* s_Substitute = GetPlugin();
    if (!s_Substitute) 
    {
        WriteLog(LL_Error, "Unable to got substitute object !");
        p_Uap->hook_id = s_HookId;
        return 0;
    }

    s_HookId = s_Substitute->HookJmp(p_Thread->td_proc, p_Uap->original_function, p_Uap->hook_function);
    if (s_HookId >= 0) 
    {
        WriteLog(LL_Info, "New hook at %i", s_HookId);
    } 
    else 
    {
        WriteLog(LL_Error, "Unable to hook %p ! (%i)", p_Uap->original_function, s_HookId);
    }

    p_Uap->hook_id = s_HookId;
    return 0;
}

// Substitute (IOCTL) : Enable / Disable hook
int Substitute::OnIoctl_StateHook(struct thread* p_Thread, struct substitute_state_hook* p_Uap) 
{
    // BUG: This function needs to be rewritten, need to copyin before touching uap
    if (!p_Thread || !p_Uap) 
    {
        WriteLog(LL_Error, "Invalid argument !");        
        return EINVAL;
    }

    int s_Ret = -1;

    Substitute* s_Substitute = GetPlugin();
    if (!s_Substitute) 
    {
        WriteLog(LL_Error, "Unable to got substitute object !");
        p_Uap->result = s_Ret;
        return 0;
    }

    switch (p_Uap->state) 
    {
        case SUBSTITUTE_STATE_ENABLE: 
        {
            s_Ret = s_Substitute->EnableHook(p_Thread->td_proc, p_Uap->hook_id);
            if (s_Ret < 0) 
            {
                WriteLog(LL_Error, "Unable to enable hook %i !", p_Uap->hook_id);
                p_Uap->result = s_Ret;
            } 
            else 
            {
                s_Ret = 1;
                WriteLog(LL_Info, "Hook %i enabled !", p_Uap->hook_id);
                p_Uap->result = s_Ret;
            }

            break;
        }

        case SUBSTITUTE_STATE_DISABLE: 
        {
            s_Ret = s_Substitute->DisableHook(p_Thread->td_proc, p_Uap->hook_id);
            if (s_Ret < 0) 
            {
                WriteLog(LL_Error, "Unable to disable hook %i (%d) !", p_Uap->hook_id, s_Ret);
                p_Uap->result = s_Ret;
            } 
            else 
            {
                s_Ret = 1;
                WriteLog(LL_Info, "Hook %i disable !", p_Uap->hook_id);
                p_Uap->result = s_Ret;
            }
            
            break;
        }

        case SUBSTITUTE_STATE_UNHOOK: 
        {
            s_Ret = s_Substitute->Unhook(p_Thread->td_proc, p_Uap->hook_id, p_Uap->chain);
            if (s_Ret < 0) {
                WriteLog(LL_Error, "Unable to unhook %i (%d) !", p_Uap->hook_id, s_Ret);  
                p_Uap->result = s_Ret;
            } else {
                s_Ret = 1;
                WriteLog(LL_Info, "Unhook %i was complete !", p_Uap->hook_id);
                p_Uap->result = s_Ret;
            }
        }

        default: 
        {
            WriteLog(LL_Error, "Invalid state detected.");
            p_Uap->result = s_Ret;
            break;
        }
    }

    return 0;
}

// Substitute (IOCTL) : Main drive (See CtrlDriver in /src/Driver/)
int32_t Substitute::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    // TODO: Implement copyin/copyout so we aren't accessing userland memory from kernel
    // RESPECT THE BOUNDARIES
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    
    switch (p_Command) {
        case SUBSTITUTE_HOOK_IAT: 
        {
            struct substitute_hook_iat s_Uap = { 0 };
            if (copyin(p_Data, &s_Uap, sizeof(s_Uap)))
            {
                WriteLog(LL_Error, "could not copyin enough data for substitute hook iat.");
                return 0;
            }

            return Substitute::OnIoctl_HookIAT(p_Thread, &s_Uap);
        }

        case SUBSTITUTE_HOOK_JMP: 
        {
            return Substitute::OnIoctl_HookJMP(p_Thread, (struct substitute_hook_jmp*)p_Data);
        }

        case SUBSTITUTE_HOOK_STATE: 
        {
            return Substitute::OnIoctl_StateHook(p_Thread, (struct substitute_state_hook*)p_Data);
        }

        default: 
        {
            WriteLog(LL_Debug, "unknown command: (0x%llx).", p_Command);
            break;
        }
    }

    return 0;
}
