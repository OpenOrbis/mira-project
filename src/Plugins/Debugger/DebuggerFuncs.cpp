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

    if (IsProcessAlive(p_ProcessId))
	{
        WriteLog(LL_Error, "invalid pid (%d)", p_ProcessId);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);

    uint64_t s_ElfPathLength = 0;
    char* s_ElfPath = nullptr;

    uint64_t s_NameLength = 0;
    char* s_Name = nullptr;

    uint64_t s_RandomizedPathLength = 0;
    char* s_RandomizedPath = nullptr;

    DbgVmEntry** s_VmEntries = nullptr;
    size_t s_VmEntriesCount = 0;

    // Thread iteration
    uint64_t s_ThreadCount = 0;
    struct thread* s_Thread = nullptr;

    // Credentials
    DbgCred* s_DbgCred = nullptr;

    // Temp storage
    DbgThreadLimited** s_DbgThreads = nullptr;
    size_t s_CurrentDbgThreadIndex = 0;

    bool s_Success = false;

    struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		goto cleanup;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    // Get the threads
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        s_ThreadCount++;
    
    s_Thread = nullptr;

    if (s_ThreadCount == 0)
    {
        WriteLog(LL_Error, "could not get any threads");
        goto cleanup;
    }

    s_DbgThreads = new DbgThreadLimited*[s_ThreadCount];
    if (s_DbgThreads == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg threads");
        goto cleanup;
    }

    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
    {
        thread_lock(s_Thread);

        DbgThreadLimited* l_ThreadLimited = new DbgThreadLimited();
        if (l_ThreadLimited == nullptr)
        {
            thread_unlock(s_Thread);
            WriteLog(LL_Error, "could allocate thread limited");
            continue;
        }
        memset(l_ThreadLimited, 0, sizeof(*l_ThreadLimited));

        // Fill out the structure
        if (!GetThreadLimitedInfo(s_Thread, l_ThreadLimited))
        {
            delete l_ThreadLimited;

            thread_unlock(s_Thread);
            WriteLog(LL_Error, "could nto get limited thread info");
            continue;
        }
        thread_unlock(s_Thread);

        s_DbgThreads[s_CurrentDbgThreadIndex] = l_ThreadLimited;
        s_CurrentDbgThreadIndex++;
    }

    // Credentials
    s_DbgCred = new DbgCred();
    if (s_DbgCred == nullptr)
    {
        WriteLog(LL_Error, "could not allocate credential");
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

    // Elf path
    s_ElfPathLength = sizeof(s_Process->p_elfpath);
    s_ElfPath = new char[s_ElfPathLength];
    if (s_ElfPath == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory for elf path");
        goto cleanup;
    }
    memset(s_ElfPath, 0, s_ElfPathLength);
    memcpy(s_ElfPath, s_Process->p_elfpath, s_ElfPathLength);
    
    // Randomized path
    s_RandomizedPathLength = s_Process->p_randomized_path_len;
    s_RandomizedPath = new char[s_RandomizedPathLength];
    if (s_RandomizedPath == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory for randomized path len: (%x)", s_RandomizedPathLength);
        goto cleanup;
    }
    memset(s_RandomizedPath, 0, s_RandomizedPathLength);
    memcpy(s_RandomizedPath, s_Process->p_randomized_path, s_RandomizedPathLength);

    if (!GetVmMapEntries(s_Process, s_VmEntries, s_VmEntriesCount))
    {
        WriteLog(LL_Error, "could not get vm map entries");
        goto cleanup;
    }

    // Initialize our message
    *p_Info = DBG_PROCESS_FULL__INIT;

    p_Info->name = s_Name;
    p_Info->elfpath = s_ElfPath;
    p_Info->randomizedpath = s_RandomizedPath;

    p_Info->processid = s_Process->p_pid;
    p_Info->parentproc = reinterpret_cast<uint64_t>(s_Process->p_pptr);
    p_Info->oppid = s_Process->p_oppid;
    p_Info->dbgchild = s_Process->p_dbg_child;
    p_Info->vmspace = reinterpret_cast<uint64_t>(s_Process->p_vmspace);
    p_Info->exitthreads = s_Process->p_exitthreads;
    p_Info->sigparent = s_Process->p_sigparent;
    p_Info->sig = s_Process->p_sig;
    p_Info->code = s_Process->p_code;
    p_Info->stype = s_Process->p_stype;
    p_Info->singlethread = reinterpret_cast<uint64_t>(s_Process->p_singlethread);
    p_Info->suspendcount = s_Process->p_suspcount;
    p_Info->dynlib = reinterpret_cast<uint64_t>(s_Process->p_dynlib);

    if (s_Process->p_ucred != nullptr)
    {
        s_DbgCred->effectiveuserid = s_Process->p_ucred->cr_uid;
        s_DbgCred->realuserid = s_Process->p_ucred->cr_ruid;
        s_DbgCred->saveduserid = s_Process->p_ucred->cr_svuid;
        s_DbgCred->numgroups = s_Process->p_ucred->cr_ngroups;
        s_DbgCred->realgroupid = s_Process->p_ucred->cr_rgid;
        s_DbgCred->savedgroupid = s_Process->p_ucred->cr_svgid;
        s_DbgCred->prison = reinterpret_cast<uint64_t>(s_Process->p_ucred->cr_prison);
        s_DbgCred->sceauthid = s_Process->p_ucred->cr_sceAuthID;

        s_DbgCred->scecaps = s_Process->p_ucred->cr_sceCaps;
        s_DbgCred->n_scecaps = ARRAYSIZE(s_Process->p_ucred->cr_sceCaps);

        s_DbgCred->sceattr = s_Process->p_ucred->cr_sceAttr;
        s_DbgCred->n_sceattr = ARRAYSIZE(s_Process->p_ucred->cr_sceAttr);
    }

    p_Info->cred = s_DbgCred;
    p_Info->mapentries = s_VmEntries;
    p_Info->n_mapentries = s_VmEntriesCount;

    p_Info->threads = s_DbgThreads;
    p_Info->n_threads = s_ThreadCount;
    p_Info->numthreads = s_Process->p_numthreads;
   
    s_Success = true;

cleanup:
    // Handle error condition
    if (s_Success)
        return s_Success;

    // If we have allocated a thread array, we need to iterate and check we don't leak anything
    if (s_DbgThreads)
    {
        // Iterate over the array
        for (auto i = 0; i < s_ThreadCount; ++i)
        {
            // Check if the thread is allocated
            if (s_DbgThreads[i] == nullptr)
                continue;
            
            // Check if the name was allocated
            if (s_DbgThreads[i]->name)
                delete [] s_DbgThreads[i]->name;
            
            // Delete the object
            delete s_DbgThreads[i];

            // Clear the entry in the list
            s_DbgThreads[i] = nullptr;
        }

        // Delete the list (all entries should be nullptr and free'd by now)
        delete [] s_DbgThreads;
    }

    // Free the debug credentials
    if (s_DbgCred != nullptr)
    {
        // There are no allocated fields in dbgcred
        delete s_DbgCred;
        s_DbgCred = nullptr;
    }

    // Delete the name variable
    if (s_Name != nullptr)
        delete [] s_Name;

    // Delete the elf path
    if (s_ElfPath != nullptr)
        delete [] s_ElfPath;

    // Delete the randomized path
    if (s_RandomizedPath != nullptr)
        delete [] s_RandomizedPath;

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

    // Finally reset the output structure so uaf won't happen
    if (p_Info != nullptr)
        *p_Info = DBG_PROCESS_FULL__INIT;

    return s_Success;
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

    *p_Info = DBG_THREAD_LIMITED__INIT;
    p_Info->err_no = s_Thread->td_errno;
    p_Info->kernelstack = s_Thread->td_kstack;
    p_Info->kernelstackpages = s_Thread->td_kstack_pages;
    p_Info->retval = s_Thread->td_retval[0];
    p_Info->threadid = s_Thread->td_tid;

    return true;
}

bool Debugger2::GetThreadLimitedInfo(struct thread* p_Thread, DbgThreadLimited* p_Info)
{
    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread");
        return false;
    }

    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid output info");
        return false;
    }

    *p_Info = DBG_THREAD_LIMITED__INIT;
    p_Info->err_no = p_Thread->td_errno;
    p_Info->kernelstack = p_Thread->td_kstack;
    p_Info->kernelstackpages = p_Thread->td_kstack_pages;
    p_Info->retval = p_Thread->td_retval[0];
    p_Info->threadid = p_Thread->td_tid;

    return true;
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

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    bool s_Success = false;
    int32_t s_Ret = 0;

    size_t s_NameLength = 0;
    char* s_Name = nullptr;

    DbgGpRegisters* s_DbgGpRegisters = nullptr;
    DbgFpRegisters* s_DbgFpRegisters = nullptr;
    DbgDbRegisters* s_DbgDbRegisters = nullptr;

    struct reg* s_GpRegisters;
    struct fpreg* s_FpRegisters;
    struct dbreg* s_DbRegisters;

    size_t s_DrCount = 0;
    uint32_t* s_Dr = nullptr;

    // Name
    s_NameLength = sizeof(p_Thread->td_name);
    s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        goto cleanup;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, p_Thread->td_name, s_NameLength);
    
    // Stop the process
    s_Ret = kkill_t(p_Thread->td_proc->p_pid, SIGSTOP, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not stop process (%d).", s_Ret);
        goto cleanup;
    }

    // Get the general purpose registers
    s_GpRegisters = new struct reg;
    if (s_GpRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocated gp regs");
        goto cleanup;
    }
    memset(s_GpRegisters, 0, sizeof(*s_GpRegisters));
    s_Ret = kptrace_t(PT_GETREGS, p_Thread->td_proc->p_pid, reinterpret_cast<caddr_t>(s_GpRegisters), 0, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get gp regs (%d).", s_Ret);
        goto cleanup;
    }

    // Handle general-purpose registers
    s_DbgGpRegisters = new DbgGpRegisters;
    if (s_DbgGpRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg gp registers");
        goto cleanup;
    }
    memset(s_DbgGpRegisters, 0, sizeof(*s_DbgGpRegisters));

    *s_DbgGpRegisters = DBG_GP_REGISTERS__INIT;
    s_DbgGpRegisters->r_r15 = s_GpRegisters->r_r15;
    s_DbgGpRegisters->r_r14 = s_GpRegisters->r_r14;
    s_DbgGpRegisters->r_r13 = s_GpRegisters->r_r13;
    s_DbgGpRegisters->r_r12 = s_GpRegisters->r_r12;
    s_DbgGpRegisters->r_r11 = s_GpRegisters->r_r11;
    s_DbgGpRegisters->r_r10 = s_GpRegisters->r_r10;
    s_DbgGpRegisters->r_r9 = s_GpRegisters->r_r9;
    s_DbgGpRegisters->r_r8 = s_GpRegisters->r_r8;
    s_DbgGpRegisters->r_rdi = s_GpRegisters->r_rdi;
    s_DbgGpRegisters->r_rsi = s_GpRegisters->r_rsi;
    s_DbgGpRegisters->r_rbp = s_GpRegisters->r_rbp;
    s_DbgGpRegisters->r_rbx = s_GpRegisters->r_rbx;
    s_DbgGpRegisters->r_rdx = s_GpRegisters->r_rdx;
    s_DbgGpRegisters->r_trapno = s_GpRegisters->r_trapno;
    s_DbgGpRegisters->r_fs = s_GpRegisters->r_fs;
    s_DbgGpRegisters->r_gs = s_GpRegisters->r_gs;
    s_DbgGpRegisters->r_err = s_GpRegisters->r_err;
    s_DbgGpRegisters->r_es = s_GpRegisters->r_es;
    s_DbgGpRegisters->r_ds = s_GpRegisters->r_ds;
    s_DbgGpRegisters->r_rip = s_GpRegisters->r_rip;
    s_DbgGpRegisters->r_cs = s_GpRegisters->r_cs;
    s_DbgGpRegisters->r_rflags = s_GpRegisters->r_rflags;
    s_DbgGpRegisters->r_rsp = s_GpRegisters->r_rsp;
    s_DbgGpRegisters->r_ss = s_GpRegisters->r_ss;

    // Handle floating-point registers
    s_FpRegisters = new struct fpreg;
    if (s_FpRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate fp registers");
        goto cleanup;
    }

    memset(s_FpRegisters, 0, sizeof(*s_FpRegisters));
    s_Ret = kptrace_t(PT_GETFPREGS, p_Thread->td_proc->p_pid, reinterpret_cast<caddr_t>(s_FpRegisters), 0, s_MainThread);
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
    s_DbgFpRegisters->data.data = reinterpret_cast<uint8_t*>(s_FpRegisters); // BAD: Must be allocated
    s_DbgFpRegisters->data.len = sizeof(s_FpRegisters);

    // Handle debug registers
    s_DbRegisters = new struct dbreg;
    if (s_DbRegisters == nullptr)
    {
        WriteLog(LL_Error, "could not allocate db regs");
        goto cleanup;
    }
    memset(s_DbRegisters, 0, sizeof(*s_DbRegisters));
    s_Ret = kptrace_t(PT_GETDBREGS, p_Thread->td_proc->p_pid, reinterpret_cast<caddr_t>(s_DbRegisters), 0, s_MainThread);
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

    s_DrCount = ARRAYSIZE(s_DbRegisters->dr);
    s_Dr = new uint32_t[s_DrCount];
    if (s_Dr == nullptr)
    {
        WriteLog(LL_Error, "could not allocate debug registers");
        goto cleanup;
    }

    // Copy this garbage
    for (auto i = 0; i < s_DrCount; ++i)
        s_Dr[i] = s_DbRegisters->dr[i];

    s_Ret = kkill_t(p_Thread->td_proc->p_pid, SIGCONT, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not resume process (%d).", s_Ret);
        goto cleanup;
    }

    *p_Info = DBG_THREAD_FULL__INIT;
    p_Info->proc = reinterpret_cast<uint64_t>(p_Thread->td_proc);
    p_Info->threadid = p_Thread->td_tid;

    p_Info->name = s_Name;
    p_Info->retval = p_Thread->td_retval[0];
    p_Info->kernelstack = p_Thread->td_kstack;
    p_Info->kernelstackpages = p_Thread->td_kstack_pages;
    p_Info->err_no = p_Thread->td_errno;

    s_DbgDbRegisters->debugregs = s_Dr;

    p_Info->fpregisters = s_DbgFpRegisters;
    p_Info->dbregisters = s_DbgDbRegisters;
    p_Info->gpregisters = s_DbgGpRegisters;

    

    s_Success = true;

cleanup:
    if (s_Success)
        return s_Success;
    
    if (s_Dr)
        delete [] s_Dr;
    
    if (s_DbgDbRegisters)
        delete s_DbgDbRegisters;
    
    if (s_DbgFpRegisters)
        delete s_DbgFpRegisters;
    
    if (s_DbgGpRegisters)
        delete s_DbgGpRegisters;
    
    if (s_Name)
        delete [] s_Name;
    
    return s_Success;
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

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
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


    s_Ret = kkill_t(m_AttachedPid, SIGSTOP, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not stop process (%d).", s_Ret);
        goto cleanup;
    }

    
    memset(&s_GpRegisters, 0, sizeof(s_GpRegisters));
    s_Ret = kptrace_t(PT_GETREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_GpRegisters), 0, s_MainThread);
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
    s_Ret = kptrace_t(PT_GETFPREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_FpRegisters), 0, s_MainThread);
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
    s_Ret = kptrace_t(PT_GETDBREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_DbRegisters), 0, s_MainThread);
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



    s_Ret = kkill_t(m_AttachedPid, SIGCONT, s_MainThread);
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
    
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    auto s_Ret = kkill_t(m_AttachedPid, p_Signal, s_MainThread);
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

