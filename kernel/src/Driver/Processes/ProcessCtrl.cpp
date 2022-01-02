#include "ProcessCtrl.hpp"
#include <mira/Driver/DriverStructs.hpp>
#include <OrbisOS/Utilities.hpp>

using namespace Mira::Driver::Processes;

bool ProcessCtrl::GetProcessList()
{
    const uint32_t c_ProcessListHeaderSize = sizeof(s_ProcessListHeader);
    const uint32_t c_PidOffset = offsetof(MiraProcessList, ProcessIds);

    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
    auto _sx_slock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_slock);
	auto _sx_sunlock = (void(*)(struct sx *sx, const char *file, int line))kdlsym(_sx_sunlock);

    struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);
    struct sx* allproclock = (struct sx*)kdlsym(allproc_lock);

    struct thread* p_Thread = nullptr;
    struct proc* myProcess = p_Thread->td_proc;
    int32_t* s_UserProcessIdList = (int32_t*)c_PidOffset;

    uint32_t s_CurrentPidIndex = 0;
    struct proc* p = NULL;
    _sx_slock(allproclock, 0, __FILE__, __LINE__);
    FOREACH_PROC_IN_SYSTEM(p)
    {
        _mtx_lock_flags(&p->p_mtx, 0);
        size_t s_PidSize = sizeof(p->p_pid);
        Mira::OrbisOS::Utilities::ProcessReadWriteMemory(myProcess, s_UserProcessIdList + s_CurrentPidIndex, sizeof(p->p_pid), &p->p_pid, &s_PidSize, true);
        _mtx_unlock_flags(&p->p_mtx, 0);

        ++s_CurrentPidIndex;
    }
    _sx_sunlock(allproclock, __FILE__, __LINE__);

    return false;
}