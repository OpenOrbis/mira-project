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
Substitute::Substitute() :
    m_processStartHandler(nullptr),
    m_processEndHandler(nullptr)
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
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);

    WriteLog(LL_Info, "Loading Substitute ...");
 
    // Substitute mount / unmount
    m_processStartHandler = EVENTHANDLER_REGISTER(process_exec_end, reinterpret_cast<void*>(OnProcessStart), nullptr, EVENTHANDLER_PRI_ANY);
    m_processEndHandler = EVENTHANDLER_REGISTER(process_exit, reinterpret_cast<void*>(OnProcessExit), nullptr, EVENTHANDLER_PRI_ANY);

    // Substitute syscall hook (PRX Loader)
    sys_dynlib_load_prx_p = (void*)sysents[SYS_DYNLIB_LOAD_PRX].sy_call;
    sysents[SYS_DYNLIB_LOAD_PRX].sy_call = (sy_call_t*)Sys_dynlib_load_prx_hook;

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
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
    auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    WriteLog(LL_Error, "Unloading Substitute ...");

    // Cleanup substitute mount / unmount
    if (m_processStartHandler) {
        EVENTHANDLER_DEREGISTER(process_exec_end, m_processStartHandler);
        m_processStartHandler = nullptr;
    }

    if (m_processEndHandler) {
        EVENTHANDLER_DEREGISTER(process_exit, m_processEndHandler);
        m_processStartHandler = nullptr;
    }

    // Cleanup substitute hook (PRX Loader)
    if (sys_dynlib_load_prx_p) {
        sysents[SYS_DYNLIB_LOAD_PRX].sy_call = (sy_call_t*)sys_dynlib_load_prx_p;
        sys_dynlib_load_prx_p = nullptr;
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

//////////////////////////
// HOOK MANAGEMENT SYSTEM
//////////////////////////

// Substitute : Check if the process is Alive
bool Substitute::IsProcessAlive(struct proc* p_alive) {
    struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);

    struct proc* p = NULL;
    FOREACH_PROC_IN_SYSTEM(p)
    {
        if (p == p_alive) {
            return true;
        }
    }

    return false;
}

// Substitute : Return hook struct by this id
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

// Substitute : Allocate a new hook to the list
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

// Substitute : Free a hook from the list
void Substitute::FreeHook(int hook_id) {
    SubstituteHook* hook = GetHookByID(hook_id);

    if (hook) {
        memset(hook, 0, sizeof(SubstituteHook));
        WriteLog(LL_Info, "The hook %i have been deleted.", hook_id);
    }

    return;
}

// Substitute : Disable the hook
int Substitute::DisableHook(struct proc* p, int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

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

    /*
    if (!IsProcessAlive(hook->process)) {
        WriteLog(LL_Error, "The process is not alive !");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -3;
    }
    */

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            if (hook->hook_enable && hook->process && hook->original_function) {
                int r_error = proc_rw_mem(hook->process, hook->jmpslot_address, sizeof(uint64_t), &hook->original_function, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write original address: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -4;
                }

                hook->hook_enable = false;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        case HOOKTYPE_JMP : {
            if (hook->hook_enable && hook->process && hook->original_function && hook->backupSize > 0 && hook->backupData) {
                int r_error = proc_rw_mem(hook->process, hook->original_function, hook->backupSize, hook->backupData, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write original address: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -5;
                }

                hook->hook_enable = false;
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

    return 0;
}

// Substitute : Enable the hook
int Substitute::EnableHook(struct proc* p, int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
  
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

    /*
    if (!IsProcessAlive(hook->process)) {
        WriteLog(LL_Error, "The process is not alive !");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -3;
    }
    */

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            if (!hook->hook_enable && hook->process && hook->hook_function) {
                int r_error = proc_rw_mem(hook->process, hook->jmpslot_address, sizeof(uint64_t), &hook->hook_function, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write the iat system: (%d)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -4;
                }

                hook->hook_enable = true;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        case HOOKTYPE_JMP : {
            if (!hook->hook_enable && hook->process && hook->original_function && hook->hook_function) {

                // Use the jmpBuffer from Hook.cpp
                uint8_t jumpBuffer[] = {
                    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,             // # jmp    QWORD PTR [rip+0x0]
                    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, // # DQ: AbsoluteAddress
                }; // Shit takes 14 bytes

                uint64_t* jumpBufferAddress = (uint64_t*)(jumpBuffer + 6);

                // Assign the address
                *jumpBufferAddress = (uint64_t)hook->hook_function;

                int r_error = proc_rw_mem(hook->process, hook->original_function, sizeof(jumpBuffer), jumpBuffer, nullptr, true);
                if (r_error != 0) {
                    WriteLog(LL_Error, "Unable to write the jmp system: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -5;
                }

                hook->hook_enable = true;
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

    return 0;
}

// Substitute : Unhook the function
int Substitute::Unhook(struct proc* p, int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);
    FreeHook(hook_id);
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "%i is now unhooked.", hook_id);

    return 0;
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

// Substitute : Hook the function in the process (With Import Address Table)
int Substitute::HookIAT(struct proc* p, const char* module_name, const char* name, int32_t flags, void* hook_function, uint64_t* original_function_out) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p || !name || !hook_function) {
        WriteLog(LL_Error, "One of the parameter is incorrect !");
        return -1;
    }

    // Get the jmpslot offset for this nids
    void* jmpslot_address = (void*)FindJmpslotAddress(p, module_name, name, flags);
    if (!jmpslot_address) {
        WriteLog(LL_Error, "Unable to find the jmpslot address !");
        return -2;
    }

    // Get the original value for this jmpslot
    void* original_function = FindOriginalAddress(p, name, flags);
    if (!original_function) {
        WriteLog(LL_Error, "Unable to get the original value from the jmpslot !");
        return -3;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    int hook_id = -5;
    SubstituteHook* new_hook = AllocateHook(&hook_id);
    if (!new_hook) {
        WriteLog(LL_Error, "Unable to allocate new hook !", new_hook);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -4;
    }

    // Set default value
    new_hook->process = p;
    new_hook->hook_type = HOOKTYPE_IAT;
    new_hook->hook_function = hook_function;
    new_hook->jmpslot_address = jmpslot_address;
    new_hook->original_function = original_function;
    new_hook->hook_enable = false;

    if (original_function_out) {
        *original_function_out = (uint64_t)original_function;
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
        return -1;

    // Get buffer from original function for calculate size
    char buffer[500];
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)original_address, sizeof(buffer), (void*)buffer, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Error, "Unable to get the buffer !");
        return -2;
    }

    // Set the original address
    int backupSize = Utils::Hook::GetMinimumHookSize(buffer);
    if (backupSize <= 0) {
        WriteLog(LL_Error, "Unable to get the minimum hook size.");
        return -3;
    }

    // Malloc data for the backup data
    char* backupData = new char[backupSize];
    if (!backupData) {
        WriteLog(LL_Error, "Unable to allocate memory for backup");
        return -4;
    }

    // Get the backup data
    read_size = 0;
    r_error = proc_rw_mem(p, (void*)original_address, backupSize, (void*)backupData, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Info, "Unable to get backupData (%d)", r_error);
        delete[] (backupData);
        return -5;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    int hook_id = -7;
    SubstituteHook* new_hook = AllocateHook(&hook_id);
    if (!new_hook) {
        WriteLog(LL_Error, "Unable to allocate new hook !", new_hook);
        delete[] (backupData);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -6;
    }

    // Set default value
    new_hook->process = p;
    new_hook->hook_type = HOOKTYPE_JMP;
    new_hook->hook_function = hook_function;
    new_hook->original_function = original_address;
    new_hook->backupData = backupData;
    new_hook->backupSize = backupSize;
    new_hook->hook_enable = false;

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "New hook is created (JMP) :  %i", hook_id);

    return hook_id;
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

    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    void* addr = nullptr;

    if (p->p_dynlib) {
        // Lock dynlib object
        struct sx* dynlib_bind_lock = (struct sx*)((uint64_t)p->p_dynlib + 0x70);
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        uint64_t main_dylib_obj = *(uint64_t*)((uint64_t)p->p_dynlib + 0x10);

        if (main_dylib_obj) {
            // Search in all library
            int total = 0;
            uint64_t dynlib_obj = main_dylib_obj;
            for (;;) {
                total++;

                /*
                char* lib_name = (char*)(*(uint64_t*)(dynlib_obj + 8));
                void* relocbase = (void*)(*(uint64_t*)(dynlib_obj + 0x70));
                uint64_t handle = *(uint64_t*)(dynlib_obj + 0x28);
                */

                //WriteLog(LL_Info, "[%s] search(%i): %p lib_name: %s handle: 0x%lx relocbase: %p ...", s_TitleId, total, (void*)dynlib_obj, lib_name, handle, relocbase);

                // Doing a dlsym with nids or name
                if ( (flags & SUBSTITUTE_IAT_NIDS) ) {
                    addr = dynlib_do_dlsym((void*)p->p_dynlib, (void*)dynlib_obj, name, NULL, 0x1); // name = nids
                } else {
                    addr = dynlib_do_dlsym((void*)p->p_dynlib, (void*)dynlib_obj, name, NULL, 0x0); // use name (dynlib_do_dlsym will calculate later)
                }

                if (addr) {
                    break;
                }

                dynlib_obj = *(uint64_t*)(dynlib_obj);
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

    return addr;
}

// Substitute : Find pre-offset from the name or nids
uint64_t Substitute::FindJmpslotAddress(struct proc* p, const char* module_name, const char* name, int32_t flags) {
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto name_to_nids = (void(*)(const char *name, const char *nids_out))kdlsym(name_to_nids);

    if (!name || !p) {
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

    if (p->p_dynlib) {
        // Lock dynlib object (Note: Locking will panic kernel sometime)
        struct sx* dynlib_bind_lock = (struct sx*)((uint64_t)p->p_dynlib + 0x70);
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        uint64_t main_dylib_obj = *(uint64_t*)((uint64_t)p->p_dynlib + 0x10);

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

//////////////////////////
// PROCESS MANAGEMENT
//////////////////////////

// Substitute : Mount Substitute folder
void Substitute::OnProcessStart(void *arg, struct proc *p)
{
    if (!p)
        return;

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);

    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    WriteLog(LL_Debug, "process start (%s) (%d).", s_TitleId, p->p_pid);

    // Check if it's a valid process
    if ( !(strstr(s_TitleId, "CUSA") || strstr(s_TitleId, "NPXS")) )
        return;

    char s_SprxDirPath[PATH_MAX];
    snprintf(s_SprxDirPath, PATH_MAX, "/data/mira/substitute/%s/", s_TitleId);

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);

        return;
    }

    // Getting jailed path for the process
    struct filedesc* fd = p->p_fd;

    char* s_SandboxPath = nullptr;
    char* s_Freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &s_Freepath);

    if (s_SandboxPath == nullptr) {
        if (s_Freepath)
            delete (s_Freepath);
        return;
    }

    // Mounting substitute folder into the process
    char s_RealSprxFolderPath[PATH_MAX];
    char s_substituteFullMountPath[PATH_MAX];

    snprintf(s_substituteFullMountPath, PATH_MAX, "%s/substitute", s_SandboxPath);
    snprintf(s_RealSprxFolderPath, PATH_MAX, "/data/mira/substitute/%s", s_TitleId);

    // Opening substitute folder
    auto s_DirectoryHandle = kopen_t(s_SprxDirPath, 0x0000 | 0x00020000, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        return;
    }

    WriteLog(LL_Info, "[%s] Substitute start ...", s_TitleId);

    // Create folder
    int ret = kmkdir_t(s_substituteFullMountPath, 0511, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "[%s] could not create the directory for mount (%s) (%d).", s_TitleId, s_substituteFullMountPath, ret);
        return;
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

    // Mounting
    ret = Utilities::MountNullFS(s_substituteFullMountPath, s_RealSprxFolderPath, MNT_RDONLY);
    if (ret >= 0) {
        WriteLog(LL_Info, "[%s] mount folder success (%s => %s) (%d).", s_TitleId, s_RealSprxFolderPath, s_substituteFullMountPath, ret);
    } else {
        krmdir_t(s_substituteFullMountPath, s_MainThread);
        WriteLog(LL_Error, "[%s] could not mount folder (%s => %s) (%d).", s_TitleId, s_RealSprxFolderPath, s_substituteFullMountPath, ret);
        return;
    }

    // Restore fd and cred
    *curthread_fd = orig_curthread_fd;
    *curthread_cred = orig_curthread_cred;

    // Cleanup
    if (s_Freepath)
        delete (s_Freepath);

    s_SandboxPath = nullptr;

    // Closing substitute folder
    kclose_t(s_DirectoryHandle, s_MainThread);
}

// Substitute : Unmount Substitute folder
void Substitute::OnProcessExit(void *arg, struct proc *p) {
    if (!p)
        return;

    // Get process information
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    Substitute* substitute = GetPlugin();

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // If it's a compatible application
    if ( !(strstr(s_TitleId, "CUSA") || strstr(s_TitleId, "NPXS")) )
        return;

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);
        return;
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

        return;
    }

    // Finding substitute folder
    char s_substituteFullMountPath[PATH_MAX];
    snprintf(s_substituteFullMountPath, PATH_MAX, "%s/substitute", s_SandboxPath);

    auto s_DirectoryHandle = kopen_t(s_substituteFullMountPath, 0x0000 | 0x00020000, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        return;
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    WriteLog(LL_Info, "[%s] Cleaning substitute ...", s_TitleId);

    // Unmount substitute folder if the folder exist
    int ret = kunmount_t(s_substituteFullMountPath, MNT_FORCE, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not unmount folder (%s) (%d), Trying to remove anyway ...", s_substituteFullMountPath, ret);
    }

    // Remove substitute folder
    ret = krmdir_t(s_substituteFullMountPath, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not remove substitute folder (%s) (%d).", s_substituteFullMountPath, ret);
        return;
    }

    if (s_Freepath)
        delete (s_Freepath);

    WriteLog(LL_Info, "[%s] Substitute have been cleaned.", s_TitleId);
}

// Substitute : Load PRX from Substitute folder
int Substitute::Sys_dynlib_load_prx_hook(struct thread* td, struct dynlib_load_prx_args_ex* uap) {
    Substitute* substitute = GetPlugin();

    char* s_TitleId = (char*)((uint64_t)td->td_proc + 0x390);

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto copyinstr = (int(*)(const void *uaddr, void *kaddr, size_t len, size_t *done))kdlsym(copyinstr);

    auto sys_dynlib_load_prx = (int(*)(struct thread*, struct dynlib_load_prx_args_ex*))substitute->sys_dynlib_load_prx_p;

    // Call original syscall
    int ret = sys_dynlib_load_prx(td, uap);

    // Save current td ret value
    int original_td_value = td->td_retval[0];

    // Get the current path of the loaded library
    char path[PATH_MAX];
    memset(path, 0, sizeof(path));
    size_t done = 0;

    if (!uap->path) {
        return ret;
    }

    copyinstr((void*)uap->path, (void*)path, PATH_MAX, &done);

    // If libc is loaded
    if (strstr(path, "libc.prx") || strstr(path, "libc.sprx")) {

        // Opening substitute folder
        auto s_DirectoryHandle = kopen_t("/substitute/", 0x0000 | 0x00020000, 0777, td);
        if (s_DirectoryHandle < 0)
        {
            // Restore td ret value
            td->td_retval[0] = original_td_value;
            return ret;
        }

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
                    snprintf(s_RelativeSprxPath, PATH_MAX, "/substitute/%s", l_Dent->d_name);

                    // Create relative path for the load
                    WriteLog(LL_Info, "[%s] Loading  %s ...", s_TitleId, s_RelativeSprxPath);

                    Utilities::LoadPRXModule(td->td_proc, s_RelativeSprxPath);
                }

                l_Pos += l_Dent->d_reclen;
            }
        }

        // Closing substitute folder
        kclose_t(s_DirectoryHandle, td);
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

    uint64_t original_function = 0;
    hook_id = substitute->HookIAT(td->td_proc, uap->module_name, uap->name, uap->flags, uap->hook_function, &original_function);
    if (hook_id >= 0) {
        WriteLog(LL_Info, "New hook at %i", hook_id);
    } else {
        WriteLog(LL_Error, "Unable to hook %s ! (%i)", uap->name, hook_id);
    }

    if (original_function <= 0) {
        WriteLog(LL_Error, "Something weird have appened.");
        uap->hook_id = -1;
        return 0;
    }

    uap->hook_id = hook_id;
    uap->original_function = (void*)original_function;
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
            ret = substitute->Unhook(td->td_proc, uap->hook_id);
            if (ret < 0) {
                WriteLog(LL_Error, "Unable to unhook %i (%d) !", uap->hook_id, ret);  
                uap->result = ret;
            } else {
                ret = 1;
                WriteLog(LL_Info, "Unhook %i was complete !", uap->hook_id);
                uap->result = ret;
            }
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
