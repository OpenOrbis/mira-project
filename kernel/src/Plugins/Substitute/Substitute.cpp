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
    memset(hooks, 0, sizeof(SubstituteHook) * SUBSTITUTE_MAX_HOOKS);
}

// Substitute : Destructor
Substitute::~Substitute()
{
}

// Substitute : Plugin loaded
bool Substitute::OnLoad()
{
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    struct sysent* sysents = sv->sv_table;
    //auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    WriteLog(LL_Info, "Loading Substitute ...");

    // Substitute syscall hook (PRX Loader)
    sys_dynlib_dlsym_p = (void*)sysents[SYS_DYNLIB_DLSYM].sy_call;
    sysents[SYS_DYNLIB_DLSYM].sy_call = (sy_call_t*)Sys_dynlib_dlsym_hook;

    mtx_init(&hook_mtx, "Substitute SPIN Lock", NULL, MTX_SPIN);

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
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    struct sysent* sysents = sv->sv_table;
    //auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
    //auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    WriteLog(LL_Error, "Unloading Substitute ...");

    // Cleanup substitute hook (PRX Loader)
    if (sys_dynlib_dlsym_p) {
        sysents[SYS_DYNLIB_DLSYM].sy_call = (sy_call_t*)sys_dynlib_dlsym_p;
        sys_dynlib_dlsym_p = nullptr;
    }

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
SubstituteHook* Substitute::GetHookByID(int hook_id) {
    // Check if the hook_id is in the specs
    if (hook_id < 0 || hook_id > SUBSTITUTE_MAX_HOOKS) {
        WriteLog(LL_Error, "Invalid hook id ! (%d)", hook_id);
        return nullptr;
    }

    // If the hook is empty, it's an invalid hook !
    SubstituteHook empty;
    memset(&empty, 0, sizeof(SubstituteHook));
    if (memcmp(&hooks[hook_id], &empty, sizeof(SubstituteHook)) == 0) {
        return nullptr;
    }

    // Return the address
    return &hooks[hook_id];
}

// Substitute : Allocate a new hook to the list (MUTEX NEEDED)
SubstituteHook* Substitute::AllocateHook(int* hook_id) {
    if (!hook_id) {
        WriteLog(LL_Error, "Invalid argument !");
        return nullptr;
    }

    SubstituteHook empty;
    memset(&empty, 0, sizeof(SubstituteHook));

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) {
        if (memcmp(&hooks[i], &empty, sizeof(SubstituteHook)) == 0) {
            *hook_id = i;
            return &hooks[i];
        }
    }

    *hook_id = -1;
    return nullptr;
}

// Substitute : Free a hook from the list (MUTEX NEEDED)
void Substitute::FreeHook(int hook_id) {
    SubstituteHook* hook = GetHookByID(hook_id);

    if (hook) {
        // Free the chain list
        if (hook->hook_type == HOOKTYPE_IAT && hook->iat.uap_chains) {
            delete (hook->iat.uap_chains);
        }

        // Set to 0, now everything is available
        memset(hook, 0, sizeof(SubstituteHook));
        WriteLog(LL_Info, "The hook %i have been deleted.", hook_id);
    }

    return;
}


//////////////////////////////////////////
// HOOK MANAGEMENT SYSTEM (Kernel Side) //
//////////////////////////////////////////

// Substitute : Find a hook with similar process and jmpslot (MUTEX NEEDED)
bool Substitute::FindJmpslotSimilarity(struct proc* p, void* jmpslot_addr, int* hook_id) {
    if (!hook_id) {
        WriteLog(LL_Error, "Invalid argument !");
        *hook_id = -1;
        return false;
    }

    SubstituteHook empty;
    memset(&empty, 0, sizeof(SubstituteHook));

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) {
        // If empty, don't continue, just break instead of do all the list
        if (memcmp(&hooks[i], &empty, sizeof(SubstituteHook)) == 0) {
            break;
        }

        if (hooks[i].hook_type == HOOKTYPE_IAT) {
            if (hooks[i].process == p && hooks[i].iat.jmpslot_address == jmpslot_addr) {
                *hook_id = i;
                return true;
            }
        }
    }

    *hook_id = -1;
    return false;
}

// Substitute : Find position by chains address
int Substitute::FindPositionByChain(uint64_t* chains, uint64_t chain) {
    if (!chains) {
        WriteLog(LL_Error, "Invalid argument !");
        return -1;
    }

    for (int i = 0; i < SUBSTITUTE_MAX_CHAINS; i++) {
        if (chains[i] == chain) {
            return i;
        }
    }

    return -1;
}

// Substitute : Get UAP Address of last occurence of a chains (MUTEX NEEDED)
void* Substitute::FindLastChainOccurence(uint64_t* chains, int* position) {
    if (!chains || !position) {
        WriteLog(LL_Error, "Invalid argument !");
        return nullptr;
    }

    for (int i = 0; i < SUBSTITUTE_MAX_CHAINS; i++) {
        if (chains[i] == 0) {

            if ( (i-1) < 0 )
                return nullptr;
            *position = (i-1);
            return (void*)chains[i-1];
        }
    }

    return nullptr;
}

// Substitute : Hook the function in the process (With Import Address Table)
int Substitute::HookIAT(struct proc* p, struct substitute_hook_uat* chain, const char* module_name, const char* name, int32_t flags, void* hook_function) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int r_error = 0;

    if (!p || !name || !hook_function || !chain) {
        WriteLog(LL_Error, "One of the parameter is incorrect !");
        WriteLog(LL_Error, "p: %p", p);
        WriteLog(LL_Error, "chain: %p", chain);
        WriteLog(LL_Error, "name: %p", name);
        WriteLog(LL_Error, "hook_function: %p", hook_function);
        WriteLog(LL_Error, "One of the parameter is incorrect !");
        return SUBSTITUTE_BAD_ARGS;
    }

    // Get the jmpslot offset for this nids (Work like an id for detect if hook already present)
    void* jmpslot_address = (void*)FindJmpslotAddress(p, module_name, name, flags);
    if (!jmpslot_address) {
        WriteLog(LL_Error, "Unable to find the jmpslot address !");
        return SUBSTITUTE_INVALID;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    // Find if something is similar for this process
    int hook_id = -1;
    WriteLog(LL_Info, "jmpslot addr: %p", jmpslot_address);
    if (FindJmpslotSimilarity(p, jmpslot_address, &hook_id)) {
        // Similarity : Rebuild the hook chain for handle new hook of same function/process
        WriteLog(LL_Info, "Similarity : Rebuild the hook chain");
        SubstituteHook* hook = GetHookByID(hook_id);

        if (!hook) {
            WriteLog(LL_Error, "Unable to get the hook !");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return SUBSTITUTE_INVALID;
        }

        // Build the current chain
        struct substitute_hook_uat current_chain;
        current_chain.hook_id = hook_id;
        current_chain.hook_function = hook_function;
        current_chain.original_function = hook->iat.original_function;
        current_chain.next = nullptr;

        // Find lasted chain uap position and address
        int last_chain_position = -1;
        void* last_chain_addr = FindLastChainOccurence(hook->iat.uap_chains, &last_chain_position);
        if (!last_chain_addr || last_chain_position < 0) {
            WriteLog(LL_Error, "Unable to get the lasted chain address.");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return SUBSTITUTE_INVALID;
        }

        // Determinate if we have space for write
        if ( (last_chain_position + 1) > SUBSTITUTE_MAX_CHAINS ) {
            WriteLog(LL_Error, "SUBSTITUTE_MAX_CHAINS reached !");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_NOMEM;
        }

        // Write current chain in the process
        r_error = proc_rw_mem(p, chain, sizeof(struct substitute_hook_uat), &current_chain, nullptr, true);
        if (r_error != 0) {
            WriteLog(LL_Error, "Unable to write chains structure: (%i)", r_error);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_INVALID;
        }

        // Got the last chains struct into kernel
        struct substitute_hook_uat last_chain;
        r_error = proc_rw_mem(p, last_chain_addr, sizeof(struct substitute_hook_uat), &last_chain, nullptr, false);
        if (r_error != 0) {
            WriteLog(LL_Error, "Unable to read the last chain: (%d)", r_error);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_INVALID;
        }

        // The next chain of last chain is the current chain
        last_chain.next = chain;

        // Write the edited last chain in the process
        r_error = proc_rw_mem(p, last_chain_addr, sizeof(struct substitute_hook_uat), &last_chain, nullptr, true);
        if (r_error != 0) {
            WriteLog(LL_Error, "Unable to write chains structure: (%i)", r_error);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_INVALID;
        }

        // Write the new chain address into the chain structure
        hook->iat.uap_chains[(last_chain_position + 1)] = (uint64_t)chain;
    } else {
        // No similarity : Build a new hook block
        WriteLog(LL_Info, "No similarity : Build a new hook block ...");

        // Get the original value for this jmpslot
        void* original_function = FindOriginalAddress(p, name, flags);
        if (!original_function) {
            WriteLog(LL_Error, "Unable to get the original value from the jmpslot !");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_INVALID;
        }

        hook_id = -1;
        SubstituteHook* new_hook = AllocateHook(&hook_id);
        if (!new_hook || hook_id < 0) {
            WriteLog(LL_Error, "Unable to allocate new hook (%p => %d) !", new_hook, hook_id);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return SUBSTITUTE_NOMEM;
        }

        // Initialize chains
        uint64_t* chains = new uint64_t[SUBSTITUTE_MAX_CHAINS];
        memset(chains, 0, sizeof(uint64_t) * SUBSTITUTE_MAX_CHAINS);

        // Setup uat values
        struct substitute_hook_uat setup;
        setup.hook_id = hook_id;
        setup.hook_function = hook_function;
        setup.original_function = original_function;
        setup.next = nullptr;

        // Write uat values into the process
        r_error = proc_rw_mem(p, chain, sizeof(struct substitute_hook_uat), &setup, nullptr, true);
        if (r_error != 0) {
            WriteLog(LL_Error, "Unable to write chains structure: (%i)", r_error);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            // Free memory
            delete [] chains;
            return SUBSTITUTE_INVALID;
        }

        // Write the first chains address
        chains[0] = (uint64_t)chain;

        // Set default value
        new_hook->process = p;
        new_hook->hook_type = HOOKTYPE_IAT;
        new_hook->iat.uap_chains = chains;
        new_hook->iat.jmpslot_address = jmpslot_address;
        new_hook->iat.original_function = original_function;

        // Enable first hook !
        // Overwrite the jmpslot slot address
        r_error = proc_rw_mem(p, jmpslot_address, sizeof(void*), &hook_function, nullptr, true);
        if (r_error != 0) {
            WriteLog(LL_Error, "Unable to write to the jmpslot address: (%d)", r_error);
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            delete[] chains;
            return SUBSTITUTE_INVALID;
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "New hook is created (IAT) :  %i", hook_id);
    return hook_id;
}

// Substitute : Hook the function in the process (With longjmp)
int Substitute::HookJmp(struct proc* p, void* original_address, void* hook_function) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p || !original_address || !hook_function)
        return SUBSTITUTE_BAD_ARGS;

    // Get buffer from original function for calculate size
    char buffer[500];
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)original_address, sizeof(buffer), (void*)buffer, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Error, "Unable to get the buffer !");
        return SUBSTITUTE_INVALID;
    }

    // Set the original address
    int backupSize = Utils::Hook::GetMinimumHookSize(buffer);
    if (backupSize <= 0) {
        WriteLog(LL_Error, "Unable to get the minimum hook size.");
        return SUBSTITUTE_INVALID;
    }

    // Malloc data for the backup data
    char* backupData = new char[backupSize];
    if (!backupData) {
        WriteLog(LL_Error, "Unable to allocate memory for backup");
        return SUBSTITUTE_NOMEM;
    }

    // Get the backup data
    read_size = 0;
    r_error = proc_rw_mem(p, (void*)original_address, backupSize, (void*)backupData, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Info, "Unable to get backupData (%d)", r_error);
        delete[] (backupData);
        return SUBSTITUTE_INVALID;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    int hook_id = -7;
    SubstituteHook* new_hook = AllocateHook(&hook_id);
    if (!new_hook || hook_id < 0) {
        WriteLog(LL_Error, "Unable to allocate new hook (%p => %d) !", new_hook, hook_id);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return SUBSTITUTE_NOMEM;
    }

    // Set default value
    new_hook->process = p;
    new_hook->hook_type = HOOKTYPE_JMP;
    new_hook->jmp.jmpto = hook_function;
    new_hook->jmp.orig_addr = original_address;
    new_hook->jmp.backupData = backupData;
    new_hook->jmp.backupSize = backupSize;
    new_hook->jmp.enable = false;

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "New hook is created (JMP) :  %i", hook_id);
    return hook_id;
}

// Substitute : Disable the hook
int Substitute::DisableHook(struct proc* p, int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    // Lock the process

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* hook = GetHookByID(hook_id);
    if (!hook) {
        WriteLog(LL_Error, "The hook %i is not found !.", hook_id);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

        return SUBSTITUTE_NOTFOUND;
    }

    if (hook->process != p) {
        WriteLog(LL_Error, "Invalid handle : Another process trying to disable hook.");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

        return SUBSTITUTE_INVALID;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            WriteLog(LL_Error, "!!! WARNING !!! Not compatible with IAT Hook.");
            break;
        }

        case HOOKTYPE_JMP : {
            if (hook->process && hook->jmp.enable && hook->jmp.orig_addr && hook->jmp.backupSize > 0 && hook->jmp.backupData) {
                int r_error = proc_rw_mem(hook->process, hook->jmp.orig_addr, hook->jmp.backupSize, hook->jmp.backupData, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write original address: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

                    return SUBSTITUTE_BADLOGIC;
                }

                hook->jmp.enable = false;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        default: {
            WriteLog(LL_Error, "Invalid type of hook was detected.");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

            return SUBSTITUTE_INVALID;
            break;
        }
    }
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);


    return SUBSTITUTE_OK;
}

// Substitute : Enable the hook
int Substitute::EnableHook(struct proc* p, int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int r_error = -1;

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* hook = GetHookByID(hook_id);
    if (!hook) {
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -1;
    }

    if (hook->process != p) {
        WriteLog(LL_Error, "Invalid handle : Another process trying to enable hook.");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -2;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            WriteLog(LL_Error, "!!! WARNING !!! Not compatible with IAT Hook.");
            break;
        }

        case HOOKTYPE_JMP : {
            if (hook->process && !hook->jmp.enable && hook->jmp.orig_addr && hook->jmp.jmpto) {

                // Use the jmpBuffer from Hook.cpp
                uint8_t jumpBuffer[] = {
                    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,             // # jmp    QWORD PTR [rip+0x0]
                    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, // # DQ: AbsoluteAddress
                }; // Shit takes 14 bytes

                uint64_t* jumpBufferAddress = (uint64_t*)(jumpBuffer + 6);

                // Assign the address
                *jumpBufferAddress = (uint64_t)hook->jmp.jmpto;

                r_error = proc_rw_mem(hook->process, hook->jmp.orig_addr, sizeof(jumpBuffer), jumpBuffer, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write the jmp system: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -5;
                }

                hook->jmp.enable = true;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        default: {
            WriteLog(LL_Error, "Invalid type of hook was detected.");
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return -6;
            break;
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return SUBSTITUTE_OK;
}

// Substitute : Unhook the function
int Substitute::Unhook(struct proc* p, int hook_id, struct substitute_hook_uat* chain) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int r_error = -1;

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* hook = GetHookByID(hook_id);
    if (!hook) {
        WriteLog(LL_Error, "The hook %i is not found !.", hook_id);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -1;
    }

    if (hook->process != p) {
        WriteLog(LL_Error, "Invalid handle : Another process trying to disable hook.");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -2;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            // Need to fix the chains

            // Got the current chain
            struct substitute_hook_uat current_chain;
            r_error = proc_rw_mem(p, chain, sizeof(struct substitute_hook_uat), &current_chain, nullptr, false);
            if (r_error != 0) {
                WriteLog(LL_Error, "Unable to read the current chain: (%d)", r_error);
                _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                return -3;
            }

            // I get the current position on the chain
            int chain_position = FindPositionByChain(hook->iat.uap_chains, (uint64_t)chain);
            if (chain_position < 0) {
                WriteLog(LL_Error, "Unable to get the current chain position: (%d)", chain_position);
                _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                return -4;
            }

            // Before need to have the address of after
            if (chain_position > 0) {
                // If it's something inside, just need to do before->next = after
                struct substitute_hook_uat before_chain;
                void* before_chain_addr = (void*)hook->iat.uap_chains[chain_position-1];

                // Read the before chain
                r_error = proc_rw_mem(p, before_chain_addr, sizeof(struct substitute_hook_uat), &before_chain, nullptr, false);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to read the before chain: (%d)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -5;
                }

                uint64_t next_addr = 0;
                if ( (chain_position+1) < SUBSTITUTE_MAX_CHAINS ) {
                    next_addr = hook->iat.uap_chains[chain_position+1];
                }

                before_chain.next = (struct substitute_hook_uat*)next_addr;
            } else if (chain_position == 0) {
                // If it's 0, all the chain is okey, just need to edit the jmpslot

                // Get the next function chain
                struct substitute_hook_uat next;
                void* next_addr = (void*)hook->iat.uap_chains[1];

                // Read the next chain
                r_error = proc_rw_mem(p, next_addr, sizeof(struct substitute_hook_uat), &next, nullptr, false);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to read the next chain: (%d)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -7;
                }

                // Edit the jmpslot address for the current hook
                r_error = proc_rw_mem(p, hook->iat.jmpslot_address, sizeof(void*), &next.hook_function, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write to the jmpslot address: (%d)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -8;
                }
            } else {
                WriteLog(LL_Error, "Very very very weird issues is here");
                _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                return -9;

            }

            // Update the chains table !
            for (int i = chain_position; i < SUBSTITUTE_MAX_CHAINS; i++) {
                if (hook->iat.uap_chains[i] == 0)
                    break;

                if ( (i+1) < SUBSTITUTE_MAX_CHAINS)
                    hook->iat.uap_chains[i] = hook->iat.uap_chains[i+1];
            }
        }

        case HOOKTYPE_JMP: {
            // Simply free the space :)
            FreeHook(hook_id);
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "%i is now unhooked.", hook_id);

    return SUBSTITUTE_OK;
}

// Substitute : Cleanup all hook
void Substitute::CleanupAllHook() {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    WriteLog(LL_Info, "Cleaning up all hook ...");

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) {
        FreeHook(i);
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
}

// Substitute : Cleanup hook for a process
void Substitute::CleanupProcessHook(struct proc* p) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p)
        return;

    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    WriteLog(LL_Info, "Cleaning up hook for %s", s_TitleId);


    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    for (int i = 0; i < SUBSTITUTE_MAX_HOOKS; i++) {
        SubstituteHook* hook = GetHookByID(i);
        if (hook && hook->process == p) {
            FreeHook(i);
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
}

//////////////////////////
// IAT HOOK UTILITY
//////////////////////////

// Substitute : Find original function address by this name (or nids)
void* Substitute::FindOriginalAddress(struct proc* p, const char* name, int32_t flags)
{
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto dynlib_do_dlsym = (void*(*)(void* dl, void* obj, const char* name, const char* libname, unsigned int flags))kdlsym(dynlib_do_dlsym);

    if (p == nullptr)
        return nullptr;

    // TODO: Fix this structure within proc
    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    void* s_Address = nullptr;

    WriteLog(LL_Info, "TitleId: (%s).", s_TitleId);

    if (p->p_dynlib) {
        // Lock dynlib object
        struct sx* dynlib_bind_lock = &p->p_dynlib->bind_lock;
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        auto main_dylib_obj = p->p_dynlib->main_obj;

        if (main_dylib_obj) {
            // Search in all library
            int total = 0;
            auto dynlib_obj = main_dylib_obj;
            for (;;) {
                total++;

                /*
                char* lib_name = (char*)(*(uint64_t*)(dynlib_obj + 8));
                void* relocbase = (void*)(*(uint64_t*)(dynlib_obj + 0x70));
                uint64_t handle = *(uint64_t*)(dynlib_obj + 0x28);

                WriteLog(LL_Info, "[%s] search(%i): %p lib_name: %s handle: 0x%lx relocbase: %p ...", s_TitleId, total, (void*)dynlib_obj, lib_name, handle, relocbase);
                */

                // Doing a dlsym with nids or name
                if ( (flags & SUBSTITUTE_IAT_NIDS) ) {
                    s_Address = dynlib_do_dlsym((void*)p->p_dynlib, (void*)dynlib_obj, name, NULL, 0x1); // name = nids
                } else {
                    s_Address = dynlib_do_dlsym((void*)p->p_dynlib, (void*)dynlib_obj, name, NULL, 0x0); // use name (dynlib_do_dlsym will calculate later)
                }

                if (s_Address) {
                    break;
                }

                dynlib_obj = dynlib_obj->link.sle_next;
                if (!dynlib_obj)
                    break;
            }
        } else {
            WriteLog(LL_Error, "[%s] Unable to find main object !", s_TitleId);
        }

        // Unlock dynlib object
        A_sx_xunlock_hard(dynlib_bind_lock);
    } else {
        WriteLog(LL_Error, "[%s] The process is not Dynamic Linkable", s_TitleId);
    }

    return s_Address;
}

// Substitute : Find pre-offset from the name or nids
uint64_t Substitute::FindJmpslotAddress(struct proc* p, const char* module_name, const char* name, int32_t flags) {
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto name_to_nids = (void(*)(const char *name, const char *nids_out))kdlsym(name_to_nids);

    if (name == nullptr || p == nullptr) {
        WriteLog(LL_Error, "Invalid argument.");
        return 0;
    }

    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    // Get the nids of the function
    char nids[0xD] = { 0 };
    if ( (flags & SUBSTITUTE_IAT_NIDS) ) {
        snprintf(nids, sizeof(nids), "%s", name); // nids = name
    } else {
        name_to_nids(name, nids); // nids calculated by name
    }

    uint64_t nids_offset_found = 0;

    if (p->p_dynlib->objs.slh_first) {
        // Lock dynlib object (Note: Locking will panic kernel sometime)
        struct sx* dynlib_bind_lock = &p->p_dynlib->bind_lock; //(struct sx*)((uint64_t)p->p_dynlib + 0x70);
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        uint64_t main_dylib_obj = (uint64_t)p->p_dynlib->main_obj; //*(uint64_t*)((uint64_t)p->p_dynlib + 0x10);

        if (main_dylib_obj) {
            // Search in all library
            uint64_t dynlib_obj = main_dylib_obj;

            // Check if we not are in the main executable
            if (strncmp(module_name, SUBSTITUTE_MAIN_MODULE, SUBSTITUTE_MAX_NAME) != 0) {
                for (;;) {
                    char* lib_name = (char*)(*(uint64_t*)(dynlib_obj + 8));

                    // If the libname (a path) containt the module name, it's the good object, break it
                    if (lib_name && strstr(lib_name, module_name)) {
                        break;
                    }

                    dynlib_obj = *(uint64_t*)(dynlib_obj);
                    if (!dynlib_obj) {
                        WriteLog(LL_Error, "Unable to find the library.");
                        A_sx_xunlock_hard(dynlib_bind_lock);
                        return 0; // Library not found
                    }
                }
            }

            // Get the main relocbase address, for calculation after
            uint64_t relocbase = (uint64_t)(*(uint64_t*)(dynlib_obj + 0x70));

            uint64_t unk_obj = *(uint64_t*)(dynlib_obj + 0x150);
            if (unk_obj) {
                uint64_t string_table = *(uint64_t*)(unk_obj + 0x38);

                uint64_t unk_obj_size_in_unk_obj = *(uint64_t*)(unk_obj + 0x50);
                uint64_t unk_obj_in_obj = *(uint64_t*)(unk_obj + 0x48);

                // Idk what is it ^^', check it anyway, conform to Sony kernel
                if (unk_obj_in_obj && unk_obj_size_in_unk_obj) {
                    uint64_t current = unk_obj_in_obj;
                    uint64_t end_addr = unk_obj_in_obj + unk_obj_size_in_unk_obj;
                    while(current < end_addr)
                    {
                        uint64_t value_of_rcx = (*(uint64_t*)(current + 0x8)) >> 32;
                        uint64_t value_of_rdx = value_of_rcx * 24;

                        uint64_t nids_offset = 0;
                        if (*(uint64_t*)(unk_obj + 0x30) <= value_of_rdx) {
                            nids_offset = 0;
                        } else {
                            uint64_t nids_offset_ptr = *(uint64_t*)(unk_obj + 0x28) + value_of_rdx;
                            nids_offset = (uint64_t)(*(uint32_t*)(nids_offset_ptr));
                        }

                        uint64_t nids_ptr_validation = *(uint64_t*)(unk_obj + 0x40);
                        if (nids_ptr_validation <= nids_offset) {
                             WriteLog(LL_Error, "[%s] (%p <= %p) : Error", s_TitleId, (void*)nids_ptr_validation, (void*)nids_offset);
                        } else {
                            nids_offset += string_table;

                            /* uint64_t r_info = *(uint64_t*)(current + 0x8); */

                            uint64_t r_offset = *(uint64_t*)(current);

                            char* nids_f = (char*)nids_offset;

                            // If the nids_offset is a valid address
                            if (nids_f) {
                                // Check if it's the good nids
                                if (strncmp(nids_f, nids, 11) == 0) {
                                    nids_offset_found = relocbase + r_offset;
                                    break;
                                }
                            }
                        }

                        current += 0x18;
                    }
                } else {
                    WriteLog(LL_Error, "[%s] Not conform to Sony code.", s_TitleId);
                    WriteLog(LL_Error, "[%s] unk_obj_in_obj: %p", s_TitleId, (void*)unk_obj_in_obj);
                    WriteLog(LL_Error, "[%s] unk_obj_size_in_unk_obj: %p", s_TitleId, (void*)unk_obj_size_in_unk_obj);
                }
            } else {
                WriteLog(LL_Error, "[%s] Unable to find unk object !", s_TitleId);
            }
        } else {
            WriteLog(LL_Error, "[%s] Unable to find main object !", s_TitleId);
        }

        // Unlock dynlib object
        A_sx_xunlock_hard(dynlib_bind_lock);
    } else {
        WriteLog(LL_Error, "[%s] The process is not Dynamic Linkable", s_TitleId);
    }

    return nids_offset_found;
}

///////////////////////////////////////
// PROCESS MANAGEMENT (Mount & Load) //
///////////////////////////////////////

// Substitute : Load SPRX over a folder (Thread selection)
// Note: Relative path (from sandbox of the target app)
void Substitute::LoadAllPrx(struct thread* td, const char* folder_path)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);

    // Check for arguments
    if (!td || !folder_path) {
        WriteLog(LL_Error, "Invalid arguments.");
        return;
    }

    char* s_TitleId = (char*)((uint64_t)td->td_proc + 0x390);

    // Opening substitute folder
    auto s_DirectoryHandle = kopen_t(folder_path, O_RDONLY | O_DIRECTORY, 0777, td);
    if (s_DirectoryHandle > 0)
    {
        // Reading folder and search for sprx ...
        uint64_t s_DentCount = 0;
        char s_Buffer[0x1000] = { 0 };
        memset(s_Buffer, 0, sizeof(s_Buffer));
        int32_t s_ReadCount = 0;
        for (;;)
        {
            memset(s_Buffer, 0, sizeof(s_Buffer));
            s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), td);
            if (s_ReadCount <= 0)
                break;

            for (auto l_Pos = 0; l_Pos < s_ReadCount;)
            {
                auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);
                s_DentCount++;

                // Check if the sprx is legit
                if (strstr(l_Dent->d_name, ".sprx")) {
                    // Generating relative path
                    char s_RelativeSprxPath[PATH_MAX];
                    snprintf(s_RelativeSprxPath, PATH_MAX, "%s%s", folder_path, l_Dent->d_name);

                    // Create relative path for the load
                    WriteLog(LL_Info, "[%s] Loading  %s ...", s_TitleId, s_RelativeSprxPath);
                    Utilities::LoadPRXModule(td->td_proc, s_RelativeSprxPath);                    

                    WriteLog(LL_Info, "Loading PRX Done !");
                }

                l_Pos += l_Dent->d_reclen;
            }
        }

        // Closing substitute folder
        kclose_t(s_DirectoryHandle, td);

        // TODO: It's impossible to cleanup because page is needed after !
        //WriteLog(LL_Info, "[%s] cleanup payload (fake result: %p)...", s_TitleId, (void*)uap->result);
        //kmunmap_t(uap->result, 0x8000, td);
    }
}

// Substitute : Mount Substitute folder and prepare prx for load
bool Substitute::OnProcessExecEnd(struct proc *p)
{
    if (!p)
        return false;

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    // Check if it's a valid process
    if ( !s_TitleId || s_TitleId[0] == 0 )
        return false;

    char s_SprxDirPath[PATH_MAX];
    snprintf(s_SprxDirPath, PATH_MAX, "/data/mira/substitute/%s/", s_TitleId);

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);

        return false;
    }

    // Getting jailed path for the process
    struct filedesc* fd = p->p_fd;

    char* s_SandboxPath = nullptr;
    char* s_Freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &s_Freepath);

    if (s_SandboxPath == nullptr) {
        if (s_Freepath)
            delete (s_Freepath);
        return false;
    }

    // Get ucred and filedesc of current thread
    struct ucred* curthread_cred = curthread->td_proc->p_ucred;
    struct filedesc* curthread_fd = curthread->td_proc->p_fd;

    // Save current cred and fd
    struct ucred orig_curthread_cred = *curthread_cred;
    struct filedesc orig_curthread_fd = *curthread_fd;

    // Set max to current thread cred and fd
    curthread_cred->cr_uid = 0;
    curthread_cred->cr_ruid = 0;
    curthread_cred->cr_rgid = 0;
    curthread_cred->cr_groups[0] = 0;

    curthread_cred->cr_prison = *(struct prison**)kdlsym(prison0);
    curthread_fd->fd_rdir = curthread_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);

    // Mounting substitute folder into the process
    char s_RealSprxFolderPath[PATH_MAX];
    char s_substituteFullMountPath[PATH_MAX];

    snprintf(s_substituteFullMountPath, PATH_MAX, "%s/substitute", s_SandboxPath);
    snprintf(s_RealSprxFolderPath, PATH_MAX, "/data/mira/substitute/%s", s_TitleId);

    // Opening substitute folder
    auto s_DirectoryHandle = kopen_t(s_SprxDirPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        // Restore fd and cred
        *curthread_fd = orig_curthread_fd;
        *curthread_cred = orig_curthread_cred;
        return false;
    }

    WriteLog(LL_Info, "[%s] Substitute start ...", s_TitleId);
    WriteLog(LL_Info, "[%s] Mount path: %s", s_TitleId, s_substituteFullMountPath);
    WriteLog(LL_Info, "[%s] Real path: %s", s_TitleId, s_RealSprxFolderPath);

    // Create folder
    int ret = kmkdir_t(s_substituteFullMountPath, 0511, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "[%s] could not create the directory for mount (%s) (%d).", s_TitleId, s_substituteFullMountPath, ret);

        // Restore fd and cred
        *curthread_fd = orig_curthread_fd;
        *curthread_cred = orig_curthread_cred;
        return false;
    }

    // Mounting
    ret = Utilities::MountNullFS(s_substituteFullMountPath, s_RealSprxFolderPath, MNT_RDONLY);
    if (ret < 0) {
        krmdir_t(s_substituteFullMountPath, s_MainThread);
        WriteLog(LL_Error, "[%s] could not mount folder (%s => %s) (%d).", s_TitleId, s_RealSprxFolderPath, s_substituteFullMountPath, ret);
        return false;
    }

    // Cleanup
    if (s_Freepath)
        delete (s_Freepath);

    s_SandboxPath = nullptr;

    // Closing substitute folder
    kclose_t(s_DirectoryHandle, s_MainThread);

    // Restore fd and cred
    *curthread_fd = orig_curthread_fd;
    *curthread_cred = orig_curthread_cred;
    return true;
}

// Substitute : Unmount Substitute folder
bool Substitute::OnProcessExit(struct proc *p) {
    if (!p)
        return false;

    int ret = 0;

    // Get process information
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    Substitute* substitute = GetPlugin();

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // Check if it's a valid process
    if ( !s_TitleId || s_TitleId[0] == 0 )
        return false;

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
    substitute->CleanupProcessHook(p);

    // Getting jailed path for the process
    struct filedesc* fd = p->p_fd;

    char* s_SandboxPath = nullptr;
    char* s_Freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &s_Freepath);

    if (s_SandboxPath == nullptr) {
        if (s_Freepath)
            delete (s_Freepath);

        return false;
    }

    // Finding substitute folder
    char s_substituteFullMountPath[PATH_MAX];
    snprintf(s_substituteFullMountPath, PATH_MAX, "%s/substitute", s_SandboxPath);

    auto s_DirectoryHandle = kopen_t(s_substituteFullMountPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        return false;
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    WriteLog(LL_Info, "[%s] Cleaning substitute ...", s_TitleId);

    // Unmount substitute folder if the folder exist
    ret = kunmount_t(s_substituteFullMountPath, MNT_FORCE, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not unmount folder (%s) (%d), Trying to remove anyway ...", s_substituteFullMountPath, ret);
    }

    // Remove substitute folder
    ret = krmdir_t(s_substituteFullMountPath, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not remove substitute folder (%s) (%d).", s_substituteFullMountPath, ret);
        return false;
    }

    if (s_Freepath)
        delete (s_Freepath);

    WriteLog(LL_Info, "[%s] Substitute have been cleaned.", s_TitleId);
    return true;
}

// Substitute : Load PRX from Substitute folder
int Substitute::Sys_dynlib_dlsym_hook(struct thread* td, struct dynlib_dlsym_args* uap) {
    if (!td || !uap) {
        WriteLog(LL_Error, "Invalid argument !");
        return EINVAL;
    }

    Substitute* substitute = GetPlugin();
    if (!substitute) {
        WriteLog(LL_Error, "Substitute dependency is needed");
        return 1;
    }

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);
    auto copyinstr = (int(*)(const void *uaddr, void *kaddr, size_t len, size_t *done))kdlsym(copyinstr);
    auto copyout = (int(*)(const void *kaddr, void *uaddr, size_t len))kdlsym(copyout);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);
    auto sys_dynlib_dlsym = (int(*)(struct thread*, void*))substitute->sys_dynlib_dlsym_p;
    if (!sys_dynlib_dlsym)
        return 1;

    // Call original syscall
    int ret = sys_dynlib_dlsym(td, uap);
    int original_td_value = td->td_retval[0];

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (!s_MainThread)
    {
        WriteLog(LL_Error, "Unable to got Mira thread");
        return ret;
    }

    char* s_TitleId = (char*)((uint64_t)td->td_proc + 0x390);

    // Check if it's a valid process
    if ( !s_TitleId || s_TitleId[0] == 0) {
        td->td_retval[0] = original_td_value;
        return ret;
    }

    // Check if substitute folder exist on this process
    struct filedesc* fd = td->td_proc->p_fd;

    char* s_SandboxPath = nullptr;
    char* s_Freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &s_Freepath);

    if (s_SandboxPath == nullptr) {
        if (s_Freepath)
            delete (s_Freepath);

        td->td_retval[0] = original_td_value;
        return ret;
    }

    // Finding substitute folder, if not exist simply doesn't continue the execution flow
    char s_substituteFullMountPath[PATH_MAX];
    snprintf(s_substituteFullMountPath, PATH_MAX, "%s/substitute", s_SandboxPath);

    auto s_DirectoryHandle = kopen_t(s_substituteFullMountPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        kclose_t(s_DirectoryHandle, s_MainThread);
        td->td_retval[0] = original_td_value;
        return ret;
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    char name[50]; // Todo: Find max name character !
    size_t done;
    copyinstr(uap->name, name, sizeof(name), &done);

    // Process search for sysmodule preload symbol, give him a custom function instead
    if (strncmp("sceSysmodulePreloadModuleForLibkernel", name, sizeof(name)) == 0) {
        WriteLog(LL_Info, "[%s] sceSysmodulePreloadModuleForLibkernel trigerred (ret: %d td_retval[0]: %d) !", s_TitleId, ret, td->td_retval[0]);

        // Get the original value
        void* sysmodule_preload_original = substitute->FindOriginalAddress(td->td_proc, "sceSysmodulePreloadModuleForLibkernel", 0);
        WriteLog(LL_Info, "sceSysmodulePreloadModuleForLibkernel: %p", sysmodule_preload_original);

        // Payload [entrypoint_trigger] (The payload is inside the folders is in /src/OrbisOS/asm/, compile with NASM)
        //unsigned char s_Payload[0x150] = "\x4D\x49\x52\x41\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x64\x6F\x53\x75\x62\x73\x74\x69\x74\x75\x74\x65\x4C\x6F\x61\x64\x50\x52\x58\x00\x48\x8B\x05\xD5\xFF\xFF\xFF\xFF\xD0\x57\x56\x52\x50\xBF\x00\x00\x00\x00\x48\x8D\x35\xD3\xFF\xFF\xFF\x48\x8D\x15\xC4\xFF\xFF\xFF\xB8\x4F\x02\x00\x00\x0F\x05\x58\x5A\x5E\x5F\xC3";
        unsigned char s_Payload[0x150] = "\x4D\x49\x52\x41\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x64\x6F\x53\x75\x62\x73\x74\x69\x74\x75\x74\x65\x4C\x6F\x61\x64\x50\x52\x58\x00\x41\x51\x4C\x8B\x0D\xD3\xFF\xFF\xFF\x41\xFF\xD1\x41\x59\x57\x56\x52\x50\xBF\x00\x00\x00\x00\x48\x8D\x35\xCE\xFF\xFF\xFF\x48\x8D\x15\xBF\xFF\xFF\xFF\xB8\x4F\x02\x00\x00\x0F\x05\x58\x5A\x5E\x5F\xC3";

        // Allocate memory on the remote process
        size_t s_PayloadSize = 0x8000; //sizeof(s_Payload) + PATH_MAX but need more for allow the allocation
        auto s_PayloadSpace = kmmap_t(nullptr, s_PayloadSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PREFAULT_READ, -1, 0, td);
        if (s_PayloadSpace == nullptr || s_PayloadSpace == MAP_FAILED || (uint64_t)s_PayloadSpace < 0) {
            WriteLog(LL_Error, "[%s] Unable to allocate remote process memory (%llx size) (ret: %llx)", s_TitleId, s_PayloadSize, s_PayloadSpace);
            td->td_retval[0] = original_td_value;
            return ret;
        }

        // Lock the memory page
        int s_Ret = kmlock_t(s_PayloadSpace, s_PayloadSize, td);
        if (s_Ret < 0) {
            WriteLog(LL_Error, "[%s] Unable to lock the remote process memory (%llx size) (ret: %d)", s_TitleId, s_PayloadSize, s_Ret);
            kmunmap_t(s_PayloadSpace, s_PayloadSize, td);
            td->td_retval[0] = original_td_value;
            return ret;
        }

        // Setup payload
        struct entrypointhook_header* s_PayloadHeader = (struct entrypointhook_header*)s_Payload;
        s_PayloadHeader->epdone = 0;
        s_PayloadHeader->sceSysmodulePreloadModuleForLibkernel = (uint64_t)sysmodule_preload_original;
        s_PayloadHeader->fakeReturnAddress = (uint64_t)s_PayloadSpace;

        // Define entry point
        uint64_t s_PayloadEntrypoint = (uint64_t)(s_PayloadSpace + s_PayloadHeader->entrypoint);

        WriteLog(LL_Info, "s_PayloadSpace: %p", (void*)s_PayloadSpace);
        WriteLog(LL_Info, "s_PayloadEntrypoint: %p", (void*)s_PayloadEntrypoint);

        // Copy payload to process
        size_t s_Size = sizeof(s_Payload);
        s_Ret = proc_rw_mem(td->td_proc, (void*)(s_PayloadSpace), s_Size, s_Payload, &s_Size, true);
        if (s_Ret > 0) {
            WriteLog(LL_Error, "[%s] Unable to write process memory at %p !", s_TitleId, (void*)(s_PayloadSpace));
            kmunmap_t(s_PayloadSpace, s_PayloadSize, td);
            td->td_retval[0] = original_td_value;
            return ret;
        }

        // Setup the new address instead of original
        copyout(&s_PayloadEntrypoint, uap->result, sizeof(uint64_t));
    }

    // Load every custom library !
    if (strncmp("doSubstituteLoadPRX", name, sizeof(name)) == 0) {
        WriteLog(LL_Info, "[%s] doSubstituteLoadPRX trigerred, load prx ...", s_TitleId);
        substitute->LoadAllPrx(td, "/substitute/");
    }

    // Restore td ret value
    td->td_retval[0] = original_td_value;
    return ret;
}

//////////////////////////
// USERLAND IOCTL WRAPPER
//////////////////////////

// Substitute (IOCTL) : Do a IAT Hook for the specified thread
int Substitute::OnIoctl_HookIAT(struct thread* td, struct substitute_hook_iat* uap) {
    int hook_id = -1;
    if (!td || !uap) {
        WriteLog(LL_Error, "Invalid argument !");
        return EINVAL;
    }

    Substitute* substitute = GetPlugin();
    if (!substitute) {
        WriteLog(LL_Error, "Unable to got substitute object !");
        uap->hook_id = -1;
        return 0;
    }

    hook_id = substitute->HookIAT(td->td_proc, uap->chain, uap->module_name, uap->name, uap->flags, uap->hook_function);
    if (hook_id >= 0) {
        WriteLog(LL_Info, "New hook at %i", hook_id);
    } else {
        WriteLog(LL_Error, "Unable to hook %s ! (%i)", uap->name, hook_id);
    }

    uap->hook_id = hook_id;
    return 0;
}

// Substitute (IOCTL) : Do a JMP Hook for the specified thread
int Substitute::OnIoctl_HookJMP(struct thread* td, struct substitute_hook_jmp* uap) {
    int hook_id = -1;
    if (!td || !uap) {
        WriteLog(LL_Error, "Invalid argument !");
        return EINVAL;
    }

    Substitute* substitute = GetPlugin();
    if (!substitute) {
        WriteLog(LL_Error, "Unable to got substitute object !");
        uap->hook_id = hook_id;
        return 0;
    }

    hook_id = substitute->HookJmp(td->td_proc, uap->original_function, uap->hook_function);
    if (hook_id >= 0) {
        WriteLog(LL_Info, "New hook at %i", hook_id);
    } else {
        WriteLog(LL_Error, "Unable to hook %p ! (%i)", uap->original_function, hook_id);
    }

    uap->hook_id = hook_id;
    return 0;
}

// Substitute (IOCTL) : Enable / Disable hook
int Substitute::OnIoctl_StateHook(struct thread* td, struct substitute_state_hook* uap) {
    if (!td || !uap) {
        WriteLog(LL_Error, "Invalid argument !");
        return EINVAL;
    }

    int ret = -1;

    Substitute* substitute = GetPlugin();
    if (!substitute) {
        WriteLog(LL_Error, "Unable to got substitute object !");
        uap->result = ret;
        return 0;
    }

    switch (uap->state) {
        case SUBSTITUTE_STATE_ENABLE: {
            ret = substitute->EnableHook(td->td_proc, uap->hook_id);
            if (ret < 0) {
                WriteLog(LL_Error, "Unable to enable hook %i !", uap->hook_id);
                uap->result = ret;
            } else {
                ret = 1;
                WriteLog(LL_Info, "Hook %i enabled !", uap->hook_id);
                uap->result = ret;
            }

            break;
        }

        case SUBSTITUTE_STATE_DISABLE: {
            ret = substitute->DisableHook(td->td_proc, uap->hook_id);
            if (ret < 0) {
                WriteLog(LL_Error, "Unable to disable hook %i (%d) !", uap->hook_id, ret);
                uap->result = ret;
            } else {
                ret = 1;
                WriteLog(LL_Info, "Hook %i disable !", uap->hook_id);
                uap->result = ret;
            }

            break;
        }

        case SUBSTITUTE_STATE_UNHOOK: {
            ret = substitute->Unhook(td->td_proc, uap->hook_id, uap->chain);
            if (ret < 0) {
                WriteLog(LL_Error, "Unable to unhook %i (%d) !", uap->hook_id, ret);
                uap->result = ret;
            } else {
                ret = 1;
                WriteLog(LL_Info, "Unhook %i was complete !", uap->hook_id);
                uap->result = ret;
            }

            break;
        }

        default: {
            WriteLog(LL_Error, "Invalid state detected.");
            uap->result = ret;
            break;
        }
    }

    return 0;
}

// Substitute (IOCTL) : Main drive (See CtrlDriver in /src/Driver/)
int32_t Substitute::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    switch (p_Command) {
        case SUBSTITUTE_HOOK_IAT: {
            return Substitute::OnIoctl_HookIAT(p_Thread, (struct substitute_hook_iat*)p_Data);
        }

        case SUBSTITUTE_HOOK_JMP: {
            return Substitute::OnIoctl_HookJMP(p_Thread, (struct substitute_hook_jmp*)p_Data);
        }

        case SUBSTITUTE_HOOK_STATE: {
            return Substitute::OnIoctl_StateHook(p_Thread, (struct substitute_state_hook*)p_Data);
        }

        default: {
            WriteLog(LL_Debug, "unknown command: (0x%llx).", p_Command);
            break;
        }
    }

    return 0;
}
