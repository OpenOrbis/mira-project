#include "Debugger2.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <OrbisOS/Utilities.hpp>
#include <Messaging/MessageManager.hpp>
#include <Plugins/PluginManager.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <Messaging/Rpc/rpc.pb-c.h>

    #include <sys/uio.h>
	#include <sys/proc.h>
	#include <sys/ioccom.h>
    #include <sys/ptrace.h>
    #include <sys/mman.h>

	#include <vm/vm.h>
	#include <vm/pmap.h>
	#include <machine/frame.h>
	#include <machine/psl.h>
	#include <machine/pmap.h>
	#include <machine/segments.h>
};

using namespace Mira::Plugins;

Debugger2::Debugger2(uint16_t p_Port) :
    m_ServerAddress { 0 },
    m_Socket(-1),
    m_Port(p_Port),
    m_AttachedPid(-1)
{

}

Debugger2::~Debugger2()
{
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Echo, OnEcho);
}

bool Debugger2::OnLoad()
{
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Attach, OnAttach);
    return true;
}

bool Debugger2::OnUnload()
{
    return true;
}

bool Debugger2::OnSuspend()
{
    return OnUnload();
}

bool Debugger2::OnResume()
{
    return OnLoad();
}

void Debugger2::EscapePacket(char* p_PacketData, uint32_t p_Size)
{
    // TODO: Implement
    WriteLog(LL_Warn, "not implemented");
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

bool Debugger2::ReadProcessMemory(uint64_t p_Address, uint32_t p_Size, uint8_t*& p_OutputBuffer)
{
	// Validate that we have a valid process
	if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }
	
	// Check if the buffer is valid
	if (p_OutputBuffer == nullptr)
	{
		WriteLog(LL_Error, "invalid output buffer");
		return false;
	}

	// Zero out the output buffer
	(void)memset(p_OutputBuffer, 0, p_Size);

	// Validate that the process exists
	if (!IsProcessAlive(m_AttachedPid))
		return false;
	
	size_t s_OutputSize = p_Size;
	size_t s_Ret = proc_rw_mem_pid(m_AttachedPid, reinterpret_cast<void*>(p_Address), p_Size, p_OutputBuffer, &s_OutputSize, false);
	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "proc_rw_mem_pid ret: (%d) could not read process (%d) memory (%p) size: (%x) outputSize: (%llx)", s_Ret, m_AttachedPid, p_Address, p_Size, s_OutputSize);
		return false;
	}

	return true;
}

bool Debugger2::WriteProcessMemory(uint64_t p_Address, uint8_t* p_Data, uint32_t p_Size)
{
	if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

	if (p_Data == nullptr)
	{
		WriteLog(LL_Error, "invalid input buffer");
		return false;
	}

	if (!IsProcessAlive(m_AttachedPid))
	{
		WriteLog(LL_Error, "process (%d) is not alive.", m_AttachedPid);
		return false;
	}
	
	size_t s_InputSize = p_Size;
	auto s_Ret = proc_rw_mem_pid(m_AttachedPid, reinterpret_cast<void*>(p_Address), p_Size, p_Data, &s_InputSize,  true);
	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "proc_rw_mem_pid ret: (%d) could not read process (%d) memory (%p) size: (%x) inputSize: (%llx)", s_Ret, m_AttachedPid, p_Address, p_Size, s_InputSize);
		return false;
	}

	return s_InputSize;
}

bool Debugger2::ProtectProcessMemory(uint64_t p_Address, uint32_t p_Size, int32_t p_Protection)
{
    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto s_ProcessMainThread = GetProcessMainThread();

    uint64_t s_PageStart = p_Address & ~(PAGE_SIZE - 1);

    WriteLog(LL_Debug, "pageStart: %llx", s_PageStart);
    
    auto s_Ret = kmprotect_t(reinterpret_cast<void*>(s_PageStart), PAGE_SIZE, p_Protection, s_ProcessMainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not mprotect (%d).", s_Ret);
        return false;
    }

    return true;
}

struct thread* Debugger2::GetProcessMainThread()
{
    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(m_AttachedPid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_AttachedPid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    return s_Process->p_singlethread != nullptr ? s_Process->p_singlethread :s_Process->p_threads.tqh_first;
}

bool Debugger2::GetProcessFullInfo (DbgProcessFull* p_Info)
{
    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid process info");
        return false;
    }

    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);

    struct proc* s_Process = pfind(m_AttachedPid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_AttachedPid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    // Initialize our message
    *p_Info = DBG_PROCESS_FULL__INIT;

    // Get the threads
    uint64_t s_ThreadCount = 0;
    struct thread* s_Thread = nullptr;
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        s_ThreadCount++;
    
    s_Thread = nullptr;

    DbgThreadLimited* s_Threads[s_ThreadCount];
    memset(s_Threads, 0, sizeof(s_Threads));

    uint64_t s_CurrentThreadIndex = 0;
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

        s_Threads[s_CurrentThreadIndex] = l_ThreadLimited;
        s_CurrentThreadIndex++;
    }
    p_Info->threads = s_Threads;
    p_Info->n_threads = s_ThreadCount;

    // Credentials
    DbgCred* s_Cred = new DbgCred();
    if (s_Cred == nullptr)
    {
        // TODO: Free/destroy the array
        WriteLog(LL_Error, "could not allocate credential");
        return false;
    }

    struct ucred* s_UCred = s_Process->p_ucred;
    if (s_UCred != nullptr)
    {
        s_Cred->effectiveuserid = s_UCred->cr_uid;
        s_Cred->realuserid = s_UCred->cr_ruid;
        s_Cred->saveduserid = s_UCred->cr_svuid;
        s_Cred->numgroups = s_UCred->cr_ngroups;
        s_Cred->realgroupid = s_UCred->cr_rgid;
        s_Cred->savedgroupid = s_UCred->cr_svgid;
        s_Cred->prison = reinterpret_cast<uint64_t>(s_UCred->cr_prison);
        s_Cred->sceauthid = s_UCred->cr_sceAuthID;

        s_Cred->scecaps = s_UCred->cr_sceCaps;
        s_Cred->n_scecaps = ARRAYSIZE(s_UCred->cr_sceCaps);

        s_Cred->sceattr = s_UCred->cr_sceAttr;
        s_Cred->n_sceattr = ARRAYSIZE(s_UCred->cr_sceAttr);
    }

    p_Info->pid = s_Process->p_pid;
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


    // Name
    auto s_NameLength = sizeof(s_Process->p_comm);
    auto s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        return false;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, s_Process->p_comm, s_NameLength);
    p_Info->name = s_Name;

    // Elf path
    auto s_ElfPathLength = sizeof(s_Process->p_elfpath);
    auto s_ElfPath = new char[s_ElfPathLength];
    if (s_ElfPath == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate memory for elf path");
        return false;
    }
    memset(s_ElfPath, 0, s_ElfPathLength);
    memcpy(s_ElfPath, s_Process->p_elfpath, s_ElfPathLength);
    p_Info->elfpath = s_ElfPath;
    
    // Randomized path
    auto s_RandomizedPathLength = s_Process->p_randomized_path_len;
    auto s_RandomizedPath = new char[s_RandomizedPathLength];
    if (s_RandomizedPath == nullptr)
    {
        // TODO: Free resources
        delete [] s_Name;
        WriteLog(LL_Error, "could not allocate memory for randomized path len: (%x)", s_RandomizedPathLength);
        return false;
    }
    memset(s_RandomizedPath, 0, s_RandomizedPathLength);
    memcpy(s_RandomizedPath, s_Process->p_randomized_path, s_RandomizedPathLength);

    p_Info->numthreads = s_Process->p_numthreads;

    // Get the VM Entries
    ProcVmMapEntry* s_Entries = nullptr;
    size_t s_NumEntries = 0;
    auto s_Ret = OrbisOS::Utilities::GetProcessVmMap(s_Process, &s_Entries, &s_NumEntries);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not get process vm map (%d)", s_Ret);
        return false;
    }

    DbgVmEntry* s_VmEntries[s_NumEntries];
    memset(&s_VmEntries, 0, sizeof(s_VmEntries));

    for (auto i = 0; i < s_NumEntries; ++i)
    {
        auto l_Entry = new DbgVmEntry();
        if (l_Entry == nullptr)
        {
            // TODO: Free resources
            WriteLog(LL_Error, "could not allocate dbg vm entry");
            continue;
        }
        memset(l_Entry, 0, sizeof(*l_Entry));

        *l_Entry = DBG_VM_ENTRY__INIT;
        auto l_NameLen = sizeof(s_Entries[i].name);
        auto l_Name = new char[l_NameLen];
        if (l_Name == nullptr)
        {
            // TODO: Free resources
            delete l_Entry;
            WriteLog(LL_Error, "could not allocate name (%d)", l_NameLen);
            continue;
        }
        memset(l_Name, 0, l_NameLen);
        memcpy(l_Name, s_Entries[i].name, l_NameLen);

        l_Entry->name = l_Name;

        l_Entry->start = s_Entries[i].start;
        l_Entry->end = s_Entries[i].end;
        l_Entry->offset = s_Entries[i].offset;
        l_Entry->protection = s_Entries[i].prot;

        s_VmEntries[i] = l_Entry;
    }

    return true;
}

bool Debugger2::GetProcessLimitedInfo(DbgProcessLimited* p_Info)
{
    if (p_Info == nullptr)
    {
        WriteLog(LL_Error, "invalid info");
        return false;
    }
    
    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(m_AttachedPid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_AttachedPid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    *p_Info = DBG_PROCESS_LIMITED__INIT;

    p_Info->pid = s_Process->p_pid;

    // Name
    auto s_NameLength = sizeof(s_Process->p_comm);
    auto s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        return false;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, s_Process->p_comm, s_NameLength);
    p_Info->name = s_Name;

    // Get the VM Entries
    ProcVmMapEntry* s_Entries = nullptr;
    size_t s_NumEntries = 0;
    auto s_Ret = OrbisOS::Utilities::GetProcessVmMap(s_Process, &s_Entries, &s_NumEntries);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not get process vm map (%d)", s_Ret);
        return false;
    }

    DbgVmEntry* s_VmEntries[s_NumEntries];
    memset(&s_VmEntries, 0, sizeof(s_VmEntries));

    for (auto i = 0; i < s_NumEntries; ++i)
    {
        auto l_Entry = new DbgVmEntry();
        if (l_Entry == nullptr)
        {
            // TODO: Free resources
            WriteLog(LL_Error, "could not allocate dbg vm entry");
            continue;
        }
        memset(l_Entry, 0, sizeof(*l_Entry));

        *l_Entry = DBG_VM_ENTRY__INIT;
        auto l_NameLen = sizeof(s_Entries[i].name);
        auto l_Name = new char[l_NameLen];
        if (l_Name == nullptr)
        {
            // TODO: Free resources
            delete l_Entry;
            WriteLog(LL_Error, "could not allocate name (%d)", l_NameLen);
            continue;
        }
        memset(l_Name, 0, l_NameLen);
        memcpy(l_Name, s_Entries[i].name, l_NameLen);

        l_Entry->name = l_Name;

        l_Entry->start = s_Entries[i].start;
        l_Entry->end = s_Entries[i].end;
        l_Entry->offset = s_Entries[i].offset;
        l_Entry->protection = s_Entries[i].prot;

        s_VmEntries[i] = l_Entry;
    }

    delete [] s_Entries;
    s_Entries = nullptr;

    p_Info->entries = s_VmEntries;
    p_Info->n_entries = s_NumEntries;

    return true;
}

bool Debugger2::GetThreadLimitedInfo(struct thread* p_Thread, DbgThreadLimited* p_Info)
{
    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

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

    *p_Info = DBG_THREAD_LIMITED__INIT;
    p_Info->errno = p_Thread->td_errno;
    p_Info->kernelstack = p_Thread->td_kstack;
    p_Info->kernelstackpages = p_Thread->td_kstack_pages;
    p_Info->retval = p_Thread->td_retval[0];
    p_Info->threadid = p_Thread->td_tid;

    return true;
}

bool Debugger2::GetThreadFullInfo(struct thread* p_Thread, DbgThreadFull* p_Info)
{
     if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

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

    *p_Info = DBG_THREAD_FULL__INIT;
    p_Info->proc = reinterpret_cast<uint64_t>(p_Thread->td_proc);
    p_Info->threadid = p_Thread->td_tid;

    // Name
    auto s_NameLength = sizeof(p_Thread->td_name);
    auto s_Name = new char[s_NameLength];
    if (s_Name == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate memory for name len: (%x)", s_NameLength);
        return false;
    }
    memset(s_Name, 0, s_NameLength);
    memcpy(s_Name, p_Thread->td_name, s_NameLength);
    p_Info->name = s_Name;

    p_Info->retval = p_Thread->td_retval[0];
    p_Info->kernelstack = p_Thread->td_kstack;
    p_Info->kernelstackpages = p_Thread->td_kstack_pages;
    p_Info->errno = p_Thread->td_errno;

    auto s_Ret = kkill_t(m_AttachedPid, SIGSTOP, s_MainThread);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not stop process (%d).", s_Ret);
        return false;
    }

    struct reg s_GpRegisters;
    memset(&s_GpRegisters, 0, sizeof(s_GpRegisters));
    s_Ret = kptrace_t(PT_GETREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_GpRegisters), 0, s_MainThread);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not get gp regs (%d).", s_Ret);
        return false;
    }

    DbgGpRegisters* s_DbgGpRegisters = new DbgGpRegisters;
    if (s_DbgGpRegisters == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate dbg gp registers");
        return false;
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

    p_Info->gpregisters = s_DbgGpRegisters;

    struct fpreg s_FpRegisters;
    memset(&s_FpRegisters, 0, sizeof(s_FpRegisters));
    s_Ret = kptrace_t(PT_GETFPREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_FpRegisters), 0, s_MainThread);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not get fp regs (%d).", s_Ret);
        return false;
    }

    DbgFpRegisters* s_DbgFpRegisters = new DbgFpRegisters;
    if (s_DbgFpRegisters == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate dbg fp register");
        return false;
    }
    *s_DbgFpRegisters = DBG_FP_REGISTERS__INIT;
    s_DbgFpRegisters->data.data = reinterpret_cast<uint8_t*>(&s_FpRegisters);
    s_DbgFpRegisters->data.len = sizeof(s_FpRegisters);

    p_Info->fpregisters = s_DbgFpRegisters;

    struct dbreg s_DbRegisters;
    memset(&s_DbRegisters, 0, sizeof(s_DbRegisters));
    s_Ret = kptrace_t(PT_GETDBREGS, m_AttachedPid, reinterpret_cast<caddr_t>(&s_DbRegisters), 0, s_MainThread);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not get db regs (%d).", s_Ret);
        return false;
    }

    DbgDbRegisters* s_DbgDbRegisters = new DbgDbRegisters;
    if (s_DbgDbRegisters == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate dbg db registers");
        return false;
    }
    *s_DbgDbRegisters = DBG_DB_REGISTERS__INIT;

    auto s_DrLen = ARRAYSIZE(s_DbRegisters.dr);
    uint32_t* s_Dr = new uint32_t[s_DrLen];
    if (s_Dr == nullptr)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not allocate debug registers");
        return false;
    }

    // Copy this garbage
    for (auto i = 0; i < s_DrLen; ++i)
        s_Dr[i] = s_DbRegisters.dr[i];
    
    s_DbgDbRegisters->debugregs = s_Dr;

    p_Info->dbregisters = s_DbgDbRegisters;

    s_Ret = kkill_t(m_AttachedPid, SIGCONT, s_MainThread);
    if (s_Ret < 0)
    {
        // TODO: Free resources
        WriteLog(LL_Error, "could not resume process (%d).", s_Ret);
        return false;
    }

    return true;
}

uint64_t Debugger2::AllocateProcessMemory(uint32_t p_Size, bool p_Zero)
{
    auto s_Thread = GetProcessMainThread();
    if (s_Thread == nullptr)
        return 0;

    auto s_Ret = kmmap_t(0, p_Size, PROT_READ | PROT_WRITE, MAP_SHARED, 0, 0, s_Thread);
    if ((int64_t)s_Ret < 0)
    {
        WriteLog(LL_Error, "could not allocate memory in process (%lld).", s_Ret);
        return 0;
    }

    if (p_Zero)
    {
        // TODO: Zero memory in the process
    }

    return reinterpret_cast<uint64_t>(s_Ret);
}

bool Debugger2::FreeProcessMemory(uint64_t p_Address, uint32_t p_Size)
{
    if (p_Address == 0)
        return false;
    
    auto s_Thread = GetProcessMainThread();
    if (s_Thread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    auto s_Ret = kmunmap_t(reinterpret_cast<void*>(p_Address), p_Size, s_Thread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not unmap (%p) with size (%x) ret: (%d).", p_Address, p_Size, s_Ret);
        return false;
    }

    return s_Ret == 0;
}

bool Debugger2::Attach(int32_t p_ProcessId, int32_t* p_Status)
{
    // Check that we have a valid process id number
    if (p_ProcessId < 0)
    {
        if (p_Status)
            *p_Status = -EEXIST;
        WriteLog(LL_Error, "invalid process id");
        return false;
    }
    
    // Bail if the process does not exist
    if (!IsProcessAlive(p_ProcessId))
    {
        if (p_Status)
            *p_Status = -EEXIST;
        
        WriteLog(LL_Error, "pid (%d) is not alive", p_ProcessId);
        return false;
    }
    
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        if (p_Status)
            *p_Status = -EIO;
        
        WriteLog(LL_Error, "could not get main thread to attach");
        return false;
    }

    auto s_Ret = kptrace_t(PT_ATTACH, p_ProcessId, 0, 0, s_MainThread);
    if (s_Ret < 0)
    {
        if (p_Status)
            *p_Status = s_Ret;
        
        WriteLog(LL_Error, "could not attach to pid (%d), ret: (%d)", p_ProcessId, s_Ret);
        return false;
    }

    // Update our debugger pid, now all functions should be available
    m_AttachedPid = p_ProcessId;

    // Attempt to continue the process that we attached to
    s_Ret = kptrace_t(PT_CONTINUE, m_AttachedPid, reinterpret_cast<caddr_t>(1), 0, s_MainThread);
    if (s_Ret < 0)
    {
        if (p_Status)
            *p_Status = s_Ret;
        
        Detach();
        WriteLog(LL_Error, "could not continue attached pid (%d), err: (%d).", m_AttachedPid, s_Ret);
        return false;
    }

    if (p_Status)
        *p_Status = 0;

    return true;
}

bool Debugger2::Detach(bool p_Force, int32_t* p_Status)
{
    if (m_AttachedPid < 0)
    {
        if (p_Status)
            *p_Status = -EEXIST;
        
        return false;
    }
    
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        if (p_Status)
            *p_Status = -EEXIST;

        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    // Stop the process, so we can detach
    auto s_Ret = kkill_t(m_AttachedPid, SIGSTOP, s_MainThread);
    if (s_Ret < 0)
    {
        if (p_Status)
            *p_Status = s_Ret;

        WriteLog(LL_Error, "could not stop the pid (%d).", m_AttachedPid);
        
        // Don't return if we are forced
        if (!p_Force)
            return false;
    }

    // Detach from the process
    s_Ret = kptrace_t(PT_DETACH, m_AttachedPid, 0, 0, s_MainThread);
    if (s_Ret < 0)
    {
        if (p_Status)
            *p_Status = s_Ret;

        WriteLog(LL_Error, "could not detach from pid (%d), ret: (%d).", m_AttachedPid, s_Ret);
        
        // Don't return if we are forcing cleanup of the debugger
        if (!p_Force)
            return false;
    }

    m_AttachedPid = -1;

    if (p_Status)
        *p_Status = 0;

    return true;
}

bool Debugger2::GetProcessThreads(DbgThreadLimited** p_OutThreads, uint32_t p_ThreadCount)
{
    if (p_OutThreads == nullptr)
        return false;
    
    // Zero out the entire allocated array
    for (auto i = 0; i < p_ThreadCount; ++i)
        p_OutThreads[i] = nullptr;

    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);
    struct proc* s_Process = pfind(m_AttachedPid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_AttachedPid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    uint64_t s_ThreadCount = 0;
    struct thread* s_Thread = nullptr;
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        s_ThreadCount++;
    
    s_Thread = nullptr;

    uint64_t s_CurrentThreadIndex = 0;
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
    {
        thread_lock(s_Thread);

        if (s_CurrentThreadIndex >= s_ThreadCount)
        {
            thread_unlock(s_Thread);
            WriteLog(LL_Error, "there was a new thread spun up during iteration");
            break;
        }

        DbgThreadLimited* l_Thread = new DbgThreadLimited;
        if (l_Thread == nullptr)
        {
            thread_unlock(s_Thread);
            s_CurrentThreadIndex++;
            WriteLog(LL_Error, "could not allocate thread limited");
            continue;
        }
        
        if (!GetThreadLimitedInfo(s_Thread, l_Thread))
        {
            thread_unlock(s_Thread);

            delete l_Thread;
            s_CurrentThreadIndex++;
            WriteLog(LL_Error, "could not set thread limited info");
            continue;
        }

        thread_unlock(s_Thread);
        p_OutThreads[s_CurrentThreadIndex] = l_Thread;
        s_CurrentThreadIndex++;
    }

    return true;
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

void Debugger2::OnAttach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgAttachRequest* s_Request = dbg_attach_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack attach request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    WriteLog(LL_Info, "request to attach to (%d).", s_Request->pid);

    int32_t s_Ret = -1;
    if (!s_Debugger->Attach(s_Request->pid, &s_Ret))
    {
        dbg_attach_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "could not attach ret (%d)", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    WriteLog(LL_Debug, "attached to: (%d).", s_Debugger->m_AttachedPid);

    dbg_attach_request__free_unpacked(s_Request, nullptr);
    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_Attach, s_Debugger->m_AttachedPid, nullptr, 0);
}

void Debugger2::OnDetach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgDetachRequest* s_Request = dbg_detach_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack detach request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    int32_t s_Ret = -1;
    if (!s_Debugger->Detach(s_Request->force, &s_Ret))
    {
        dbg_detach_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "could not detach ret (%d)", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    WriteLog(LL_Debug, "successfully detached");
    dbg_detach_request__free_unpacked(s_Request, nullptr);
    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_Attach, 0, nullptr, 0);
}

void Debugger2::OnGetProcList(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    
}

void Debugger2::OnReadProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgReadProcessMemoryRequest* s_Request = dbg_read_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack rpm request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    // Get the address
    auto s_Address = s_Request->address;
    if (s_Address == 0)
    {
        WriteLog(LL_Error, "invalid address");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EINVAL);
        return;
    }

    // Allocate buffer for the response
    size_t s_BufferSize = s_Request->size;
    auto s_Buffer = new uint8_t[s_BufferSize];
    if (s_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate buffer of size (%x).", s_BufferSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_Buffer, 0, s_BufferSize);

    auto s_Ret = proc_rw_mem_pid(s_Debugger->m_AttachedPid, reinterpret_cast<void*>(s_Request->address), s_Request->size, s_Buffer, &s_BufferSize, 0);
    if (s_Ret < 0)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not read (%p) size (%x).", s_BufferSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgReadProcessMemoryResponse s_Response = DBG_READ_PROCESS_MEMORY_RESPONSE__INIT;
    s_Response.data.data = s_Buffer;
    s_Response.data.len = s_BufferSize;

    auto s_SerializedSize = dbg_read_process_memory_response__get_packed_size(&s_Response);
    if (s_SerializedSize == 0)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "invalid packed size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_SerializedData = new uint8_t[s_SerializedSize];
    if (s_SerializedData == nullptr)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not allocate serialized data of size (%x).", s_SerializedSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_SerializedData, 0, s_SerializedSize);

    auto s_PackedRet = dbg_read_process_memory_response__pack(&s_Response, s_SerializedData);
    if (s_PackedRet != s_SerializedSize)
    {
        delete [] s_SerializedData;
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not allocate serialized data of size (%x).", s_SerializedSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_ReadMem, 0, s_SerializedData, s_SerializedSize);

    delete [] s_SerializedData;
    delete [] s_Buffer;
}