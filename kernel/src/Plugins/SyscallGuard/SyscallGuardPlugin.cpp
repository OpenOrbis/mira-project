// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SyscallGuardPlugin.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/_Syscall.hpp>

#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

using namespace Mira::Plugins;

SyscallGuard::SyscallGuard() :
    m_SyscallCount(0),
    m_CallStatuses(nullptr),
    m_SyCalls(nullptr)
{
    // Ensure that this only takes up 1 byte
    static_assert(sizeof(CallStatus) == 1, "CallStatus size incorrect");

    // Get the sysvec
    auto s_SysVec = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    if (s_SysVec == nullptr)
    {
        WriteLog(LL_Error, "could not get the sysvec");
        return;
    }
    
    // Get the syscall count
    auto s_SyscallCount = s_SysVec->sv_size;

    // Ensure that we only use up to 1MiB of size
    if (s_SyscallCount > SyscallCount_Max)
    {
        WriteLog(LL_Error, "syscall count (%d) > max (%d), check kernel or increase limits.", s_SyscallCount, SyscallCount_Max);
        return;
    }
    
    // Update the syscall count
    m_SyscallCount = s_SyscallCount;
    WriteLog(LL_Info, "Current syscall count: (%d).", m_SyscallCount);

    // Allocate the call statuses
    m_CallStatuses = new CallStatus[m_SyscallCount];
    if (m_CallStatuses == nullptr)
    {
        WriteLog(LL_Error, "could not allocate call statuses");
        m_SyscallCount = 0;
        return;
    }

    m_SyCalls = new sy_call_t*[m_SyscallCount];
    if (m_SyCalls == nullptr)
    {
        WriteLog(LL_Error, "Could not allocate sy_call table.");
        m_SyscallCount = 0;
        return;
    }
    memset(m_SyCalls, 0, sizeof(void*) * m_SyscallCount);

    // TODO: Check to see if users have all syscalls enabled

    // Start with a all disabled
    for (auto i = 0; i < s_SyscallCount; ++i)
        m_CallStatuses[i] = CallStatus::Status_Disabled;
    
    // Backup the current syscall table
    for (auto i = 0; i < m_SyscallCount; ++i)
        m_SyCalls[i] = s_SysVec->sv_table[i].sy_call;
    
    // Replace all syscall handlers with our own
    for (auto i = 0; i < m_SyscallCount; ++i)
        s_SysVec->sv_table[i].sy_call = SyscallGuard::SyscallHandler;
    
    WriteLog(LL_Info, "syscall guard initialized");
}

SyscallGuard::~SyscallGuard()
{

}

bool SyscallGuard::SetReservedSyscall(int p_SyscallNumber, sy_call_t p_Call)
{
    if (p_Call == nullptr)
        return false;
    
    if (p_SyscallNumber <= NOSYS || p_SyscallNumber > m_SyscallCount)
        return false; 
    
    if (m_CallStatuses == nullptr)
        return false;

    if (m_SyCalls == nullptr)
        return false;
    
    m_CallStatuses[p_SyscallNumber] = CallStatus::Status_Reserved;
    m_SyCalls[p_SyscallNumber] = p_Call;

    return true;
}

int SyscallGuard::SyscallHandler(struct thread * p_Thread, void * p_Uap)
{
    // Get the syscall number from eax (hopefully)?
    int s_SyscallNumber = -1;
    asm(
        "mov %%eax, %0"
        : "=r"(s_SyscallNumber) // Output
        :                       // Input
        :                       // Clobbered
    );

    WriteLog(LL_Info, "syscall (%d) called.", s_SyscallNumber);

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return EPERM;
    }

    // Get the syscall guard reference
    auto s_SyscallGuard = static_cast<Plugins::SyscallGuard*>(s_Framework->GetPluginManager()->GetSyscallGuard());
    if (s_SyscallGuard == nullptr)
    {
        WriteLog(LL_Error, "could not get the syscall guard.");
        return EPERM;
    }

    // Get the main mira process
    auto s_MiraProcess = s_Framework->GetInitParams()->process;
    if (s_MiraProcess == nullptr)
    {
        WriteLog(LL_Error, "could not get the mira process.");
        return EPERM;
    }

    // Validate that our syscall number is valid
    if (s_SyscallNumber < NOSYS || s_SyscallNumber >= s_SyscallGuard->m_SyscallCount)
    {
        WriteLog(LL_Error, "invalid syscall called (%d).", s_SyscallNumber);
        return EPERM;
    }

    // Get the call from our list
    auto s_Call = s_SyscallGuard->m_SyCalls[s_SyscallNumber];
    if (s_Call == nullptr)
    {
        WriteLog(LL_Error, "could not get a sy_call for syscall (%d).", s_SyscallNumber);
        return EPERM;
    }
    
    // Get the status
    auto s_Status = s_SyscallGuard->m_CallStatuses[s_SyscallNumber];
    switch (s_Status)
    {
    case CallStatus::Status_Reserved:
        // Check to make sure that reserved syscalls are only called by mira
        if (p_Thread->td_proc != s_MiraProcess)
        {
            WriteLog(LL_Error, "syscall (%d) was called by a non-whitelisted process (%d) (%s).", s_SyscallNumber, p_Thread->td_proc->p_pid, p_Thread->td_proc->p_comm);
            return EPERM;
        }
        break;
    case CallStatus::Status_Enabled:
        // Do nothing, allow
        break;
    case CallStatus::Status_Disabled:
        WriteLog(LL_Error, "syscall (%d) disabled.", s_SyscallNumber);
        return EPERM;
    default:
        WriteLog(LL_Error, "invalid syscall (%d) status (%d).", s_SyscallNumber, s_Status);
        return EPERM;
    }

    // Call and return the original call (or replaced in case of reserved)
    return s_Call(p_Thread, p_Uap);
}