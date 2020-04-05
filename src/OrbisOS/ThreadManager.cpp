#include "ThreadManager.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kernel.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/proc.h>
    #include <sys/filedesc.h>
};

using namespace Mira::OrbisOS;

ThreadManager::ThreadManager() :
    m_ThreadManagerRunning(false),
    m_IoThreadRunning(false),
    m_DebugThreadRunning(false),
    m_IoThread(nullptr),
    m_DebugThread(nullptr)
{

}

ThreadManager::~ThreadManager()
{
    m_IoThreadRunning = false;
}

bool ThreadManager::IsSyscoreAlive()
{
    struct proc* s_SyscoreProc = OrbisOS::Utilities::FindProcessByName("SceSysCore");

    return s_SyscoreProc != nullptr;
}

bool ThreadManager::IsShellcoreAlive()
{
    struct proc* s_ShellcoreProc = OrbisOS::Utilities::FindProcessByName("SceShellCore");

    return s_ShellcoreProc != nullptr;
}

struct thread* ThreadManager::GetSyscoreMainThread()
{
    auto s_SyscoreProcess = OrbisOS::Utilities::FindProcessByName("SceSysCore");
    if (s_SyscoreProcess == nullptr)
        return nullptr;
        
    return s_SyscoreProcess->p_singlethread ? s_SyscoreProcess->p_singlethread : s_SyscoreProcess->p_threads.tqh_first;
}

struct thread* ThreadManager::GetShellcoreMainThread()
{
    auto s_ShellcoreProcess = OrbisOS::Utilities::FindProcessByName("SceShellCore");
    if (s_ShellcoreProcess == nullptr)
        return nullptr;
        
    return s_ShellcoreProcess->p_singlethread ? s_ShellcoreProcess->p_singlethread : s_ShellcoreProcess->p_threads.tqh_first;
}

struct thread* ThreadManager::GetMiraMainThread()
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return nullptr;
    
    return s_Framework->GetMainThread();
}

bool ThreadManager::OnLoad()
{
    // This does not need kthread_exit as it is called "flatly"
    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

    auto s_MiraMainThread = GetMiraMainThread();
    if (s_MiraMainThread == nullptr)
        return false;

    auto s_MiraProc = s_MiraMainThread->td_proc;
    if (s_MiraProc == nullptr)
        return false;
    
    // Update the thread manager
    m_ThreadManagerRunning = true;

    // Create the io and debug thread
    auto s_IoThreadRet = kthread_add(ThreadManager::FileIoThread, this, s_MiraProc, &m_IoThread, 0, 200, "miraIo");
    auto s_DebugThreadRet = kthread_add(ThreadManager::DebuggerThread, this, s_MiraProc, &m_DebugThread, 0, 200, "miraDbg");

    WriteLog(LL_Debug, "IoThread (%d) DebugThread (%d).", s_IoThreadRet, s_DebugThreadRet);

    return !s_IoThreadRet && !s_DebugThreadRet;
}

bool ThreadManager::OnUnload()
{
    return true;
}

bool ThreadManager::OnSuspend()
{
    return true;
}

bool ThreadManager::OnResume()
{
    return true;
}

struct thread* ThreadManager::GetDebuggerThread()
{
    return m_DebugThread;
}

struct thread* ThreadManager::GetFileIoThread()
{
    return m_IoThread;
}

void ThreadManager::FileIoThread(void* p_Argument)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
    auto avcontrol_sleep = (void(*)(int milliseconds))kdlsym(avcontrol_sleep);

    // Get the thread manager
    auto s_ThreadManager = static_cast<ThreadManager*>(p_Argument);
    if (!s_ThreadManager)
    {
        WriteLog(LL_Error, "could not get thread manager");
        kthread_exit();
        return;
    }

    // Create new credentials
	auto s_RetVal = ksetuid_t(0, curthread);
    if (s_RetVal < 0)
    {
        WriteLog(LL_Debug, "could not create new credentials (%s).", curthread->td_name);
        kthread_exit();
        return;
    }

    // Root and escape our thread
    auto s_Cred = curthread->td_ucred;
	if (s_Cred)
	{
		WriteLog(LL_Info, "escaping thread (%s).", curthread->td_name);

		s_Cred->cr_rgid = 0;
		s_Cred->cr_svgid = 0;
		
		s_Cred->cr_uid = 0;
		s_Cred->cr_ruid = 0;

        s_Cred->cr_prison = *(struct prison**)kdlsym(prison0);

        // Set our auth id as shellcore
		s_Cred->cr_sceAuthID = SceAuthenticationId::SceShellCore;

		// give maximum credentials
        for (auto i = 0; i < ARRAYSIZE(s_Cred->cr_sceCaps); ++i)
		    s_Cred->cr_sceCaps[i] = SceCapabilites::Max;
    }

    // Get the parent proc, this should always be Mira
    auto s_ParentProc = curthread->td_proc;
    if (s_ParentProc)
    {
        auto s_FileDesc = s_ParentProc->p_fd;
        if (s_FileDesc)
        {
            s_FileDesc->fd_rdir = *(struct vnode**)kdlsym(rootvnode);
            s_FileDesc->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
        }
    }
    WriteLog(LL_Debug, "new credentials set for (%s).", curthread->td_name);
    
    s_ThreadManager->m_IoThreadRunning = true;
    
    while (s_ThreadManager->m_ThreadManagerRunning)
        avcontrol_sleep(100);

    s_ThreadManager->m_IoThreadRunning = false;
    s_ThreadManager->m_IoThread = nullptr; // is this legal?

    kthread_exit();
}

void ThreadManager::DebuggerThread(void* p_Argument)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
    auto avcontrol_sleep = (void(*)(int milliseconds))kdlsym(avcontrol_sleep);

    // Get the thread manager
    auto s_ThreadManager = static_cast<ThreadManager*>(p_Argument);
    if (!s_ThreadManager)
    {
        WriteLog(LL_Error, "could not get thread manager");
        kthread_exit();
        return;
    }

    // Create new credentials
	auto s_RetVal = ksetuid_t(0, curthread);
    if (s_RetVal < 0)
    {
        WriteLog(LL_Debug, "could not create new credentials (%s).", curthread->td_name);
        kthread_exit();
        return;
    }

    // Root and escape our thread
    auto s_Cred = curthread->td_ucred;
	if (s_Cred)
	{
		WriteLog(LL_Info, "escaping thread (%s).", curthread->td_name);

		s_Cred->cr_rgid = 0;
		s_Cred->cr_svgid = 0;
		
		s_Cred->cr_uid = 0;
		s_Cred->cr_ruid = 0;

        s_Cred->cr_prison = *(struct prison**)kdlsym(prison0);

        // Set our auth id as shellcore
		s_Cred->cr_sceAuthID = SceAuthenticationId::Decid;

		// give maximum credentials
        for (auto i = 0; i < ARRAYSIZE(s_Cred->cr_sceCaps); ++i)
		    s_Cred->cr_sceCaps[i] = SceCapabilites::Max;
    }

    // Get the parent proc, this should always be Mira
    auto s_ParentProc = curthread->td_proc;
    if (s_ParentProc)
    {
        auto s_FileDesc = s_ParentProc->p_fd;
        if (s_FileDesc)
        {
            s_FileDesc->fd_rdir = *(struct vnode**)kdlsym(rootvnode);
            s_FileDesc->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
        }
    }
    WriteLog(LL_Debug, "new credentials set for (%s).", curthread->td_name);
    
    s_ThreadManager->m_DebugThreadRunning = true;
    
    while (s_ThreadManager->m_ThreadManagerRunning)
        avcontrol_sleep(100);

    s_ThreadManager->m_DebugThreadRunning = false;
    s_ThreadManager->m_DebugThread = nullptr;

    kthread_exit();
}