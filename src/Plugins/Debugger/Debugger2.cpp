#include "Debugger2.hpp"
#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;

Debugger2::Debugger2(uint16_t p_Port) :
    m_TrapFatalHook(nullptr),
    m_ServerAddress { 0 },
    m_Socket(-1),
    m_Port(p_Port),
    m_AttachedPid(-1)
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);

    mtx_init(&m_Mutex, "DbgLock", nullptr, MTX_SPIN);
}

Debugger2::~Debugger2()
{
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Echo, OnEcho);
}

#include <netinet/ip6.h>

bool Debugger2::ReplaceExceptionHandler(uint32_t p_ExceptionNumber, void* p_Function, void** p_PreviousFunction)
{
    if (p_Function == nullptr)
        return false;
    
    // void* s_Idt = nullptr;

    // auto setidt = (void(*)(int idx, void* func, int typ, int dpl, int ist))kdlsym(setidt);

    // setidt(IDT_DF, nullptr, SDT_SYSIGT, SEL_KPL, 0);

    // sizeof(struct ip6_hdr);

    return true;
}

bool Debugger2::OnLoad()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
	// Create the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = new Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(OnTrapFatal));
    
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        m_TrapFatalHook->Enable();
    }
#endif

    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ReadMem, OnReadProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_WriteMem, OnWriteProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ProtectMem, OnProtectProcessMemory);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ScanMem, OnScanMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcInfo, OnGetProcessInfo);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_AllocateProcMem, OnAllocateProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_FreeProcMem, OnFreeProcessMemory);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcMap, OnGetProcessMap);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Attach, OnAttach);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Detach, OnDetach);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Breakpoint, OnAddBreakpoint);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Watchpoint, OnAddWatchpoint);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcThreads, OnGetProcThreads);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_SignalProc, OnSignalProcess);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetRegs, OnGetThreadRegisters);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_SetRegs, OnSetThreadRegisters);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetThreadInfo, OnGetThreadInfo);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ThreadSinglestep, OnThreadSinglestep);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ReadKernelMem, OnReadKernelMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_WriteKernelMem, OnWriteKernelMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcList, OnGetProcList);

    return true;
}

bool Debugger2::OnUnload()
{
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ReadMem, OnReadProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_WriteMem, OnWriteProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ProtectMem, OnProtectProcessMemory);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ScanMem, OnScanMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcInfo, OnGetProcessInfo);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_AllocateProcMem, OnAllocateProcessMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_FreeProcMem, OnFreeProcessMemory);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcMap, OnGetProcessMap);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Attach, OnAttach);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Detach, OnDetach);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Breakpoint, OnAddBreakpoint);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_Watchpoint, OnAddWatchpoint);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcThreads, OnGetProcThreads);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_SignalProc, OnSignalProcess);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetRegs, OnGetThreadRegisters);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_SetRegs, OnSetThreadRegisters);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetThreadInfo, OnGetThreadInfo);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ThreadSinglestep, OnThreadSinglestep);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_ReadKernelMem, OnReadKernelMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_WriteKernelMem, OnWriteKernelMemory);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcList, OnGetProcList);

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
    WriteLog(LL_Info, "deleting trap fatal hook");
	if (m_TrapFatalHook != nullptr)
    {
        if (m_TrapFatalHook->IsEnabled())
            m_TrapFatalHook->Disable();
        
        delete m_TrapFatalHook;
        m_TrapFatalHook = nullptr;
    }
#endif

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
    // Verify that we have some kind of pid
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
    
    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        if (p_Status)
            *p_Status = -EIO;
		return false;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        if (p_Status)
            *p_Status = -EIO;
		return false;
	}

    auto s_Ret = kptrace_t(PT_ATTACH, p_ProcessId, 0, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        if (p_Status)
            *p_Status = s_Ret;
        
        WriteLog(LL_Error, "could not attach to pid (%d), ret: (%d)", p_ProcessId, s_Ret);
        return false;
    }

    int32_t s_Status = 0;
    s_Ret = kwait4_t(p_ProcessId, &s_Status, WUNTRACED, nullptr, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not wait for untraced pid (%d) ret (%d)", p_ProcessId, s_Ret);
        Detach();
        return false;
    }
    // TODO: Do we need to wait?
    // if (p_Status)
    //     *p_Status = s_Ret;

    // Update our debugger pid, now all functions should be available
    m_AttachedPid = p_ProcessId;

    // Attempt to continue the process that we attached to
    s_Ret = kptrace_t(PT_CONTINUE, m_AttachedPid, reinterpret_cast<caddr_t>(1), 0, s_DebuggerThread);
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

    // Stop the process, so we can detach
    auto s_Ret = kkill_t(m_AttachedPid, SIGSTOP, s_DebuggerThread);
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
    s_Ret = kptrace_t(PT_DETACH, m_AttachedPid, 0, 0, s_DebuggerThread);
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

bool Debugger2::Step()
{
    if (m_AttachedPid < 0)
        return false;

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
    
    auto s_Ret = kptrace_t(PT_STEP, m_AttachedPid, reinterpret_cast<caddr_t>(1), 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not step (%d).", s_Ret);
        return false;
    }

    return true;
}

bool Debugger2::Suspend()
{
    if (m_AttachedPid < 0)
        return false;

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
    
    auto s_Ret = kptrace_t(PT_SUSPEND, m_AttachedPid, 0, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not suspend (%d).", s_Ret);
        return false;
    }

    return true;
}

bool Debugger2::Resume()
{
    if (m_AttachedPid < 0)
        return false;

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
    
    auto s_Ret = kptrace_t(PT_RESUME, m_AttachedPid, 0, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not resume (%d).", s_Ret);
        return false;
    }

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

struct thread* Debugger2::GetThreadById(int32_t p_ThreadId)
{
    if (!IsProcessAlive(m_AttachedPid))
        return nullptr;
    
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);

	struct proc* s_Process = pfind(m_AttachedPid);
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
