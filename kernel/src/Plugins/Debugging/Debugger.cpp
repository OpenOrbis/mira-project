// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Debugger.hpp"
#include <Utils/Kdlsym.hpp>

#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

#include <Utils/System.hpp>

using namespace Mira::Plugins;


Debugger::Debugger() :
    m_Buffer { 0 }
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);

    // Initialize our buffer mutex
    mtx_init(&m_Mutex, "DbgMtx", nullptr, 0);
}

Debugger::~Debugger()
{
    auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);

    // Zero out our buffer
    memset(m_Buffer, 0, sizeof(m_Buffer));

    mtx_destroy(&m_Mutex);
}

bool Debugger::OnLoad()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
	// Create the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = subhook_new((void*)kdlsym(trap_fatal), (void*)OnTrapFatal, subhook_flags_t::SUBHOOK_64BIT_OFFSET);
    
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        subhook_install(m_TrapFatalHook);
    }
#endif
    return true;
}

bool Debugger::OnUnload()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
    WriteLog(LL_Info, "deleting trap fatal hook");
	if (m_TrapFatalHook != nullptr)
    {
        subhook_remove(m_TrapFatalHook);
        subhook_free(m_TrapFatalHook);
    }
#endif

    return true;
}

int32_t Debugger::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    // TODO: Handle debugger specific stuff (same that's available over rpc)
    return 0;
}


void* Debugger::FindJmpslotAddress(struct proc* p_Process, const char* p_ModuleName, const char* p_Name, int32_t p_isNid) 
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
    if (s_TitleId == nullptr)
    {
        WriteLog(LL_Warn, "Invalid title id.");
        return 0;
    }

    // Get the nids of the function
    char s_Nids[0xD] = { 0 };
    if ( p_isNid )
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
            if (strncmp(p_ModuleName, JS_MAIN_MODULE, _MAX_PATH) == 0)
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

void* Debugger::ResolveFuncAddress(struct proc* p_Process, const char* p_Name, int32_t p_isNid)
{
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto dynlib_do_dlsym = (void*(*)(void* dl, void* obj, const char* name, const char* libname, unsigned int flags))kdlsym(dynlib_do_dlsym);

    if (p_Process == nullptr)
        return nullptr;

    // TODO: Fix this structure within proc
    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);

    // Line shuts up retail builds
    if (s_TitleId == nullptr)
        return nullptr;
    
    void* s_Address = nullptr;

    WriteLog(LL_Info, "TitleId: (%s).", s_TitleId);

    if (p_Process->p_dynlib) {
        // Lock dynlib object
        struct sx* dynlib_bind_lock = &p_Process->p_dynlib->bind_lock;
        A_sx_xlock_hard(dynlib_bind_lock, 0);

        auto main_dylib_obj = p_Process->p_dynlib->main_obj;

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
                WriteLog(LL_Info, "[%s] search(%i): %p_Process lib_name: %s handle: 0x%lx relocbase: %p_Process ...", s_TitleId, total, (void*)dynlib_obj, lib_name, handle, relocbase);
                */

                // Doing a dlsym with  or name
                if ( p_isNid ) {
                    s_Address = dynlib_do_dlsym((void*)p_Process->p_dynlib, (void*)dynlib_obj, p_Name, NULL, 0x1); // name = nids
                } else {
                    s_Address = dynlib_do_dlsym((void*)p_Process->p_dynlib, (void*)dynlib_obj, p_Name, NULL, 0x0); // use name (dynlib_do_dlsym will calculate later)
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