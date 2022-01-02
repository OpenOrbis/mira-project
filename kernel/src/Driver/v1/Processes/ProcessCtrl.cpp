#include "ProcessCtrl.hpp"
#include <mira/Driver/DriverStructs.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Utils/Logger.hpp>
#include <Utils/System.hpp>

using namespace Mira::Driver::v1::Processes;

bool ProcessCtrl::GetProcessList()
{
    /*const uint32_t c_ProcessListHeaderSize = sizeof(s_ProcessListHeader);
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
    _sx_sunlock(allproclock, __FILE__, __LINE__);*/

    return false;
}
/*
bool CtrlDriver::GetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials*& p_Output)
{
    auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *, int, const char *, int))kdlsym(_thread_lock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_Output != nullptr)
        WriteLog(LL_Info, "output is filled in, is this a bug?");
    
    // Get the process
    auto s_Process = pfind(p_ProcessId);
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
        return false;
    }

    // On success this will be filled out
    MiraThreadCredentials* s_Credentials = nullptr;

    struct thread* l_Thread = nullptr;
    bool s_TidFound = false;
    FOREACH_THREAD_IN_PROC(s_Process, l_Thread)
    {        
        // Lock the thread
        thread_lock(l_Thread);

        WriteLog(LL_Debug, "checking tid: (%d) == (%d).", l_Thread->td_tid, p_ThreadId);

        // If the thread id's match then we need to copy this data out
        if (l_Thread->td_tid == p_ThreadId)
        {
            // We always want to break out of this so we unlock the thread
            do
            {
                // Check that we have a valid credential
                auto l_ThreadCredential = l_Thread->td_ucred;
                if (l_ThreadCredential == nullptr)
                {
                    WriteLog(LL_Error, "invalid ucred.");
                    break;
                }

                // Allocate new credentials
                s_Credentials = new MiraThreadCredentials;
                if (s_Credentials == nullptr)
                {
                    WriteLog(LL_Error, "could not allocate new credentials.");
                    break;
                }

                s_Credentials->State = GSState::Get;
                s_Credentials->ProcessId = p_ProcessId;
                s_Credentials->ThreadId = p_ThreadId;
                
                // ucred
                s_Credentials->EffectiveUserId = l_ThreadCredential->cr_uid;
                s_Credentials->RealUserId = l_ThreadCredential->cr_ruid;
                s_Credentials->SavedUserId = l_ThreadCredential->cr_svuid;
                s_Credentials->NumGroups = l_ThreadCredential->cr_ngroups;
                s_Credentials->RealGroupId = l_ThreadCredential->cr_rgid;
                s_Credentials->SavedGroupId = l_ThreadCredential->cr_svgid;

                // Check if this prison is rooted
                if (*(struct prison**)kdlsym(prison0) == l_ThreadCredential->cr_prison)
                    s_Credentials->Prison = MiraThreadCredentials::_MiraThreadCredentialsPrison::Root;
                else
                    s_Credentials->Prison = MiraThreadCredentials::_MiraThreadCredentialsPrison::Default;
                
                s_Credentials->SceAuthId = (SceAuthenticationId)(l_ThreadCredential->cr_sceAuthID);

                // Check that the sizes are the same and copy it as-is, idgaf
                static_assert(sizeof(s_Credentials->Capabilities) == sizeof(l_ThreadCredential->cr_sceCaps), "caps sizes don't match");
                memcpy(s_Credentials->Capabilities, l_ThreadCredential->cr_sceCaps, sizeof(s_Credentials->Capabilities));

                static_assert(sizeof(s_Credentials->Attributes) == sizeof(l_ThreadCredential->cr_sceAttr), "attribute sizes don't match");
                memcpy(s_Credentials->Attributes, l_ThreadCredential->cr_sceAttr, sizeof(s_Credentials->Attributes));

                s_TidFound = true;
            } while (false);
        }

        // Unlock the thread        
        thread_unlock(l_Thread);

        if (s_TidFound)
            break;
    }
    
    // Unlock the process
    _mtx_unlock_flags(&s_Process->p_mtx, 0);

    // Extra debugging output
    if (s_TidFound == false)
        WriteLog(LL_Error, "could not find thread id (%d).", p_ThreadId);

    // Check if we got any credentials
    if (s_Credentials == nullptr)
    {
        WriteLog(LL_Error, "could not find thread credentials.");
        return false;
    }

    // Assign
    p_Output = s_Credentials;

    return true;
}
*/

int32_t ProcessCtrl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    switch (p_Command)
    {
    case MIRA_PROCESS_ALLOCATE_MEMORY:
        return OnProcessAllocateMemory(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    case MIRA_PROCESS_FREE_MEMORY:
        return OnProcessFreeMemory(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    case MIRA_PROCESS_READ_MEMORY:
        return OnProcessReadMemory(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    case MIRA_PROCESS_WRITE_MEMORY:
        return OnProcessWriteMemory(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    // case MIRA_GET_PROC_INFORMATION:
    //     return OnMiraGetProcInformation(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    // case MIRA_GET_PID_LIST:
    //     return OnSystemGetProcessList(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    default:
        WriteLog(LL_Debug, "invalid ioctl command (%x).", p_Command);
        break;
    }

    return EINVAL;

}


int32_t ProcessCtrl::OnProcessAllocateMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    ProcessAllocateMemory s_AllocateMemory = { 0 };

    auto s_Ret = copyin(p_Data, &s_AllocateMemory, sizeof(s_AllocateMemory));
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "could not copyin allocation request.");
        return EINVAL;
    }

    // Add this default behavior where if its <= 0 then assume current pid
    if (s_AllocateMemory.ProcessId <= 0)
        s_AllocateMemory.ProcessId = p_Thread->td_proc->p_pid;

    s_AllocateMemory.Pointer = Utils::System::AllocateProcessMemory(s_AllocateMemory.ProcessId, s_AllocateMemory.Size, s_AllocateMemory.Protection);

    s_Ret = copyout(&s_AllocateMemory, p_Data, sizeof(s_AllocateMemory));
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "could not copyout allocation request.");
        return EFAULT;
    }

    return 0;
}

int32_t ProcessCtrl::OnProcessFreeMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    ProcessFreeMemory s_FreeMemory = { 0 };

    auto s_Ret = copyin(p_Data, &s_FreeMemory, sizeof(s_FreeMemory));
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "could not copyin free memory request.");
        return EINVAL;
    }

    // Default behavior where <= 0 is current proc
    if (s_FreeMemory.ProcessId <= 0)
        s_FreeMemory.ProcessId = p_Thread->td_proc->p_pid;
    
    Utils::System::FreeProcessMemory(s_FreeMemory.ProcessId, s_FreeMemory.Pointer, s_FreeMemory.Size);

    return 0;
}

int32_t ProcessCtrl::OnProcessReadMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    ProcessReadMemory s_ReadProcessMemory;

    auto s_Ret = copyin(p_Data, &s_ReadProcessMemory, sizeof(s_ReadProcessMemory));
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "copyin failed (%d).", s_Ret);
        return ENOMEM;
    }

    auto s_Size = s_ReadProcessMemory.DataSize;
    if (s_Size > PAGE_SIZE * 4)
    {
        WriteLog(LL_Error, "requested too large of size (%x).", s_Size);
        return EINVAL;
    }

    // Get the proc, the proc is locked at this point
    auto s_Proc = pfind(s_ReadProcessMemory.ProcessId);
    if (s_Proc == nullptr)
    {
        WriteLog(LL_Error, "could not find pid (%d).", s_ReadProcessMemory.ProcessId);
        return ESRCH;
    }

    do
    {
        if (!Utils::System::CopyProcessMemory(s_Proc, s_ReadProcessMemory.Address, p_Thread->td_proc, p_Data + offsetof(ProcessReadMemory, Data), s_Size))
        {
            WriteLog(LL_Error, "could not copy (%d: %p) -> (%d: %p)", s_Proc->p_pid, s_ReadProcessMemory.Address, p_Thread->td_proc->p_pid, p_Data + offsetof(ProcessReadMemory, Data));
            break;
        }
    } while (false);

    _mtx_unlock_flags(&s_Proc->p_mtx, 0);

    return 0;
}

int32_t ProcessCtrl::OnProcessWriteMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    ProcessWriteMemory s_WriteProcessMemory;

    auto s_Ret = copyin(p_Data, &s_WriteProcessMemory, sizeof(s_WriteProcessMemory));
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "copyin failed (%d).", s_Ret);
        return ENOMEM;
    }

    // Check the amount of data to write
    auto s_Size = s_WriteProcessMemory.DataSize;
    if (s_Size > PAGE_SIZE * 4)
    {
        WriteLog(LL_Error, "requested too large of size (%x).", s_Size);
        return EINVAL;
    }

    // Get the proc, the proc is locked at this point
    auto s_Proc = pfind(s_WriteProcessMemory.ProcessId);
    if (s_Proc == nullptr)
    {
        WriteLog(LL_Error, "could not find pid (%d).", s_WriteProcessMemory.ProcessId);
        return ESRCH;
    }

    // Copy from our proc to the other proc
    if (!Utils::System::CopyProcessMemory(p_Thread->td_proc, p_Data + offsetof(ProcessWriteMemory, Data), s_Proc, s_WriteProcessMemory.Address, s_Size))
        WriteLog(LL_Error, "could not copy (%d: %p) -> (%d: %p)", s_Proc->p_pid, s_WriteProcessMemory.Address, p_Thread->td_proc->p_pid, p_Data + offsetof(ProcessWriteMemory, Data));

    _mtx_unlock_flags(&s_Proc->p_mtx, 0);

    return 0;
}

/*

int32_t SystemDriverCtl::OnSystemGetProcessList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    // Check data
    // This will be a pointer to an output pointer
    // example: MiraProcessList* s_OutputAddress = nullptr;
    // example: ioctl(device, ioctl code, &s_OutputAddress, 0);
    // on success, there will be a pointer filled allocated by mmap
    // to free, call munmap

    // Validate the output pointer
    if (p_Data == nullptr)
    {
        WriteLog(LL_Error, "invalid output data");
        return (EINVAL);
    }

    MiraProcessList s_Input;
    auto s_Result = copyin(p_Data, &s_Input, sizeof(MiraProcessList));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin process list.");
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    MiraProcessList* s_Output = nullptr;
    if (!GetProcessList(s_Output))
    {
        WriteLog(LL_Error, "could not get the process list.");
        return (ENOMEM);
    }

    if (s_Output == nullptr)
    {
        WriteLog(LL_Error, "could not allocate the output.");
        return (ENOMEM);
    }

    if (s_Input.StructureSize < s_Output->StructureSize)
    {
        WriteLog(LL_Error, "input size (%d) < output size (%d).", s_Input.StructureSize, s_Output->StructureSize);
        delete [] s_Output;
        return (EMSGSIZE);
    }

    s_Result = copyout(s_Output, p_Data, s_Output->StructureSize);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyuout data (%d).", s_Result);
        delete [] s_Output;
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    delete [] s_Output;
    return 0;
}

// bool SystemDriverCtl::GetProcessListInProc(int32_t p_ProcessId, MiraProcessList*& p_OutputList)
// {
//     auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
//     auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
//     auto _sx_slock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_slock);
// 	auto _sx_sunlock = (void(*)(struct sx *sx, const char *file, int line))kdlsym(_sx_sunlock);

//     if (p_List != nullptr)
//         WriteLog(LL_Warn, "process list already filled in, is this a bug?");
    
//     Vector<int32_t> s_Pids;

//     struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);
//     struct sx* allproclock = (struct sx*)kdlsym(allproc_lock);

//     struct proc* p = NULL;
//     _sx_slock(allproclock, 0, __FILE__, __LINE__);
//     FOREACH_PROC_IN_SYSTEM(p)
//     {
//         _mtx_lock_flags(&p->p_mtx, 0);
//         s_Pids.push_back(p->p_pid);
//         _mtx_unlock_flags(&p->p_mtx, 0);
//     }
//     _sx_sunlock(allproclock, __FILE__, __LINE__);

//     auto s_Count = s_Pids.size();
//     if (s_Count == 0 || s_Count > 200)
//     {
//         WriteLog(LL_Error, "invalid count of pids (%d).", s_Count);
//         return false;
//     }

//     // Create a temporary buffer so we only need one copyout
//     auto s_ListSize = sizeof(MiraProcessList) + (sizeof(int32_t) * s_Count);
//     MiraProcessList* s_List = reinterpret_cast<MiraProcessList*>(new uint8_t[s_ListSize]);
//     if (s_List == nullptr)
//     {
//         WriteLog(LL_Error, "could not allocate output process list.");
//         return false;
//     }
//     WriteLog(LL_Debug, "process list kernel: (%p).", s_List);

//     s_List->StructureSize = s_ListSize;
//     for (auto l_Index = 0; l_Index < s_Count; ++l_Index)
//         s_List->Pids[l_Index] = s_Pids[l_Index];

//     // Allocate in process
//     return true;
// }

bool SystemDriverCtl::GetProcessList(MiraProcessList*& p_List)
{	
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
    auto _sx_slock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_slock);
	auto _sx_sunlock = (void(*)(struct sx *sx, const char *file, int line))kdlsym(_sx_sunlock);

    if (p_List != nullptr)
        WriteLog(LL_Warn, "process list already filled in, is this a bug?");
    
    Vector<int32_t> s_Pids;

    struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);
    struct sx* allproclock = (struct sx*)kdlsym(allproc_lock);

    struct proc* p = NULL;
    _sx_slock(allproclock, 0, __FILE__, __LINE__);
    FOREACH_PROC_IN_SYSTEM(p)
    {
        _mtx_lock_flags(&p->p_mtx, 0);
        s_Pids.push_back(p->p_pid);
        _mtx_unlock_flags(&p->p_mtx, 0);
    }
    _sx_sunlock(allproclock, __FILE__, __LINE__);

    auto s_Count = s_Pids.size();
    if (s_Count == 0 || s_Count > 200)
    {
        WriteLog(LL_Error, "invalid count of pids (%d).", s_Count);
        return false;
    }

    auto s_OutputSize = sizeof(MiraProcessList) + (sizeof(int32_t) * s_Count);
    auto s_Buffer = new uint8_t[s_OutputSize];
    if (s_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate output buffer (%d).", s_OutputSize);
        return false;
    }
    memset(s_Buffer, 0, s_OutputSize);

    auto s_Output = reinterpret_cast<MiraProcessList*>(s_Buffer);
    s_Output->StructureSize = s_OutputSize;

    for (auto i = 0; i < s_Count; ++i)
    {
        auto l_Pid = s_Pids[i];
        s_Output->Pids[i] = l_Pid;
    }

    p_List = s_Output;
    return true;
}


bool SystemDriverCtl::GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result)
{
    auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    //auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
    //auto _mtx_unlock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_unlock_spin_flags);
    //auto _mtx_lock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_lock_spin_flags);
    auto _thread_lock_flags = (void(*)(struct thread *, int, const char *, int))kdlsym(_thread_lock_flags);
    // Check the process id (we don't allow querying kernel)
    if (p_ProcessId <= 0)
        return false;
    
    // Debugging
    if (p_Result != nullptr)
        WriteLog(LL_Warn, "Result is not null, is this intended?");

    // Set our output pointer to null
    p_Result = nullptr;

    Vector<MiraProcessInformation::ThreadResult*> s_Threads;

    auto s_Proc = pfind(p_ProcessId);
    if (s_Proc == nullptr)
    {
        WriteLog(LL_Error, "could not get process.");
        return false;
    }
    // Proc is locked
    auto s_ProcessId = s_Proc->p_pid;
    auto s_OpPid = s_Proc->p_oppid;
    auto s_DebugChild = s_Proc->p_dbg_child;
    auto s_ExitThreads = s_Proc->p_exitthreads;
    auto s_SigParent = s_Proc->p_sigparent;
    auto s_Signal = s_Proc->p_sig;
    auto s_Code = s_Proc->p_code;
    auto s_Stops = s_Proc->p_stops;
    auto s_SType = s_Proc->p_stype;
    char s_Name[sizeof(s_Proc->p_comm)] = { 0 };
    char s_ElfPath[sizeof(s_Proc->p_elfpath)] = { 0 };
    char s_RandomizedPath[sizeof(s_Proc->p_randomized_path)] = { 0 };

    // Copy over our names to local memory
    memcpy(s_Name, s_Proc->p_comm, sizeof(s_Proc->p_comm));
    memcpy(s_ElfPath, s_Proc->p_elfpath, sizeof(s_Proc->p_elfpath));
    memcpy(s_RandomizedPath, s_Proc->p_randomized_path, sizeof(s_Proc->p_randomized_path));


    // Iterate through each thread in the process
    struct thread* s_CurrentThread = nullptr;
    FOREACH_THREAD_IN_PROC(s_Proc, s_CurrentThread)
    {
        // Lock the thread
        thread_lock(s_CurrentThread);

        // Allocate a new entry
        auto l_Info = new MiraProcessInformation::ThreadResult
        {
            .ThreadId = s_CurrentThread->td_tid,
            .ErrNo = s_CurrentThread->td_errno,
            .RetVal = s_CurrentThread->td_retval[0]
        };

        // Check if the allocation failed, it shouldn't but hey the ps4 fucks you sometimes.
        if (l_Info == nullptr)
        {
            WriteLog(LL_Error, "could not get info for pid (%d) tid (%d).", s_Proc->p_pid, s_CurrentThread->td_tid);
        }
        else
        {
            memset(l_Info->Name, 0, sizeof(l_Info->Name));
            memcpy(l_Info->Name, s_CurrentThread->td_name, sizeof(s_CurrentThread->td_name));

            s_Threads.push_back(l_Info);
        }

        thread_unlock(s_CurrentThread);
    }
    _mtx_unlock_flags(&s_Proc->p_mtx, 0);

    // Proc is unlocked, don't touch it anymore

    // Use a dowhile here because we always want to free the resource
    bool s_Success = false;
    do
    {
        auto s_Count = s_Threads.size();
        if (s_Count == 0 || s_Count > 2048)
        {
            WriteLog(LL_Error, "invalid thread count: (%d).", s_Count);
            break;
        }

        auto s_TotalSize = ( sizeof(MiraProcessInformation) + (sizeof(MiraProcessInformation::ThreadResult) * s_Count) );
        auto s_Output = new uint8_t[s_TotalSize];
        if (s_Output == nullptr)
        {
            WriteLog(LL_Error, "could not allocate (0x%x) bytes.", s_TotalSize);
            break;
        }
        memset(s_Output, 0, s_TotalSize);

        // Copy over all of the process information
        auto s_Result = reinterpret_cast<MiraProcessInformation*>(s_Output);
        s_Result->StructureSize = s_TotalSize;
        s_Result->ProcessId = s_ProcessId;
        s_Result->OpPid = s_OpPid;
        s_Result->DebugChild = s_DebugChild;
        s_Result->ExitThreads = s_ExitThreads;
        s_Result->SigParent = s_SigParent;
        s_Result->Signal = s_Signal;
        s_Result->Code = s_Code;
        s_Result->Stops = s_Stops;
        s_Result->SType = s_SType;

        memcpy(s_Result->Name, s_Name, sizeof(s_Name));
        memcpy(s_Result->ElfPath, s_ElfPath, sizeof(s_Result->ElfPath));
        memcpy(s_Result->RandomizedPath, s_RandomizedPath, sizeof(s_Result->RandomizedPath));

        s_Result->ThreadCount = s_Count;
        for (auto i = 0; i < s_Count; ++i)
        {
            auto l_Thread = s_Threads[i];
            auto l_OutThread = &s_Result->Threads[i];

            l_OutThread->ThreadId = l_Thread->ThreadId;
            l_OutThread->ErrNo = l_Thread->ErrNo;
            l_OutThread->RetVal = l_Thread->RetVal;
            memcpy(l_OutThread->Name, l_Thread->Name, sizeof(l_OutThread->Name));
        }

        p_Result = s_Result;

        s_Success = true;
    } while (false);
    
    // Clean up our allocations
    for (auto i = 0; i < s_Threads.size(); ++i)
    {
        auto l_Info = s_Threads.at(i);
        if (l_Info != nullptr)
            delete l_Info;
        
        s_Threads[i] = nullptr;
    }

    s_Threads.clear();

    return s_Success;
}


int32_t SystemDriverCtl::OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    MiraProcessInformation s_Input = { 0 };
    auto s_Result = copyin(p_Data, &s_Input, sizeof(MiraProcessInformation));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin enough data.");
        return (EFAULT);
    }

    // Get the process ID
    auto s_ProcessId = s_Input.ProcessId;

    // GetProcessInfo allocates
    MiraProcessInformation* s_Output = nullptr;
    if (!GetProcessInfo(s_ProcessId, s_Output))
    {
        WriteLog(LL_Error, "could not get process information.");
        return (ENOMEM);
    }

    if (s_Output == nullptr)
    {
        WriteLog(LL_Error, "invalid process information.");
        return (ENOMEM);
    }

    // Check the output size
    if (s_Input.StructureSize < s_Output->StructureSize)
    {
        WriteLog(LL_Error, "Output data not large enough (%d) < (%d).", s_Input.StructureSize, s_Output->StructureSize);
        delete [] s_Output;
        return (EMSGSIZE);
    }
    
    // Copy out the data if the size is large enough
    s_Result = copyout(s_Output, p_Data, s_Output->StructureSize);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyout (%d).", s_Result);
        delete [] s_Output;
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    delete [] s_Output;
    return 0;
}
*/