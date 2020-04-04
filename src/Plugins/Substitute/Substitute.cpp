#include "Substitute.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

extern "C"
{
    #include <sys/dirent.h>
    #include <sys/stat.h>
    #include <sys/socket.h>
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
    #include <external/hde64/hde64.h>
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

    mtx_init(&hook_mtx, "Substitute", NULL, 0);

    return true;
}

// Substitute : Plugin unloaded
bool Substitute::OnUnload()
{
    WriteLog(LL_Error, "Unloading Substitute ...");
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

    if (!hook_list)
        return 0;

    for (int i = 0; i < hook_nbr; i++) {
        if (hook_list[i].id > near_up_value) {
            near_up_value = hook_list[i].id;
        }
    }

    return near_up_value++;
}

// Substitute : Return hook struct by this id
SubstituteHook* Substitute::GetHookByID(int hook_id) {
    if (!hook_list)
        return nullptr;

    for (int i = 0; i < hook_nbr; i++) {
        if (hook_list[i].id == hook_id) {
            return &hook_list[i];
        }
    }

    return nullptr;
}

// Substitute : Allocate a new hook to the list
SubstituteHook* Substitute::AllocateNewHook() {
    auto malloc = (void*(*)(unsigned long size, struct malloc_type* type, int flags))kdlsym(malloc);
    auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    // Find available id
    int hookID = FindAvailableHookID();
    if (hookID <= 0) {
        return nullptr;
    }

    // Create new space
    size_t new_size = sizeof(SubstituteHook) * (hook_nbr + 1);
    SubstituteHook* temp_table = (SubstituteHook*)malloc(new_size, M_TEMP, M_ZERO | M_WAITOK);
    if (!temp_table) {
        return nullptr;
    }

    // Set to 0
    memset(temp_table, 0, new_size);

    if (hook_list) {
        // Copy old data
        memcpy(temp_table, hook_list, sizeof(SubstituteHook) * hook_nbr);
        
        // free the old space
        delete(hook_list);
    }

    // Define new address
    hook_list = temp_table;

    // Add new number of hook available
    hook_nbr++;

    // Define the new hook id for this object
    hook_list[hook_nbr--].id = hookID;

    // Return the new object address
    return &hook_list[hook_nbr--];
}

// Substitute : Free a hook from the list
void Substitute::FreeOldHook(int hook_id) {
    auto malloc = (void*(*)(unsigned long size, struct malloc_type* type, int flags))kdlsym(malloc);
    auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    if (!hook_list) {
        return;
    }

    SubstituteHook* current_hook = GetHookByID(hook_id);

    size_t new_size = sizeof(SubstituteHook) * (hook_nbr - 1);
    if (new_size <= 0) {
        hook_nbr = 0;
        delete (hook_list);
        hook_list = nullptr;
        return;
    }

    // Create new space
    SubstituteHook* temp_table = (SubstituteHook*)malloc(new_size, M_TEMP, M_ZERO | M_WAITOK);
    if (!temp_table) {
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

    return;
}

// Substitute : Disable the hook
int Substitute::DisableHook(int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
  
    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* hook = GetHookByID(hook_id);
    if (!hook) {
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return 1;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            if (hook->hook_enable && hook->process && hook->original_function) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->jmpslot_address, 8, (void*)&hook->original_function, &write_size, 1);
                if (r_error) {
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return 1;
                }

                hook->hook_enable = false;
            }
        }

        case HOOKTYPE_JMP : {
            if (hook->hook_enable && hook->process && hook->original_function && hook->backupSize > 0 && hook->backupData) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->original_function, hook->backupSize, (void*)hook->backupData, &write_size, 1);
                if (r_error) {
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return 1;
                }

                hook->hook_enable = false;
            }

            break;
        }

        default: {
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return 2;
        }
    }
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Enable the hook
int Substitute::EnableHook(int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
  
    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* hook = GetHookByID(hook_id);
    if (!hook) {
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return 1;
    }

    switch (hook->hook_type) {
        case HOOKTYPE_IAT: {
            if (!hook->hook_enable && hook->process && hook->hook_function) {
                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->jmpslot_address, 8, (void*)&hook->hook_function, &write_size, 1);
                if (r_error) {
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return 1;
                }

                hook->hook_enable = true;
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


                size_t write_size = 0;
                int r_error = proc_rw_mem(hook->process, (void*)hook->original_function, sizeof(jumpBuffer), (void*)jumpBuffer, &write_size, 1);
                if (r_error) {
                    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
                    return 1;
                }

                hook->hook_enable = true;
            }

            break;
        }

        default: {
            _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
            return 0;
        }
    }

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Unhook the function
int Substitute::Unhook(int hook_id) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    DisableHook(hook_id);

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);
    FreeOldHook(hook_id);
    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return 0;
}

// Substitute : Cleanup hook for a process
void Substitute::CleanupProcessHook(struct proc* p) {
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!hook_list)
        return;

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

    if (!p || !nids || !hook_function)
        return -1;

    // Get the jmpslot offset for this nids
    void* jmpslot_address = (void*)FindOffsetFromNids(p, nids);
    if (!jmpslot_address)
        return -2;

    // Get the original value for this jmpslot
    void* original_function = nullptr;
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)jmpslot_address, 8, (void*)&original_function, &read_size, 0);
    if (r_error || !original_function) {
        return -3;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* new_hook = AllocateNewHook();
    if (!new_hook) {
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

    _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    return new_hook->id;
}

// Substitute : Hook the function in the process (With longjmp)
int Substitute::HookJmp(struct proc* p, void* original_address, void* hook_function) {
    auto malloc = (void*(*)(unsigned long size, struct malloc_type* type, int flags))kdlsym(malloc);
    auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (!p || !original_address || !hook_function)
        return -1;

    // Set the original address
    int backupSize = GetMinimumHookSize(p, original_address);
    if (backupSize <= 0) {
        return -2;
    }

    // Malloc data for the backup data
    char* backupData = (char*) malloc(backupSize, M_TEMP, M_ZERO | M_WAITOK);
    if (!backupData) {
        return -3;
    }

    // Get the backup data
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)original_address, backupSize, (void*)backupData, &read_size, 0);
    if (r_error) {
        delete (backupData);
        return -4;
    }

    _mtx_lock_flags(&hook_mtx, 0, __FILE__, __LINE__);

    SubstituteHook* new_hook = AllocateNewHook();
    if (!new_hook) {
        delete (backupData);
        _mtx_unlock_flags(&hook_mtx, 0, __FILE__, __LINE__);
        return -5;
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
// LONGJMP HOOK UTILITY
//////////////////////////

// Substitute : Get minimum hook size for fit to the trampoline for a process
int Substitute::GetMinimumHookSize(struct proc* p, void* p_Target)
{
    hde64s hs;

    uint32_t hookSize = 14;
    uint32_t totalLength = 0;

    char buffer[500];
    size_t read_size = 0;
    int r_error = proc_rw_mem(p, (void*)p_Target, sizeof(buffer), (void*)buffer, &read_size, 0);
    if (r_error) {
        return -1;
    }
    
    while (totalLength < hookSize)
    {
        uint32_t length = hde64_disasm(p_Target, &hs);
        if (hs.flags & F_ERROR)
            return -1;

        totalLength += length;
    }

    return totalLength;
}

//////////////////////////
// PROCESS MANAGEMENT
//////////////////////////

// Substitute : PRX Loader
void Substitute::OnProcessStart(void *arg, struct proc *p)
{
    Substitute* substitute = GetPlugin();

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

    uint64_t printf_offset = substitute->FindOffsetFromNids(p, "hcuQgD53UxM"); // The function is heavy, don't static it
    WriteLog(LL_Info, "[%s] The printf function jmpslot is located at: %p", s_TitleId, printf_offset);

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

    // Mounting
    ret = Utilities::MountNullFS(s_substituteFullMountPath, s_RealSprxFolderPath, MNT_RDONLY, s_MainThread);
    if (ret >= 0) {
        WriteLog(LL_Info, "mount folder success (%s => %s) (%d).", s_RealSprxFolderPath, s_substituteFullMountPath, ret);
    } else {
        krmdir_t(s_substituteFullMountPath, s_MainThread);
        WriteLog(LL_Error, "could not mount folder (%s => %s) (%d).", s_RealSprxFolderPath, s_substituteFullMountPath, ret);
        return;
    }

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
