#include "ThreadManager.hpp"

#include <Utils/Kdlsym.hpp>

#include <sys/proc.h>

using namespace Mira::Threading;

ThreadManager::ThreadManager()
{
    // Initialize the mutex
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    mtx_init(&m_ThreadsLock, "ThreadMgr", nullptr, 0);

}

ThreadManager::~ThreadManager()
{

}

void* ThreadManager::CreateKernelThread(struct thread* p_OwningThread, void* p_EntryPoint, const char* p_ThreadName, void* p_Arguments)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

    if (p_OwningThread == nullptr || p_EntryPoint == nullptr)
        return nullptr;
    
    int32_t s_ProcessId = -1;
    if (p_OwningThread->td_proc)
        s_ProcessId = p_OwningThread->td_proc->p_pid;
    
    struct thread* s_Thread = nullptr;

    int s_Result = kthread_add((void(*)(void*))(p_EntryPoint), p_Arguments, curthread->td_proc, &s_Thread, 0, 0, p_ThreadName == nullptr ? "tm_kthr" : p_ThreadName);
    if (s_Result != 0)
        return nullptr;
    
    WriteLog(LL_Debug, "thread created (preCount: %d).", m_Threads.size());

    _mtx_lock_flags(&m_ThreadsLock, 0, __FILE__, __LINE__);
    m_Threads.push_back
    (
        ThreadContainer
        (
            ThreadType_Kernel, 
            s_ProcessId, 
            s_Thread, 
            p_OwningThread
        )
    );
    _mtx_unlock_flags(&m_ThreadsLock, 0, __FILE__, __LINE__);

    return s_Thread;
}

void* ThreadManager::CreateUserThread(int32_t p_ProcessId, void* p_EntryPoint, void* p_Arguments)
{
    WriteLog(LL_Warn, "CreateUserThread is not implemented yet");
    return nullptr;
}

bool ThreadManager::OnSuspend()
{
    WriteLog(LL_Debug, "Suspending all owned threads");
    return true;
}

bool ThreadManager::OnResume()
{
    WriteLog(LL_Debug, "Resuming all owned threads");
    return true;
}