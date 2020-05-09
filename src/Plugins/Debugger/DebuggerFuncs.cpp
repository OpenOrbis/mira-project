#include "Debugger2.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Logger.hpp>

#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/proc.h>
    #include <sys/ptrace.h>
    #include <machine/reg.h>
};

using namespace Mira::Plugins;

bool Debugger2::GetVmMapEntries(struct proc* p_Process, DbgVmEntry**& p_Entries, size_t& p_EntriesCount)
{
    // Set some default values
    p_Entries = nullptr;
    p_EntriesCount = 0;

    // Helper structures
    ProcVmMapEntry* s_VmMapEntries = nullptr;
    size_t s_VmMapEntriesCount = 0;

    // Temporary space to hold the allocation until we complete
    DbgVmEntry** s_DbgVmEntries = nullptr;

    // Yer
    int32_t s_Ret = 0;
    bool s_Success = false;

    if (p_Process == nullptr)
        goto cleanup;
    
    // Get the VM Entries
    s_Ret = Mira::OrbisOS::Utilities::GetProcessVmMap(p_Process, &s_VmMapEntries, &s_VmMapEntriesCount);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get process vm map (%d)", s_Ret);
        goto cleanup;
    }

    if (s_VmMapEntries == nullptr || s_VmMapEntriesCount == 0)
    {
        WriteLog(LL_Warn, "there are no vm entries in this process?");
        p_Entries = nullptr;
        p_EntriesCount = 0;

        s_Success = true;
        goto cleanup;
    }

    // Allocate a new list of pointers
    s_DbgVmEntries = new DbgVmEntry*[s_VmMapEntriesCount];
    if (s_DbgVmEntries == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbgvmentry list of count (%llx).", s_VmMapEntriesCount);
        goto cleanup;
    }
    
    // Zero out the entire list
    memset(s_DbgVmEntries, 0, sizeof(*s_DbgVmEntries) * s_VmMapEntriesCount);

    for (auto i = 0; i < s_VmMapEntriesCount; ++i)
    {
        // Allocate a new entry
        auto l_Entry = new DbgVmEntry();
        if (l_Entry == nullptr)
        {
            WriteLog(LL_Error, "could not allocate dbg vm entry");
            continue;
        }
        memset(l_Entry, 0, sizeof(*l_Entry));

        *l_Entry = DBG_VM_ENTRY__INIT;

        // Allocate name
        auto l_NameLen = sizeof(s_VmMapEntries[i].name);
        auto l_Name = new char[l_NameLen];
        if (l_Name == nullptr)
        {
            // Free our allocated entry
            delete l_Entry;

            WriteLog(LL_Error, "could not allocate name (%d)", l_NameLen);
            continue;
        }

        // Zero the allocated buffer
        memset(l_Name, 0, l_NameLen);

        // Copy over the buffer
        memcpy(l_Name, s_VmMapEntries[i].name, l_NameLen);

        // Assign all of our entry things
        l_Entry->name = l_Name;
        l_Entry->start = s_VmMapEntries[i].start;
        l_Entry->end = s_VmMapEntries[i].end;
        l_Entry->offset = s_VmMapEntries[i].offset;
        l_Entry->protection = s_VmMapEntries[i].prot;

        // Assign our allocate entry to our index
        s_DbgVmEntries[i] = l_Entry;
    }

    p_Entries = s_DbgVmEntries;
    p_EntriesCount = s_VmMapEntriesCount;

    s_Success = true;

cleanup:
    if (s_VmMapEntries)
        delete [] s_VmMapEntries;
    
    return s_Success;
}

bool Debugger2::GetProcessLimitedInfo(int32_t p_Pid, DbgProcessLimited* p_Info)
{
    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid info");
        return false;
    }
    
    if (!IsProcessAlive(p_Pid))
    {
        WriteLog(LL_Error, "no process");
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(p_Pid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_Pid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    bool s_Success = false;
    size_t s_NameLength = 0;
    char* s_Name = nullptr;


    //============================================
    *p_Info = DBG_PROCESS_LIMITED__INIT;

    p_Info->processid = s_Process->p_pid;

    // Get the vm map
    DbgVmEntry** s_VmEntries = nullptr;
    size_t s_VmEntriesCount = 0;

    if (!GetVmMapEntries(s_Process, s_VmEntries, s_VmEntriesCount))
    {
        WriteLog(LL_Error, "could not get vm map entries");
        goto cleanup;
    }

    // Name
    s_NameLength = sizeof(s_Process->p_comm);
    s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        goto cleanup;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, s_Process->p_comm, s_NameLength);
    p_Info->name = s_Name;

    p_Info->entries = s_VmEntries;
    p_Info->n_entries = s_VmEntriesCount;

    s_Success = true;

cleanup:
    if (s_Success)
        return s_Success;
    
    // We need to iterate over the vm entries list
    if (s_VmEntries != nullptr && s_VmEntriesCount != 0)
    {
        for (auto i = 0; i < s_VmEntriesCount; ++i)
        {
            // Get the entry
            auto l_Entry = s_VmEntries[i];
            if (l_Entry == nullptr)
                continue;
            
            // If the name was allocated free it
            if (l_Entry->name != nullptr)
                delete [] l_Entry->name;
            
            l_Entry->name = nullptr;

            // Finally delete the entry object
            delete l_Entry;

            // Clear the entry out of the list
            s_VmEntries[i] = nullptr;
        }

        // Delete the entries list
        delete [] s_VmEntries;

        //s_VmEntries = nullptr;
        //s_VmEntriesCount = 0;
    }

    if (s_Name)
        delete [] s_Name;

    return s_Success;
}

bool Debugger2::GetProcessFullInfo (int32_t p_ProcessId, DbgProcessFull* p_Info)
{
    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid process info");
        return false;
    }

    if (!IsProcessAlive(p_ProcessId))
	{
        WriteLog(LL_Error, "invalid pid (%d)", p_ProcessId);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    //auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    //auto _thread_lock_flags = (void(*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);

    // Initialize our dbg process full
    *p_Info = DBG_PROCESS_FULL__INIT;

    // Find the process
    auto s_Process = pfind(p_ProcessId);
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
        return false;
    }
    PROC_UNLOCK(s_Process); // pfind returns a locked process, unlock it

    do
    {
        struct thread* s_Thread = nullptr;
        uint64_t s_ThreadCount = 0;
        uint64_t s_CurrentThreadIndex = 0;
        FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
            s_ThreadCount++;
        
        // Verify that we have some amount of threads
        if (s_ThreadCount == 0)
        {
            WriteLog(LL_Error, "could not get any dents.");
            break;
        }

        DbgThreadLimited* s_Threads[s_ThreadCount];
        memset(s_Threads, 0, sizeof(s_Threads));

        FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        {
            // Bounds check our threads
            if (s_CurrentThreadIndex >= s_ThreadCount || s_Thread == nullptr)
                break;
            
            // Allocate a new thread information
            auto l_ThreadInfo = new DbgThreadLimited();
            if (l_ThreadInfo == nullptr)
            {
                WriteLog(LL_Error, "could not allocate thread information.");
                continue;
            }
            memset(l_ThreadInfo, 0, sizeof(*l_ThreadInfo));

            // Try and get the thread information (function initializes)
            if (!GetThreadLimitedInfo(s_Thread, l_ThreadInfo))
            {
                WriteLog(LL_Error, "could not get thread limited info.");
                delete l_ThreadInfo;
                continue;
            }

            s_Threads[s_CurrentThreadIndex] = l_ThreadInfo;
            
            // Increment our current thread index
            s_CurrentThreadIndex++;
        }

        // Assign our information
        p_Info->threads = s_Threads;

        // Placeholder value
        p_Info->unused = 0;
        p_Info->processid = s_Process->p_pid;
        p_Info->parentproc = reinterpret_cast<uint64_t>(s_Process->p_pptr);
        p_Info->oppid = s_Process->p_oppid;
        p_Info->dbgchild = s_Process->p_dbg_child;
        p_Info->vmspace = reinterpret_cast<uint64_t>(s_Process->p_vmspace);
        p_Info->exitthreads = s_Process->p_exitthreads;
        p_Info->sigparent = s_Process->p_sigparent;
        p_Info->sig = s_Process->p_sig;
        p_Info->code = s_Process->p_code;
        p_Info->stops = s_Process->p_stops;
        p_Info->stype = s_Process->p_stype;
        p_Info->singlethread = reinterpret_cast<uint64_t>(s_Process->p_singlethread);
        p_Info->suspendcount = s_Process->p_suspcount;
        p_Info->dynlib = reinterpret_cast<uint64_t>(s_Process->p_dynlib);
        
        // The process name is a static length (CALLEE MUST FREE THIS)
        auto s_ProcessNameLength = sizeof(s_Process->p_comm);
        auto s_ProcessName = new char[s_ProcessNameLength];
        if (s_ProcessName == nullptr)
        {
            WriteLog(LL_Error, "could not allocate process name.");
            break;
        }
        memset(s_ProcessName, 0, s_ProcessNameLength);
        memcpy(s_ProcessName, s_Process->p_comm, s_ProcessNameLength);
        p_Info->name = s_ProcessName;

        // The elf path is a static length (CALLEE MUST FREE THIS)
        auto s_ElfPathLength = sizeof(s_Process->p_elfpath);
        auto s_ElfPath = new char[s_ElfPathLength];
        if (s_ElfPath == nullptr)
        {
            WriteLog(LL_Error, "could not allocate elf path.");
            break;
        }
        memset(s_ElfPath, 0, s_ElfPathLength);
        memcpy(s_ElfPath, s_Process->p_elfpath, s_ElfPathLength);
        p_Info->elfpath =  s_ElfPath;

        // The randomized path is static length (CALLEE MUST FREE THIS)
        auto s_RandomizedPathLength = sizeof(s_Process->p_randomized_path);
        auto s_RandomizedPath = new char[s_RandomizedPathLength];
        if (s_RandomizedPath == nullptr)
        {
            WriteLog(LL_Error, "could not allocate randomized path");
            break;
        }
        memset(s_RandomizedPath, 0, s_RandomizedPathLength);
        memcpy(s_RandomizedPath, s_Process->p_randomized_path, s_RandomizedPathLength);
        p_Info->randomizedpath = s_RandomizedPath;

        p_Info->numthreads = s_Process->p_numthreads;

        // Get the vm map
        DbgVmEntry** s_VmEntries = nullptr;
        size_t s_VmEntriesCount = 0;

        if (!GetVmMapEntries(s_Process, s_VmEntries, s_VmEntriesCount))
        {
            WriteLog(LL_Error, "could not get vm map entries");
            break;
        }
        p_Info->mapentries = s_VmEntries;
        p_Info->n_mapentries = s_VmEntriesCount;

        return true;
    } while (false);
    
    // Handle cleanup

    // Cleanup the vm map entries
    if (p_Info->mapentries != nullptr && p_Info->n_mapentries != 0)
    {
        for (auto i = 0; i < p_Info->n_mapentries; ++i)
        {
            // Get the entry
            auto l_Entry = p_Info->mapentries[i];
            if (l_Entry == nullptr)
                continue;
            
            // If the name was allocated free it
            if (l_Entry->name != nullptr)
                delete [] l_Entry->name;
            l_Entry->name = nullptr;

            // Finally delete the entry object
            delete l_Entry;

            // Clear the entry out of the list
            p_Info->mapentries[i] = nullptr;
        }

        // Delete the entries list
        delete [] p_Info->mapentries;
        p_Info->mapentries = nullptr;
        p_Info->n_mapentries = 0;
    }

    // Cleanup the randomized path
    if (p_Info->randomizedpath != nullptr)
    {
        delete [] p_Info->randomizedpath;
        p_Info->randomizedpath = nullptr;
    }

    // Cleanup the elf path
    if (p_Info->elfpath != nullptr)
    {
        delete [] p_Info->elfpath;
        p_Info->elfpath = nullptr;
    }
    
    // Cleanup the process name
    if (p_Info->name != nullptr)
    {
        delete [] p_Info->name;
        p_Info->name = nullptr;
    }
    
    // Cleanup all of the threads
    if (p_Info->threads != nullptr && p_Info->n_threads != 0)
    {
        // Iterate through each of the threads
        for (auto i = 0; i < p_Info->n_threads; ++i)
        {
            // Verify that there's a thread in this slot
            auto l_Thread = p_Info->threads[i];
            if (l_Thread == nullptr)
                continue;
            
            // Free the thread name
            if (l_Thread->name != nullptr)
            {
                delete [] l_Thread->name;
                l_Thread->name = nullptr;
            }

            delete l_Thread;
            p_Info->threads[i] = nullptr;
        }

        // TODO: Determine if this is correct usage
        WriteLog(LL_Error, "boom?");
        delete [] p_Info->threads;
        p_Info->threads = nullptr;
        p_Info->n_threads = 0;
    }
    
    // Since everything has been freed, reset the structure
    *p_Info = DBG_PROCESS_FULL__INIT;

    return false;
}

bool Debugger2::GetCredentials(struct ucred* p_Creds, DbgCred* p_Info)
{
    if (p_Creds == nullptr)
    {
        WriteLog(LL_Error, "could not get credentials.");
        return false;
    }

    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid usage of dbg cred.");
        return false;
    }

    // Initialize our credential structure
    *p_Info = DBG_CRED__INIT;

    do
    {
        p_Info->effectiveuserid = p_Creds->cr_uid;
        p_Info->realuserid = p_Creds->cr_ruid;
        p_Info->saveduserid = p_Creds->cr_svuid;
        p_Info->numgroups = p_Creds->cr_ngroups;
        p_Info->realgroupid = p_Creds->cr_rgid;
        p_Info->saveduserid = p_Creds->cr_svgid;
        p_Info->prison = reinterpret_cast<uint64_t>(p_Creds->cr_prison);
        p_Info->sceauthid = p_Creds->cr_sceAuthID;

        // Allocate new caps
        auto s_CapsSize = ARRAYSIZE(p_Creds->cr_sceCaps);
        uint64_t* s_Caps = new uint64_t[s_CapsSize];
        if (s_Caps == nullptr)
        {
            WriteLog(LL_Error, "could not allocate sce caps.");
            break;
        }

        // Copy over all of our caps (CALLEE MUST CLEANUP)
        for (auto i = 0; i < s_CapsSize; ++i)
            s_Caps[i] = p_Creds->cr_sceCaps[i];
        
        p_Info->scecaps = s_Caps;
        p_Info->n_scecaps = s_CapsSize;
        
        // Allocate our new attributes (CALLEE MUST CLEANUP)
        auto s_AttributesSize = ARRAYSIZE(p_Creds->cr_sceAttr);
        uint64_t* s_Attributes = new uint64_t[s_AttributesSize];
        if (s_Attributes == nullptr)
        {
            WriteLog(LL_Error, "could not allocate attributes.");
            break;
        }

        // Copy over all of the attributes
        for (auto i = 0; i < s_AttributesSize; ++i)
            s_Attributes[i] = p_Creds->cr_sceAttr[i];
        
        p_Info->sceattr = s_Attributes;
        p_Info->n_sceattr = s_AttributesSize;

        return true;
    } while (false);

    // Cleanup

    // Cleanup attributes
    if (p_Info->sceattr != nullptr && p_Info->n_sceattr != 0)
    {
        for (auto i = 0; i < p_Info->n_sceattr; ++i)
            p_Info->sceattr[i] = 0;
        
        delete [] p_Info->sceattr;
        p_Info->sceattr = nullptr;
        p_Info->n_sceattr = 0;
    }

    // Cleanup caps
    if (p_Info->scecaps != nullptr && p_Info->n_scecaps != 0)
    {
        for (auto i = 0; i < p_Info->n_scecaps; ++i)
            p_Info->scecaps[i] = 0;
        
        delete [] p_Info->scecaps;
        p_Info->scecaps = nullptr;
        p_Info->n_scecaps = 0;
    }
    
    // Reset the information back to default
    *p_Info = DBG_CRED__INIT;
    return false;
}

uint64_t Debugger2::GetProcessThreadCount(int32_t p_ProcessId)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);

    uint64_t s_ThreadCount = 0;
    struct proc* s_Process = pfind(p_ProcessId);
    struct thread* s_Thread = nullptr;

	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
        return 0;
	}

    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    // Get the threads
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        s_ThreadCount++;

    return s_ThreadCount;
}

bool Debugger2::GetThreadLimitedInfo(int32_t p_ThreadId, DbgThreadLimited* p_Info)
{
    auto s_Thread = GetThreadById(p_ThreadId);
    if (s_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid tid");
        return false;
    }

    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid output info");
        return false;
    }

    return GetThreadLimitedInfo(s_Thread, p_Info);
}

bool Debugger2::GetThreadLimitedInfo(struct thread* p_Thread, DbgThreadLimited* p_Info)
{
    // Validate thread
    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread");
        return false;
    }

    // Validate output information
    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid output info");
        return false;
    }

    // Initialize our structure
    *p_Info = DBG_THREAD_LIMITED__INIT;
    
    do
    {
        // Assign fields
        p_Info->proc = reinterpret_cast<uint64_t>(p_Thread->td_proc);
        p_Info->threadid = p_Thread->td_tid;
        
        // Allocate dynamic buffer for the name (CALLEE MUST FREE THIS)
        // NOTE: thread name is a static buffer
        auto s_ThreadNameLength = sizeof(p_Thread->td_name);
        auto s_ThreadName = new char[s_ThreadNameLength];
        if (s_ThreadName == nullptr)
        {
            WriteLog(LL_Error, "could not allocate name length (%d).", s_ThreadNameLength);
            break;
        }
        memset(s_ThreadName, 0, s_ThreadNameLength);
        memcpy(s_ThreadName, p_Thread->td_name, s_ThreadNameLength);
        p_Info->name = s_ThreadName;

        p_Info->retval = p_Thread->td_retval[0];
        p_Info->kernelstack = p_Thread->td_kstack;
        p_Info->kernelstackpages = p_Thread->td_kstack_pages;
        p_Info->err_no = p_Thread->td_errno;

        return true;
    } while (false);

    // Handle cleanup

    // Name cleanup
    if (p_Info->name != nullptr)
    {
        delete [] p_Info->name;
        p_Info->name = nullptr;
    }

    // Re-initialize all of the default values on failure
    *p_Info = DBG_THREAD_LIMITED__INIT;
    
    return false;
}

// THIS CALLS DELETE ON THE OBJECT ITSELF
void Debugger2::FreeThreadLimitedInfo(DbgThreadLimited* p_Info)
{
    // TODO: Implement
}

bool Debugger2::GetThreadFullInfo(struct thread* p_Thread, DbgThreadFull* p_Info)
{
    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid tid");
        return false;
    }

    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid output info");
        return false;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
    if (s_ThreadManager == nullptr)
    {
        WriteLog(LL_Error, "could not get thread manager.");
        return false;
    }

    auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
    if (s_DebuggerThread == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger thread.");
        return false;
    }

    *p_Info = DBG_THREAD_FULL__INIT;

    do
    {
        p_Info->proc = reinterpret_cast<uint64_t>(p_Thread->td_proc);
        p_Info->threadid = p_Thread->td_tid;
        
        auto s_ThreadNameLength = sizeof(p_Thread->td_name);
        auto s_ThreadName = new char[s_ThreadNameLength];
        if (s_ThreadName == nullptr)
        {
            WriteLog(LL_Error, "could not allocate thread name.");
            break;
        }
        memset(s_ThreadName, 0, s_ThreadNameLength);
        memcpy(s_ThreadName, p_Thread->td_name, s_ThreadNameLength);

        p_Info->name = s_ThreadName;
        p_Info->retval = p_Thread->td_retval[0];
        p_Info->kernelstack = p_Thread->td_kstack;
        p_Info->kernelstackpages = p_Thread->td_kstack_pages;
        p_Info->err_no = p_Thread->td_errno;

        // Stop the process if it's not already stopped
        auto s_Process = p_Thread->td_proc;
        if (s_Process == nullptr)
        {
            WriteLog(LL_Error, "could not get process.");
            break;
        }
        
        auto s_ProcessId = s_Process->p_pid;

        struct reg s_Reg;
        struct fpreg s_FpReg;
        struct dbreg s_DbReg;
        memset(&s_Reg, 0, sizeof(s_Reg));
        memset(&s_FpReg, 0, sizeof(s_FpReg));
        memset(&s_DbReg, 0, sizeof(s_DbReg));

        // In order to get registers we need to stop the process
        auto s_Ret = kkill_t(s_Process->p_pid, SIGSTOP, s_DebuggerThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stop process (%d).", s_Ret);
            break;
        }

        // Get general purpose registers
        s_Ret = kptrace_t(PT_GETREGS, s_ProcessId, reinterpret_cast<caddr_t>(&s_Reg), 0, s_DebuggerThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not get gp registers (%d).", s_Ret);
            break;
        }

        // Get floating point registers
        s_Ret = kptrace_t(PT_GETFPREGS, s_ProcessId, reinterpret_cast<caddr_t>(&s_FpReg), 0, s_DebuggerThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not get fp registers (%d).", s_Ret);
            break;
        }

        // Get debug registers
        s_Ret = kptrace_t(PT_GETDBREGS, s_ProcessId, reinterpret_cast<caddr_t>(&s_DbReg), 0, s_DebuggerThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not get debug registers (%d).", s_Ret);
            break;
        }

        // Resume the process
        s_Ret = kkill_t(s_ProcessId, SIGCONT, s_DebuggerThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not resume process (%d).", s_Ret);
            break;
        }

        // Allocate the general purpose registers (CALLEE NEEDS TO FREE)
        auto s_GpRegisters = new DbgGpRegisters();
        if (s_GpRegisters == nullptr)
        {
            WriteLog(LL_Error, "could not allocate gp registers.");
            break;
        }

        *s_GpRegisters = DBG_GP_REGISTERS__INIT;
        s_GpRegisters->r_r15 = s_Reg.r_r15;
        s_GpRegisters->r_r14 = s_Reg.r_r14;
        s_GpRegisters->r_r13 = s_Reg.r_r13;
        s_GpRegisters->r_r12 = s_Reg.r_r12;
        s_GpRegisters->r_r11 = s_Reg.r_r11;
        s_GpRegisters->r_r10 = s_Reg.r_r10;
        s_GpRegisters->r_r9 = s_Reg.r_r9;
        s_GpRegisters->r_r8 = s_Reg.r_r8;
        s_GpRegisters->r_rdi = s_Reg.r_rdi;
        s_GpRegisters->r_rsi = s_Reg.r_rsi;
        s_GpRegisters->r_rbp = s_Reg.r_rbp;
        s_GpRegisters->r_rbx = s_Reg.r_rbx;
        s_GpRegisters->r_rdx = s_Reg.r_rdx;
        s_GpRegisters->r_rax = s_Reg.r_rax;
        s_GpRegisters->r_trapno = s_Reg.r_trapno;
        s_GpRegisters->r_fs = s_Reg.r_fs;
        s_GpRegisters->r_gs = s_Reg.r_gs;
        s_GpRegisters->r_err = s_Reg.r_err;
        s_GpRegisters->r_es = s_Reg.r_es;
        s_GpRegisters->r_ds = s_Reg.r_ds;
        s_GpRegisters->r_rip = s_Reg.r_rip;
        s_GpRegisters->r_cs = s_Reg.r_cs;
        s_GpRegisters->r_rflags = s_Reg.r_rflags;
        s_GpRegisters->r_rsp = s_Reg.r_rsp;
        s_GpRegisters->r_ss = s_Reg.r_ss;

        p_Info->gpregisters = s_GpRegisters;

        // Allocate and set the floating point registers (CALLEE MUST FREE)
        auto s_FpRegisters = new DbgFpRegisters();
        if (s_FpRegisters == nullptr)
        {
            WriteLog(LL_Error, "could not allocate fp registers.");
            break;
        }
        *s_FpRegisters = DBG_FP_REGISTERS__INIT;

        // Get the data, because I don't feel like converting this (CALLEE MUST FREE)
        auto s_FpRegistersSize = sizeof(s_FpReg);
        auto s_FpRegistersData = new uint8_t[s_FpRegistersSize];
        if (s_FpRegistersData == nullptr)
        {
            WriteLog(LL_Error, "could not allocate fp registers data.");
            break;
        }
        memset(s_FpRegistersData, 0, s_FpRegistersSize);
        memcpy(s_FpRegistersData, &s_FpReg, s_FpRegistersSize);

        s_FpRegisters->data.data = s_FpRegistersData;
        s_FpRegisters->data.len = s_FpRegistersSize;

        p_Info->fpregisters = s_FpRegisters;

        // Allocate the debug registers (CALLEE MUST FREE)
        auto s_DbRegisters = new DbgDbRegisters();
        if (s_DbRegisters == nullptr)
        {
            WriteLog(LL_Error, "could not allocate db registers.");
            break;
        }
        *s_DbRegisters = DBG_DB_REGISTERS__INIT;

        // CALLEE MUST FREE
        auto s_DbRegsSize = ARRAYSIZE(s_DbReg.dr);
        uint32_t* s_DbRegs = new uint32_t[s_DbRegsSize];
        if (s_DbRegs == nullptr)
        {
            WriteLog(LL_Error, "could not allocate db registers.");
            break;
        }
        for (auto i = 0; i < s_DbRegsSize; ++i)
            s_DbRegs[i] = s_DbReg.dr[i];
        
        s_DbRegisters->debugregs = s_DbRegs;
        s_DbRegisters->n_debugregs = s_DbRegsSize;

        p_Info->dbregisters = s_DbRegisters;

        return true;
    } while (false);
    
    // handle cleanup

    // Cleanup the dbregs
    if (p_Info->dbregisters != nullptr)
    {
        if (p_Info->dbregisters->debugregs != nullptr && p_Info->dbregisters->n_debugregs)
        {
            delete [] p_Info->dbregisters->debugregs;
            p_Info->dbregisters->debugregs = nullptr;
            p_Info->dbregisters->n_debugregs = 0;
        }

        delete p_Info->dbregisters;
        p_Info->dbregisters = nullptr;
    }

    // cleanup the fpregs
    if (p_Info->fpregisters != nullptr)
    {
        if (p_Info->fpregisters->data.data != nullptr)
        {
            delete [] p_Info->fpregisters->data.data;
            p_Info->fpregisters->data.data = nullptr;
            p_Info->fpregisters->data.len = 0;
        }

        delete p_Info->fpregisters;
        p_Info->fpregisters = nullptr;
    }

    // cleanup the gpregs
    if (p_Info->gpregisters != nullptr)
    {
        delete p_Info->gpregisters;
        p_Info->gpregisters = nullptr;
    }

    // cleanup name
    if (p_Info->name != nullptr)
    {
        delete [] p_Info->name;
        p_Info->name = nullptr;
    }

    *p_Info = DBG_THREAD_FULL__INIT;

    return false;
}

void Debugger2::FreeDbgThreadFull(DbgThreadFull* p_Info)
{
    if (p_Info == nullptr)
        return;
    
    if (p_Info->dbregisters)
    {
        if (p_Info->dbregisters->debugregs)
            delete [] p_Info->dbregisters->debugregs;
        
        p_Info->dbregisters->debugregs = nullptr;
        p_Info->dbregisters->n_debugregs = 0;

        delete p_Info->dbregisters;
        p_Info->dbregisters = nullptr;
    }

    if (p_Info->fpregisters)
    {
        // we just zero this because it points to stack
        if (p_Info->fpregisters->data.data)
            delete [] p_Info->fpregisters->data.data;
        
        p_Info->fpregisters->data.data = nullptr;
        p_Info->fpregisters->data.len = 0;

        delete p_Info->fpregisters;
        p_Info->fpregisters = nullptr;
    }

    if (p_Info->gpregisters)
    {
        delete p_Info->gpregisters;
        p_Info->gpregisters = nullptr;
    }

    if (p_Info->name)
    {
        delete [] p_Info->name;
        p_Info->name = nullptr;
    }
}

bool Debugger2::GetThreadFullInfo(int32_t p_ThreadId, DbgThreadFull* p_Info)
{
    if (p_ThreadId < 0)
    {
        WriteLog(LL_Error, "invalid tid");
        return false;
    }

    auto s_Thread = GetThreadById(p_ThreadId);
    if (s_Thread == nullptr)
    {
        WriteLog(LL_Error, "could not get thread");
        return false;
    }

    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid output info");
        return false;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
		return -EIO;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
		return -EIO;
	}

    bool s_Success = false;
    int32_t s_Ret = 0;

    size_t s_NameLength = 0;
    char* s_Name = nullptr;

    struct reg s_GpRegisters;
    struct fpreg s_FpRegisters;
    struct dbreg s_DbRegisters;

    DbgGpRegisters* s_DbgGpRegisters = nullptr;
    DbgFpRegisters* s_DbgFpRegisters = nullptr;
    DbgDbRegisters* s_DbgDbRegisters = nullptr;

    size_t s_DrLength = 0;
    uint32_t* s_Dr = nullptr;

    // Name
    s_NameLength = sizeof(s_Thread->td_name);
    s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        goto cleanup;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, s_Thread->td_name, s_NameLength);


    s_Ret = kkill_t(m_AttachedPid, SIGSTOP, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not stop process (%d).", s_Ret);
        goto cleanup;
    }

    
    memset(&s_GpRegisters, 0, sizeof(s_GpRegisters));
    s_Ret = kptrace_t(PT_GETREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_GpRegisters), 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get gp regs (%d).", s_Ret);
        goto cleanup;
    }

    s_DbgGpRegisters = new DbgGpRegisters;
    if (s_DbgGpRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg gp registers");
        goto cleanup;
    }
    memset(s_DbgGpRegisters, 0, sizeof(*s_DbgGpRegisters));

    *s_DbgGpRegisters = DBG_GP_REGISTERS__INIT;
    s_DbgGpRegisters->r_r15 = s_GpRegisters.r_r15;
    s_DbgGpRegisters->r_r14 = s_GpRegisters.r_r14;
    s_DbgGpRegisters->r_r13 = s_GpRegisters.r_r13;
    s_DbgGpRegisters->r_r12 = s_GpRegisters.r_r12;
    s_DbgGpRegisters->r_r11 = s_GpRegisters.r_r11;
    s_DbgGpRegisters->r_r10 = s_GpRegisters.r_r10;
    s_DbgGpRegisters->r_r9 = s_GpRegisters.r_r9;
    s_DbgGpRegisters->r_r8 = s_GpRegisters.r_r8;
    s_DbgGpRegisters->r_rdi = s_GpRegisters.r_rdi;
    s_DbgGpRegisters->r_rsi = s_GpRegisters.r_rsi;
    s_DbgGpRegisters->r_rbp = s_GpRegisters.r_rbp;
    s_DbgGpRegisters->r_rbx = s_GpRegisters.r_rbx;
    s_DbgGpRegisters->r_rdx = s_GpRegisters.r_rdx;
    s_DbgGpRegisters->r_trapno = s_GpRegisters.r_trapno;
    s_DbgGpRegisters->r_fs = s_GpRegisters.r_fs;
    s_DbgGpRegisters->r_gs = s_GpRegisters.r_gs;
    s_DbgGpRegisters->r_err = s_GpRegisters.r_err;
    s_DbgGpRegisters->r_es = s_GpRegisters.r_es;
    s_DbgGpRegisters->r_ds = s_GpRegisters.r_ds;
    s_DbgGpRegisters->r_rip = s_GpRegisters.r_rip;
    s_DbgGpRegisters->r_cs = s_GpRegisters.r_cs;
    s_DbgGpRegisters->r_rflags = s_GpRegisters.r_rflags;
    s_DbgGpRegisters->r_rsp = s_GpRegisters.r_rsp;
    s_DbgGpRegisters->r_ss = s_GpRegisters.r_ss;

    

    
    memset(&s_FpRegisters, 0, sizeof(s_FpRegisters));
    s_Ret = kptrace_t(PT_GETFPREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_FpRegisters), 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get fp regs (%d).", s_Ret);
        goto cleanup;
    }

    s_DbgFpRegisters = new DbgFpRegisters;
    if (s_DbgFpRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg fp register");
        goto cleanup;
    }
    *s_DbgFpRegisters = DBG_FP_REGISTERS__INIT;
    s_DbgFpRegisters->data.data = reinterpret_cast<uint8_t*>(&s_FpRegisters);
    s_DbgFpRegisters->data.len = sizeof(s_FpRegisters);

    

    
    memset(&s_DbRegisters, 0, sizeof(s_DbRegisters));
    s_Ret = kptrace_t(PT_GETDBREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_DbRegisters), 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get db regs (%d).", s_Ret);
        goto cleanup;
    }

    s_DbgDbRegisters = new DbgDbRegisters;
    if (s_DbgDbRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg db registers");
        goto cleanup;
    }
    *s_DbgDbRegisters = DBG_DB_REGISTERS__INIT;

    s_DrLength = ARRAYSIZE(s_DbRegisters.dr);
    s_Dr = new uint32_t[s_DrLength];
    if (s_Dr == nullptr)
    {
        WriteLog(LL_Error, "could not allocate debug registers");
        goto cleanup;
    }

    // Copy this garbage
    for (auto i = 0; i < s_DrLength; ++i)
        s_Dr[i] = s_DbRegisters.dr[i];
    
    s_DbgDbRegisters->debugregs = s_Dr;



    s_Ret = kkill_t(m_AttachedPid, SIGCONT, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not resume process (%d).", s_Ret);
        goto cleanup;
    }

    *p_Info = DBG_THREAD_FULL__INIT;
    p_Info->proc = reinterpret_cast<uint64_t>(s_Thread->td_proc);
    p_Info->threadid = s_Thread->td_tid;

    p_Info->name = s_Name;

    p_Info->retval = s_Thread->td_retval[0];
    p_Info->kernelstack = s_Thread->td_kstack;
    p_Info->kernelstackpages = s_Thread->td_kstack_pages;
    p_Info->err_no = s_Thread->td_errno;
    p_Info->gpregisters = s_DbgGpRegisters;
    p_Info->fpregisters = s_DbgFpRegisters;
    p_Info->dbregisters = s_DbgDbRegisters;
    s_Success = true;

cleanup:
    if (s_Success)
        return s_Success;
    
    if (s_Name)
        delete [] s_Name;
    
    if (s_DbgGpRegisters)
        delete s_DbgGpRegisters;
    
    if (s_DbgFpRegisters)
        delete s_DbgFpRegisters;
    
    if (s_DbgDbRegisters)
        delete s_DbgDbRegisters;
    
    if (s_Dr)
        delete [] s_Dr;
    
    return s_Success;
}

bool Debugger2::SignalProcess(int32_t p_Signal)
{
    if (m_AttachedPid < 0)
    {
        WriteLog(LL_Error, "invalid pid");
        return false;
    }
    
    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
		return -EIO;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
		return -EIO;
	}

    auto s_Ret = kkill_t(m_AttachedPid, p_Signal, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not send signal to pid (%d) err: (%d).", m_AttachedPid, s_Ret);
        return false;
    }

    return s_Ret == 0;
}

struct thread* Debugger2::GetThreadById(int32_t p_ProcessId, int32_t p_ThreadId)
{
    if (!IsProcessAlive(p_ProcessId))
        return nullptr;
    
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);

	struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
		return nullptr;
    
    // VVVV DO NOT RETURN VVVVV
    struct thread* s_CurThread = nullptr;
    bool s_Found = false;
    FOREACH_THREAD_IN_PROC(s_Process, s_CurThread)
    {
        if (s_CurThread->td_tid != p_ThreadId)
            continue;
        
        s_Found = true;
        // We just use curthread
        break;
    }
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);
    // VVVV OK TO RETURN VVVV

    if (!s_Found || s_CurThread == nullptr)
        return nullptr;

    return s_CurThread;
}

bool Debugger2::ReadKernelMemory(uint64_t p_Address, uint32_t p_Size, uint8_t*& p_OutData)
{
    auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);

    if (p_Address == 0)
        return false;
    
    if (p_OutData == nullptr)
    {
        WriteLog(LL_Error, "invalid output buffer");
        return false;
    }

    // Zero the output buffer
    memset(p_OutData, 0, p_Size);

    auto s_Ret = vm_fault_disable_pagefaults();
    memcpy(p_OutData, (const void*)p_Address, p_Size);
    vm_fault_enable_pagefaults(s_Ret);

    return true;
}

bool Debugger2::WriteKernelMemory(uint64_t p_Address, uint8_t* p_Data, uint32_t p_Size)
{
    auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);

    if (p_Address == 0)
        return false;
    
    if (p_Data == nullptr)
        return false;
    
    auto s_Ret = vm_fault_disable_pagefaults();
    memcpy(reinterpret_cast<void*>(p_Address), p_Data, p_Size);
    vm_fault_enable_pagefaults(s_Ret);

    return true;
}

bool Debugger2::IsProcessAlive(int32_t p_ProcessId)
{
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);
	return true;
}

bool Debugger2::PacketRequiresAcknowledgement(char p_Request)
{
    // TODO: Implement
    WriteLog(LL_Warn, "not implemented");
    return false;
}

