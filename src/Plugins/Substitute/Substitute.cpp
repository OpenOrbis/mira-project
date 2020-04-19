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
};

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

//////////////////////////
// MIRA BASE PLUGIN
//////////////////////////

// Substitute : Constructor
Substitute::Substitute() :
    m_processStartHandler(nullptr),
    m_processEndHandler(nullptr),
    hook_list(nullptr),
    hook_nbr(0)
{
}

// Substitute : Destructor
Substitute::~Substitute()
{
}

// Substitute : Plugin loaded
bool Substitute::OnLoad()
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);

    WriteLog(LL_Info, "Loading Substitute ...");

    m_processStartHandler = EVENTHANDLER_REGISTER(process_exec_end, reinterpret_cast<void*>(OnProcessStart), nullptr, EVENTHANDLER_PRI_ANY);
    m_processEndHandler = EVENTHANDLER_REGISTER(process_exit, reinterpret_cast<void*>(OnProcessExit), nullptr, EVENTHANDLER_PRI_ANY);

    mtx_init(&hook_mtx, "Substitute SPIN Lock", NULL, MTX_SPIN);

    return true;
}

// Substitute : Plugin unloaded
bool Substitute::OnUnload()
{
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
    auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    WriteLog(LL_Error, "Unloading Substitute ...");

    if (m_processStartHandler) {
        EVENTHANDLER_DEREGISTER(process_exec_end, m_processStartHandler);
        m_processStartHandler = nullptr;
    }

    if (m_processEndHandler) {
        EVENTHANDLER_DEREGISTER(process_exit, m_processEndHandler);
        m_processStartHandler = nullptr;
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

// Substitute : Find available hook id for allocation
int Substitute::FindAvailableHookID() {
    int near_up_value = 0;

    if (!hook_list) {
        WriteLog(LL_Error, "The hook list is not initialized, give the first id.");
        return -1;
    }

    for (int i = 0; i < hook_nbr; i++) {
        if (hook_list[i].id > near_up_value) {
            near_up_value = hook_list[i].id;
        }
    }

    return near_up_value + 1;
}

// Substitute : Return hook struct by this id
SubstituteHook* Substitute::GetHookByID(int hook_id) {
    if (!hook_list) {
        WriteLog(LL_Error, "The hook list is not initialized !");
        return nullptr;
    }

    for (int i = 0; i < hook_nbr; i++) {
        if (hook_list[i].id == hook_id) {
            WriteLog(LL_Info, "Hook was found (id: %i ptr: %p)", hook_id, (void*)&hook_list[i]);
            return &hook_list[i];
        }
    }

    WriteLog(LL_Error, "The hook is not found !");
    return nullptr;
}

// Substitute : Allocate a new hook to the list
SubstituteHook* Substitute::AllocateNewHook() {
    WriteLog(LL_Info, "Allocating new hook ...");

    // Create new space
    size_t new_size = sizeof(SubstituteHook) * (hook_nbr + 1);
    SubstituteHook* temp_table = new SubstituteHook[hook_nbr + 1];
    if (!temp_table) {
        WriteLog(LL_Error, "Unable to allocate space ! (new_size = %lu)", new_size);
        return nullptr;
    }

    WriteLog(LL_Info, "New hook list allocated to %p", temp_table);

    // Set to 0
    memset(temp_table, 0, new_size);

    if (hook_list) {
        WriteLog(LL_Info, "Old data existing in %p, copy old data ...", hook_list);

        // Copy old data
        memcpy(temp_table, hook_list, sizeof(SubstituteHook) * hook_nbr);
        
        // free the old space
        delete[] (hook_list);
    } else {
        WriteLog(LL_Info, "No old hook table was detected, skipping ...");
    }

    // Define new value
    hook_list = temp_table;
    hook_nbr++;

    WriteLog(LL_Info, "New data was set. hook_list: %p, hook_nbr: %i", hook_list, hook_nbr);

    // Find available id
    int hookID = FindAvailableHookID();
    if (hookID <= 0) {
        WriteLog(LL_Error, "Unable to find available hook id !");
        return nullptr;
    }
    WriteLog(LL_Info, "Hook id available ! (%i)", hookID);

    SubstituteHook* new_hook = &hook_list[hook_nbr - 1];

    // Define the new hook id for this object
    new_hook->id = hookID;

    WriteLog(LL_Info, "The new hook is allocated at %p", new_hook);

    // Return the new object address
    return new_hook;
}

// Substitute : Free a hook from the list
void Substitute::FreeOldHook(int hook_id) {
    if (hook_id <= 0) {
        WriteLog(LL_Error, "Invalid hook id (%i)", hook_id);
        return;
    }

    if (!hook_list) {
        WriteLog(LL_Error, "The hook list is not initialized !");
        return;
    }

    SubstituteHook* current_hook = GetHookByID(hook_id);

    size_t new_size = sizeof(SubstituteHook) * (hook_nbr - 1);
    if (new_size <= 0) {
        WriteLog(LL_Info, "The hook list will have no data ! Cleaning up ...");
        hook_nbr = 0;
        delete[] (hook_list);
        hook_list = nullptr;
        return;
    }

    // Create new space
    SubstituteHook* temp_table = new SubstituteHook[hook_nbr - 1];
    if (!temp_table) {
        WriteLog(LL_Error, "Unable to allocate space ! (new_size = %lu)", new_size);
        return;
    }

    // Set to 0
    memset(temp_table, 0, new_size);

    // Size calculation
    size_t total_current_size = sizeof(SubstituteHook) * hook_nbr;
    size_t before_size = (size_t)current_hook - (size_t)hook_list;
    size_t after_size = total_current_size - before_size - sizeof(SubstituteHook);

    // Copy all
    memcpy(temp_table, (void*)hook_list, before_size);
    memcpy(temp_table + before_size, (void*)((uint64_t)hook_list + before_size + sizeof(SubstituteHook)), after_size);

    // Delete the old table
    delete (hook_list);

    // Set the actual number
    hook_nbr--;
    hook_list = temp_table;

    WriteLog(LL_Info, "The hook %i have been deleted.", hook_id);

    return;
}

// Substitute : Disable the hook
int Substitute::DisableHook(struct proc* p, int hook_id) {
    if (hook_id <= 0) {
        WriteLog(LL_Error, "Invalid hook id (%i)", hook_id);
        return -1;
    }

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
        WriteLog(LL_Error, "Invalid handle : Another process trying to enable hook.");
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -1;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            WriteLog(LL_Info, "Hook Type: IAT.");

            if (hook->hook_enable && hook->process && hook->original_function) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->jmpslot_address, 8, (void*)&hook->original_function, &write_size, 1);
                if (r_error) {
                    WriteLog(LL_Error, "Unable to write original address: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -1;
                }

                hook->hook_enable = false;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        case HOOKTYPE_JMP : {
            WriteLog(LL_Info, "Hook Type: JMP.");

            if (hook->hook_enable && hook->process && hook->original_function && hook->backupSize > 0 && hook->backupData) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->original_function, hook->backupSize, (void*)hook->backupData, &write_size, 1);
                if (r_error) {
                    WriteLog(LL_Error, "Unable to write original address: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -1;
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
            return -2;
            break;
        }
    }
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Enable the hook
int Substitute::EnableHook(struct proc* p, int hook_id) {
    if (hook_id <= 0) {
        WriteLog(LL_Error, "Invalid hook id (%i)", hook_id);
        return -1;
    }

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
        return -1;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            WriteLog(LL_Info, "Hook Type: IAT.");

            if (!hook->hook_enable && hook->process && hook->hook_function) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->jmpslot_address, 8, (void*)&hook->hook_function, &write_size, 1);
                if (r_error) {
                    WriteLog(LL_Error, "Unable to write the jmp system: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -1;
                }

                hook->hook_enable = true;
            } else {
                WriteLog(LL_Error, "Invalid value was detected.");
            }

            break;
        }

        case HOOKTYPE_JMP : {
            WriteLog(LL_Info, "Hook Type: JMP.");

            if (!hook->hook_enable && hook->process && hook->original_function && hook->hook_function) {

                // Use the jmpBuffer from Hook.cpp
                uint8_t jumpBuffer[] = {
                    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,             // # jmp    QWORD PTR [rip+0x0]
                    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, // # DQ: AbsoluteAddress
                }; // Shit takes 14 bytes

                uint64_t* jumpBufferAddress = (uint64_t*)(jumpBuffer + 6);

                // Assign the address
                *jumpBufferAddress = (uint64_t)hook->hook_function;

                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->original_function, sizeof(jumpBuffer), (void*)jumpBuffer, &write_size, 1);
                if (r_error) {
                    WriteLog(LL_Error, "Unable to write the jmp system: (%i)", r_error);
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return -1;
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
            return -2;
            break;
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Unhook the function
int Substitute::Unhook(struct proc* p, int hook_id) {
    if (hook_id <= 0) {
        WriteLog(LL_Error, "Invalid hook id (%i)", hook_id);
        return -1;
    }

    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    WriteLog(LL_Info, "Unhooking ... (%i)", hook_id);

    if (DisableHook(p, hook_id) != 0)
        return -1;

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);
    FreeOldHook(hook_id);
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Cleanup all hook
void Substitute::CleanupAllHook() {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!hook_list)
        return;

    WriteLog(LL_Info, "Cleaning up all hook ...");


    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    for (int i = 0; i < hook_nbr; i++) {
        FreeOldHook(hook_list[i].id);
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
}

// Substitute : Cleanup hook for a process
void Substitute::CleanupProcessHook(struct proc* p) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    if (!hook_list)
        return;

    WriteLog(LL_Info, "Cleaning up hook for %s", s_TitleId);


    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    for (int i = 0; i < hook_nbr; i++) {
        if (hook_list[i].process == p) {
            FreeOldHook(hook_list[i].id);
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
}

// Substitute : Hook the function in the process (With Import Address Table)
int Substitute::HookIAT(struct proc* p, const char* nids, void* hook_function) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    WriteLog(LL_Info, "Finding jumpslot address ...");

    if (!p || !nids || !hook_function) {
        WriteLog(LL_Error, "One of the parameter is inccorect !");
        return -1;
    }

    // Get the jmpslot offset for this nids
    void* jmpslot_address = (void*)FindOffsetFromNids(p, nids);
    if (!jmpslot_address) {
        WriteLog(LL_Error, "Unable to find the nids address !");
        return -2;
    }

    WriteLog(LL_Info, "The jmpslot address is %p, getting original value ...", jmpslot_address);

    // Get the original value for this jmpslot
    void* original_function = nullptr;
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)jmpslot_address, 8, (void*)&original_function, &read_size, 0);
    if (r_error || !original_function) {
        WriteLog(LL_Error, "Unable to get the original value from the jmpslot ! (%i)", r_error);
        return -3;
    }

    WriteLog(LL_Info, "Allocating new hook ...");

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* new_hook = AllocateNewHook();
    if (!new_hook) {
        WriteLog(LL_Error, "Unable to allocate new hook !", new_hook);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -4;
    }

    WriteLog(LL_Info, "New hook allocated at %p", new_hook);

    // Set default value
    new_hook->process = p;
    new_hook->hook_type = HOOKTYPE_IAT;
    new_hook->hook_function = hook_function;
    new_hook->jmpslot_address = jmpslot_address;
    new_hook->original_function = original_function;
    new_hook->hook_enable = false;

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    WriteLog(LL_Info, "All data is set. The hook id is %i", new_hook->id);

    return new_hook->id;
}

// Substitute : Hook the function in the process (With longjmp)
int Substitute::HookJmp(struct proc* p, void* original_address, void* hook_function) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p || !original_address || !hook_function)
        return -1;

    WriteLog(LL_Info, "Getting minimum hook size ...");

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

    WriteLog(LL_Info, "Allocating backup buffer ... (size: %i)", backupSize);

    // Malloc data for the backup data
    char* backupData = new char[backupSize];
    if (!backupData) {
        WriteLog(LL_Error, "Unable to allocate memory for backup");
        return -4;
    }

    WriteLog(LL_Info, "backupData: %p", backupData);

    WriteLog(LL_Info, "Getting backup data ... (size: %i)", backupSize);

    // Get the backup data
    read_size = 0;
    r_error = proc_rw_mem(p, (void*)original_address, backupSize, (void*)backupData, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Info, "Unable to get backupData (%d)", r_error);
        delete[] (backupData);
        return -5;
    }

    WriteLog(LL_Info, "Allocating new hook ...");

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* new_hook = AllocateNewHook();
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

    WriteLog(LL_Info, "All data is set. The hook id is %i", new_hook->id);

    return new_hook->id;
}

//////////////////////////
// IAT HOOK UTILITY
//////////////////////////

// Substitute : Print debug information from import table
void Substitute::DebugImportTable(struct proc* p)
{
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    if (p->p_dynlib) {
        WriteLog(LL_Info, "[%s] Address of p_dynlib: %p", s_TitleId, p->p_dynlib);
        uint64_t main_dylib_obj = *(uint64_t*)((uint64_t)p->p_dynlib + 0x10);

        if (main_dylib_obj) {
            WriteLog(LL_Info, "[%s] Starting scan object ... (Main at %p)", s_TitleId, (void*)main_dylib_obj);

            int total = 0;
            uint64_t dynlib_obj = main_dylib_obj;
            for (;;) {
                total++;

                char* name = (char*)(*(uint64_t*)(dynlib_obj + 8));
                void* relocbase = (void*)(*(uint64_t*)(dynlib_obj + 0x70));
                WriteLog(LL_Info, "dynlib_obj(%i): %p name: %s relocbase: %p", total, (void*)dynlib_obj, name, relocbase);

                dynlib_obj = *(uint64_t*)(dynlib_obj);
                if (!dynlib_obj)
                    break;
            }

            WriteLog(LL_Info, "[%s] Scan done. (Total: %i)", s_TitleId, total);

            WriteLog(LL_Info, "[%s] Loading PLT table for the main dynlib object ...", s_TitleId);
            uint64_t unk_obj = *(uint64_t*)(main_dylib_obj + 0x150);
            if (unk_obj) {
                uint64_t string_table = *(uint64_t*)(unk_obj + 0x38);

                uint64_t unk_obj_size_in_unk_obj = *(uint64_t*)(unk_obj + 0x50);
                uint64_t unk_obj_in_obj = *(uint64_t*)(unk_obj + 0x48);

                WriteLog(LL_Error, "[%s] string_table: %p", s_TitleId, (void*)string_table);
                WriteLog(LL_Error, "[%s] unk_obj_in_obj: %p", s_TitleId, (void*)unk_obj_in_obj);
                WriteLog(LL_Error, "[%s] unk_obj_size_in_unk_obj: %p", s_TitleId, (void*)unk_obj_size_in_unk_obj);

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

                            uint64_t r_offset = *(uint64_t*)(current);
                            uint64_t r_info = *(uint64_t*)(current + 0x8);

                            WriteLog(LL_Info, "[%s] r_offset: %p r_info: %p", s_TitleId, (void*)r_offset, (void*)r_info);

                            char* nids = (char*)nids_offset;
                            if (nids) {
                                WriteLog(LL_Info, "[%s] nids: %-25s (%p)", s_TitleId, nids, (void*)nids);
                            } else {
                                WriteLog(LL_Error, "[%s] nids: UNKNOWN", s_TitleId);
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
    } else {
        WriteLog(LL_Error, "[%s] The process is not Dynamic Linkable", s_TitleId);
    }
}

// Substitute : Find pre-offset from the nids
uint64_t Substitute::FindOffsetFromNids(struct proc* p, const char* nids_to_find) {
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    // Get the start text address of my process
    uint64_t s_TextStart = 0;
    ProcVmMapEntry* s_Entries = nullptr;
    size_t s_NumEntries = 0;
    auto s_Ret = Utilities::GetProcessVmMap(p, &s_Entries, &s_NumEntries);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "[%s] Could not get the VM Map.", s_TitleId);
        return 0;
    }

    if (s_Entries == nullptr || s_NumEntries == 0)
    {
        WriteLog(LL_Error, "[%s] Invalid entries (%p) or numEntries (%d)", s_TitleId, s_Entries, s_NumEntries);
        return 0;
    }

    for (auto i = 0; i < s_NumEntries; ++i)
    {
        if (s_Entries[i].prot == (PROT_READ | PROT_EXEC))
        {
            s_TextStart = (uint64_t)s_Entries[i].start;
            break;
        }
    }

    if (s_TextStart == 0)
    {
        WriteLog(LL_Error, "[%s] Could not find text start for this process !", s_TitleId);
        return 0;
    } else {
        WriteLog(LL_Info, "[%s] text pointer: %p !", s_TitleId, s_TextStart);
    }

    // Free the s_Entries
    delete [] s_Entries;
    s_Entries = nullptr;

    uint64_t nids_offset_found = 0;

    if (p->p_dynlib) {
        uint64_t main_dylib_obj = *(uint64_t*)((uint64_t)p->p_dynlib + 0x10);

        if (main_dylib_obj) {
            uint64_t unk_obj = *(uint64_t*)(main_dylib_obj + 0x150);
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

                            uint64_t r_offset = *(uint64_t*)(current);

                            char* nids = (char*)nids_offset;
                            if (nids) {
                                if (strncmp(nids, nids_to_find, 11) == 0) {
                                    nids_offset_found = r_offset;
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
    } else {
        WriteLog(LL_Error, "[%s] The process is not Dynamic Linkable", s_TitleId);
    }

    if (nids_offset_found > 0) {
        nids_offset_found += s_TextStart;
    }

    return nids_offset_found;
}

//////////////////////////
// PROCESS MANAGEMENT
//////////////////////////

// Substitute : PRX Loader
void Substitute::OnProcessStart(void *arg, struct proc *p)
{
    /*
    Substitute* substitute = GetPlugin();

    // Debug Hook IAT
    void* jmpslot_address = (void*)substitute->FindOffsetFromNids(p, "gQX+4GDQjpM");
    if (!jmpslot_address) {
        WriteLog(LL_Error, "Unable to find jmpslot address !");
    }

    void* original_function = nullptr;
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)jmpslot_address, 8, (void*)&original_function, &read_size, 0);
    if (r_error) {
        WriteLog(LL_Error, "Unable to get the original value from the jmpslot ! (%i)", r_error);
    } else {
        WriteLog(LL_Info, "Trying Hooking ...");
        int hook_id = substitute->HookIAT(p, "gQX+4GDQjpM", original_function);
        if (hook_id > 0) {
            WriteLog(LL_Info, "New hook at %i", hook_id);
            substitute->EnableHook(hook_id);
        } else {
            WriteLog(LL_Error, "Unable to hook ! (%i)", hook_id);
        }
    }
    */

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

    char s_SprxDirPath[255];
    snprintf(s_SprxDirPath, 255, "/data/mira/substitute/%s/", s_TitleId);

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
    char* freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &freepath);

    if (s_SandboxPath == nullptr) {
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

    // Reading folder and search for sprx ...
    uint64_t s_DentCount = 0;
    char s_Buffer[0x1000] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));
    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), s_MainThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);
            s_DentCount++;

            // Check if the sprx is legit
            if (strstr(l_Dent->d_name, ".sprx")) {
                char s_RelativeSprxPath[255];
                snprintf(s_RelativeSprxPath, 255, "/substitute/%s", l_Dent->d_name);

                // Create relative path for the load
                WriteLog(LL_Info, "[%s] Trying to load  %s ...", s_TitleId, s_RelativeSprxPath);

                // Load the SPRX
                int moduleID = 0;
                int ret = kdynlib_load_prx_t(s_RelativeSprxPath, 0, NULL, 0, NULL, &moduleID, s_ProcessThread);

                WriteLog(LL_Info, "kdynlib_load_prx_t: ret: %i (0x%08x) moduleID: %i (0x%08x)", ret, ret, moduleID, moduleID);
            }

            l_Pos += l_Dent->d_reclen;
        }
    }

    // Closing substrate folder
    kclose_t(s_DirectoryHandle, s_MainThread);
}

// Substitute : Unmount substitute folder
void Substitute::OnProcessExit(void *arg, struct proc *p) {
    Substitute* substitute = GetPlugin();

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // Start by cleanup hook list
    substitute->CleanupProcessHook(p);

    // Get process information
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);

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
    char* freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &freepath);

    if (s_SandboxPath == nullptr) {
        return;
    }

    // Finding substitute folder
    char s_substituteFullMountPath[255];
    snprintf(s_substituteFullMountPath, 255, "%s/substitute", s_SandboxPath);

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

    WriteLog(LL_Info, "[%s] Substitute have been cleaned.", s_TitleId);
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
        uap->hook_id = hook_id;
        return 0;
    }

    hook_id = substitute->HookIAT(td->td_proc, uap->nids, uap->hook_function);
    if (hook_id > 0) {
        WriteLog(LL_Info, "New hook at %i", hook_id);
    } else {
        WriteLog(LL_Error, "Unable to hook %s ! (%i)", uap->nids, hook_id);
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
    if (hook_id > 0) {
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
        case 0: {
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

        case 1: {
            ret = substitute->DisableHook(td->td_proc, uap->hook_id);
            if (ret < 0) {
                WriteLog(LL_Error, "Unable to disable hook %i !", uap->hook_id);
                uap->result = ret;
            } else {
                ret = 1;
                WriteLog(LL_Info, "Hook %i disable !", uap->hook_id);
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
        }
    }

    return 0;
}