#include "System.hpp"
#include <Utils/Logger.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/proc.h>
    #include <vm/vm.h>
    #include <vm/vm_map.h>
}
using namespace Mira::Driver;

int32_t SystemDriverCtl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    return 0;
}

void* SystemDriverCtl::ProcessAllocate(struct proc* p_Process, uint64_t p_Size)
{
    uint64_t s_PageAlignedSize = (p_Size + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
    int32_t s_ProcessId = p_Process->p_pid;

    // Get the vmspace
    struct vmspace* s_VmSpace = p_Process->p_vmspace;
    if (s_VmSpace == nullptr)
    {
        WriteLog(LL_Error, "could not get process (%d) vmspace.", s_ProcessId);
        return nullptr;
    }

    struct vm_map* s_VmMap = &s_VmSpace->vm_map;
    if (s_VmMap == nullptr)
    {
        WriteLog(LL_Error, "could not get vm map for proc (%d).", s_ProcessId);
        return nullptr;
    }
}