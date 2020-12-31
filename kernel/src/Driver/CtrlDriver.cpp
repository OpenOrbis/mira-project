// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CtrlDriver.hpp"
#include <mira/Driver/DriverCmds.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Vector.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Plugins/Substitute/Substitute.hpp>
#include <Trainers/TrainerManager.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/sx.h>
    #include <sys/proc.h>
    #include <sys/stat.h>
    #include <sys/fcntl.h>
    #include <paths.h>
    #include <fs/devfs/devfs.h>

    #include <sys/sysproto.h>
    #include <sys/sysent.h>
    #include <sys/ioccom.h>
    #include <sys/proc.h>
    #include <sys/filedesc.h>
    #include <sys/mount.h>

    struct intstr {
        const char *s;
        int i;
    };
};

using namespace Mira::Driver;

void* orig_ioctl;

CtrlDriver::CtrlDriver() :
    m_DeviceSw { 0 },
    m_Device(nullptr)
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    auto make_dev_p = (int(*)(int _flags, struct cdev **_cdev, struct cdevsw *_devsw, struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode, const char *_fmt, ...))kdlsym(make_dev_p);

    // Set up our device driver information
    m_DeviceSw.d_version = D_VERSION;
    m_DeviceSw.d_name = "mira";
    m_DeviceSw.d_open = OnOpen;
    m_DeviceSw.d_close = OnClose;
    m_DeviceSw.d_ioctl = OnIoctl;

    // Create our mutex to protect state
    mtx_init(&m_Mutex, "MiraCtrl", nullptr, MTX_DEF);

    WriteLog(LL_Debug, "MIRA_TRAINERS_LOAD: (%x).", MIRA_TRAINERS_LOAD);

    // Add the device driver
    int32_t s_ErrorDev = make_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK,
        &m_Device,
        &m_DeviceSw,
        nullptr,
        UID_ROOT,
        GID_WHEEL,
        S_IRWXU | S_IRWXG | S_IRWXO,
        "mira");

    switch (s_ErrorDev)
    {
    case 0:
        WriteLog(LL_Debug, "device driver created successfully!");
        break;
    case EEXIST:
        WriteLog(LL_Error, "could not create device driver, device driver already exists.");
        return;
    default:
        WriteLog(LL_Error, "could not create device driver (%d).", s_ErrorDev);
        return;
    }
}

void CtrlDriver::OnProcessExec(void* __unused a1, struct proc *p)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);

    // Add rules for mira to allow process to open & see it
    struct devfs_rule s_Dr;

    memset(&s_Dr, 0, sizeof(struct devfs_rule));
    s_Dr.dr_id = 0;
    s_Dr.dr_magic = DEVFS_MAGIC;

    snprintf(s_Dr.dr_pathptrn, DEVFS_MAXPTRNLEN, "mira");
    s_Dr.dr_icond |= DRC_PATHPTRN;
    s_Dr.dr_iacts |= DRA_BACTS;
    s_Dr.dr_bacts |= DRB_UNHIDE;

    // Open devfs device
    int32_t s_DevFS = kopen_t(_PATH_DEV, O_RDONLY, 0, s_ProcessThread);
    if (s_DevFS < 0) {
        WriteLog(LL_Error, "unable to open %s file (%d).", _PATH_DEV, s_DevFS);
        return;
    }

    /*
        DevFS Rules ioctl (Because need calculation)

        0x80044401 (DEVFSIO_RDEL)
        0x80044403 (DEVFSIO_RAPPLYID)
        0x8002440A (DEVFSIO_SUSE)
        0x8002440B (DEVFSIO_SAPPY)
        0x80EC4402 (DEVFSIO_RAPPLY)
        0xC002440C (DEVFSIO_SGETNEXT)
        0xC0EC4400 (DEVFSIO_RADD)
        0xC0EC4404 (DEVFSIO_RGETNEXT)
    */

    static_assert(DEVFSIO_RAPPLY == 0x80EC4402, "incorrect ioctl code.");

    int32_t s_ErrorIoctl = kioctl_t(s_DevFS, DEVFSIO_RAPPLY, (char*)&s_Dr, s_ProcessThread);
    if (s_ErrorIoctl < 0) {
        WriteLog(LL_Error, "unable to apply devfs rule add request (%d).", s_ErrorIoctl);
        return;
    }

    kclose_t(s_DevFS, s_ProcessThread);
    WriteLog(LL_Info, "Mira device allowed to this jailed process");
}

CtrlDriver::~CtrlDriver()
{
    WriteLog(LL_Debug, "destroying driver");

    // Destroy the device driver
    auto destroy_dev = (void(*)(struct cdev *_dev))kdlsym(destroy_dev);
    destroy_dev(m_Device);
}

int32_t CtrlDriver::OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread)
{
    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver opened from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    return 0;
}

int32_t CtrlDriver::OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread)
{
    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver closed from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    return 0;
}

int32_t CtrlDriver::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    //auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    p_Command = p_Command & 0xFFFFFFFF; // Clear the upper32

    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    switch (IOCGROUP(p_Command)) 
    {
        case SUBSTITUTE_IOCTL_BASE:
            return Mira::Plugins::Substitute::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
        case MIRA_IOCTL_BASE:
        {
            // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
                // Get the requested process information
                case MIRA_GET_PROC_INFORMATION:
                    return OnMiraGetProcInformation(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                
                case MIRA_GET_PID_LIST:
                    return OnMiraGetProcList(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                
                case MIRA_MOUNT_IN_SANDBOX:
                    return OnMiraMountInSandbox(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                // Get/set the thread credentials
                case MIRA_GET_PROC_THREAD_CREDENTIALS:
                    return OnMiraThreadCredentials(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_TRAINERS_LOAD:
                {
                    WriteLog(LL_Debug, "pid: (%d) requesting trainer loading...", p_Thread->td_proc->p_pid);
                    if (Mira::Framework::GetFramework()->GetTrainerManager()->LoadTrainers(p_Thread->td_proc))
                        return 0;
                        
                    return EPROCUNAVAIL;
                }
                case MIRA_TRAINERS_ORIG_EP:
                {
                    auto s_Driver = Mira::Framework::GetFramework()->GetDriver();
                    if (s_Driver == nullptr)
                        return ENOMEM;
                    
                    auto s_Proc = p_Thread->td_proc;
                    if (s_Proc == nullptr)
                        return EPROCUNAVAIL;
                    
                    auto s_EntryPoint = s_Driver->GetEntryPoint(s_Proc->p_pid);
                    if (s_EntryPoint == nullptr)
                        return ESRCH;
                    
                    WriteLog(LL_Debug, "Found EntryPoint (%p).", s_EntryPoint);

                    copyout(&s_EntryPoint, p_Data, sizeof(s_EntryPoint));
                    return 0;
                }
                case MIRA_GET_CONFIG:
                    return OnMiraGetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_SET_CONFIG:
                    return OnMiraSetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
            }
        }

        default:
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
    }

    return EINVAL;
}

int32_t CtrlDriver::OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
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
    if (s_Input.Size < s_Output->Size)
    {
        WriteLog(LL_Error, "Output data not large enough (%d) < (%d).", s_Input.Size, s_Output->Size);
        delete [] s_Output;
        return (EMSGSIZE);
    }
    
    // Copy out the data if the size is large enough
    s_Result = copyout(s_Output, p_Data, s_Output->Size);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyout (%d).", s_Result);
        delete [] s_Output;
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    delete [] s_Output;
    return 0;
}

int32_t CtrlDriver::OnMiraGetProcList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

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

    if (s_Input.Size < s_Output->Size)
    {
        WriteLog(LL_Error, "input size (%d) < output size (%d).", s_Input.Size, s_Output->Size);
        delete [] s_Output;
        return (EMSGSIZE);
    }

    s_Result = copyout(s_Output, p_Data, s_Output->Size);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyuout data (%d).", s_Result);
        delete [] s_Output;
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    delete [] s_Output;
    return 0;
}

int32_t CtrlDriver::OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    //auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    //auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    //auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);


    if (p_Device == nullptr)
    {
        WriteLog(LL_Error, "invalid device.");
        return ENODEV;
    }

    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return EBADF;
    }

    auto s_TargetProc = p_Thread->td_proc;
    if (s_TargetProc == nullptr)
    {
        WriteLog(LL_Error, "thread does not have a parent process wtf?");
        return EPROCUNAVAIL;
    }

    auto s_Descriptor = s_TargetProc->p_fd;
    if (s_Descriptor == nullptr)
    {
        WriteLog(LL_Error, "could not get the file descriptor for proc.");
        return EBADF;
    }

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get mira main thread.");
        return EBADF;
    }

    // Read in the mount point and the flags
    MiraMountInSandbox s_Input = { 0 };
    auto s_Result = copyin(p_Data, &s_Input, sizeof(s_Input));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin all data (%d).", s_Result);
        return (s_Result < 0 ? -s_Result : s_Result);
    }

    // Check to make sure that there's some kind of path
    if (s_Input.HostPath[0] == '\0')
    {
        WriteLog(LL_Error, "invalid input path.");
        return (EACCES);
    }

    if (s_Input.SandboxPath[0] == '\0')
    {
        WriteLog(LL_Error, "invalid sandbox path.");
        return (EACCES);
    }

    if (s_Input.SandboxPath[0] == '/')
    {
        WriteLog(LL_Error, "sandbox path does not need to start with '/'.");
        return (EACCES);
    }
    
    return OrbisOS::Utilities::MountInSandbox(s_Input.HostPath, s_Input.SandboxPath, nullptr, p_Thread);
}

bool CtrlDriver::GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result)
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
    memcpy(s_ElfPath, s_Proc->p_elfpath, sizeof(s_Proc->p_comm));
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
        s_Result->Size = s_TotalSize;
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

bool CtrlDriver::GetProcessList(MiraProcessList*& p_List)
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
    s_Output->Size = s_OutputSize;

    for (auto i = 0; i < s_Count; ++i)
    {
        auto l_Pid = s_Pids[i];
        s_Output->Pids[i] = l_Pid;
    }

    p_List = s_Output;
    return true;
}

int32_t CtrlDriver::OnMiraThreadCredentials(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    
    // Read the input structure
    MiraThreadCredentials s_Input;
    auto s_Result = copyin(p_Data, &s_Input, sizeof(s_Input));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin all data (%d).", s_Result);
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    // Failsafe in case that a provided number is 0 or negative
    if (s_Input.ThreadId <= 0)
        s_Input.ThreadId = p_Thread->td_tid;

    MiraThreadCredentials* s_Output = nullptr;

    switch (s_Input.State)
    {
    case MiraThreadCredentials::GSState::Get:
    {
        // Get the thread credentials
        if (!GetThreadCredentials(s_Input.ProcessId, s_Input.ThreadId, s_Output))
        {
            WriteLog(LL_Error, "could not get thread credentials.");
            return (EPROCUNAVAIL);
        }

        if (s_Output == nullptr)
        {
            WriteLog(LL_Error, "invalid output");
            return (EFAULT);
        }

        // Copyout the data back to userland
        s_Result = copyout(s_Output, p_Data, sizeof(*s_Output));

        // Free the resource
        delete s_Output;
        s_Output = nullptr;

        return (s_Result < 0 ? -s_Result : s_Result);
    }
    case MiraThreadCredentials::GSState::Set:
    {
        // Set the thread credentials
        if (!SetThreadCredentials(s_Input.ProcessId, s_Input.ThreadId, s_Input))
        {
            WriteLog(LL_Error, "could not set thread credentials.");
            return EPROCUNAVAIL;
        }

        return 0;
    }
    default:
        WriteLog(LL_Error, "undefined state (%d).", s_Input.State);
        return EINVAL;
    }

    return EINVAL;
}

bool CtrlDriver::SetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials& p_Input)
{
    auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *, int, const char *, int))kdlsym(_thread_lock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    // Make sure that we are setting threads
    if (p_Input.State != MiraThreadCredentials::_State::Set)
        return false;
    
    // Get the process, this returns locked
    auto s_Process = pfind(p_ProcessId);
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
        return false;
    }

    bool s_ThreadModified = false;
    do
    {
        struct thread* l_Thread = nullptr;
        FOREACH_THREAD_IN_PROC(s_Process, l_Thread)
        {
            thread_lock(l_Thread);

            do
            {
                if (p_ThreadId <= 0 || l_Thread->td_tid == p_ThreadId)
                {
                    auto l_ThreadCredential = l_Thread->td_ucred;
                    if (l_ThreadCredential == nullptr)
                    {
                        WriteLog(LL_Error, "could not get thread ucred for tid (%d).", l_Thread->td_tid);
                        break;
                    }
                
                    // ucred
                    l_ThreadCredential->cr_uid = p_Input.EffectiveUserId;
                    l_ThreadCredential->cr_ruid = p_Input.RealUserId;

                    l_ThreadCredential->cr_svuid  = p_Input.SavedUserId;

                    l_ThreadCredential->cr_ngroups  = p_Input.NumGroups;

                    l_ThreadCredential->cr_rgid  = p_Input.RealGroupId;
                    l_ThreadCredential->cr_svgid  = p_Input.SavedGroupId;

                    // prison
                    if (p_Input.Prison == MiraThreadCredentials::_MiraThreadCredentialsPrison::Root)
                    {
                        l_ThreadCredential->cr_prison = *(struct prison**)kdlsym(prison0);
                    }
                    
                    l_ThreadCredential->cr_sceAuthID = p_Input.SceAuthId;
                    
                    // TODO: Static assert that these are equal
                    memcpy(l_ThreadCredential->cr_sceCaps, p_Input.Capabilities, sizeof(l_ThreadCredential->cr_sceCaps));

                    // TODO: Static assert that sizeof are equal
                    memcpy(l_ThreadCredential->cr_sceAttr, p_Input.Attributes, sizeof(l_ThreadCredential->cr_sceAttr));

                    // Update the rootvnode
                    auto l_FileDesc = s_Process->p_fd;
                    if (l_FileDesc && p_Input.Prison == MiraThreadCredentials::_MiraThreadCredentialsPrison::Root)
                    {
                        l_FileDesc->fd_rdir = *(struct vnode**)kdlsym(rootvnode);
                        l_FileDesc->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
                    }

                    s_ThreadModified = true;
                }

            } while (false);
            
            thread_unlock(l_Thread);
        }
    } while (false);
    _mtx_unlock_flags(&s_Process->p_mtx, 0);
    
    return s_ThreadModified;
}

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
        if (l_Thread == nullptr)
            continue;
        
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

                s_Credentials->State = MiraThreadCredentials::_State::Get;
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

void CtrlDriver::AddOrUpdateEntryPoint(int32_t p_ProcessId, void* p_EntryPoint)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_ProcessId <= 0)
    {
        WriteLog(LL_Error, "invalid process id (%d).", p_ProcessId);
        return;
    }

    if (p_EntryPoint == nullptr)
    {
        WriteLog(LL_Error, "invalid entry point (%p).", p_EntryPoint);
        return;
    }

    _mtx_lock_flags(&m_Mutex, 0);

    do
    {
        // Handle if there is nothing added
        if (m_Head == nullptr)
        {
            auto s_NewHead = new MiraTrainerProcessInfo;
            if (s_NewHead == nullptr)
            {
                WriteLog(LL_Error, "could not allocate a new MiraTrainerProcessInfo.");
                break;
            }

            s_NewHead->ProcessId = p_ProcessId;
            s_NewHead->EntryPoint = p_EntryPoint;
            s_NewHead->Previous = nullptr;
            s_NewHead->Next = nullptr;

            m_Head = s_NewHead;
        }
        else // We already created a head
        {
            MiraTrainerProcessInfo* s_CurrentInfo = m_Head;
            bool s_Updated = false;
            while (s_CurrentInfo != nullptr)
            {
                if (p_ProcessId == s_CurrentInfo->ProcessId)
                {
                    s_CurrentInfo->EntryPoint = p_EntryPoint;
                    s_Updated = true;
                    break;
                }

                s_CurrentInfo = s_CurrentInfo->Next;
            }

            // Add a new entry
            if (!s_Updated)
            {
                auto s_Entry = new MiraTrainerProcessInfo;
                if (s_Entry == nullptr)
                {
                    WriteLog(LL_Error, "could not create a new MiraTrainerProcessInfo.");
                    break;
                }

                s_Entry->ProcessId = p_ProcessId;
                s_Entry->EntryPoint = p_EntryPoint;
                s_Entry->Next = nullptr;
                s_Entry->Previous = nullptr;

                // Find the last entry in the chain
                MiraTrainerProcessInfo* s_CurrentInfo = m_Head;
                while (s_CurrentInfo != nullptr)
                {
                    if (s_CurrentInfo->Next == nullptr)
                    {
                        // Add to the tail
                        s_CurrentInfo->Next = s_Entry;

                        // Set the previous
                        s_Entry->Previous = s_CurrentInfo;

                        WriteLog(LL_Debug, "Added new MiraTrainerProcessInfo.");
                        break;
                    }

                    s_CurrentInfo = s_CurrentInfo->Next;
                }
            }
        }
        
    } while (false);
    
    _mtx_unlock_flags(&m_Mutex, 0);
}

void CtrlDriver::RemoveEntryPoint(int32_t p_ProcessId)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    _mtx_lock_flags(&m_Mutex, 0);

    do
    {
        MiraTrainerProcessInfo* s_CurrentInfo = m_Head;
        while (s_CurrentInfo != nullptr)
        {
            if (s_CurrentInfo->ProcessId == p_ProcessId)
            {
                WriteLog(LL_Debug, "Removing PID (%d).", p_ProcessId);

                // Set the previous entry to point to our next entry
                if (s_CurrentInfo->Previous != nullptr)
                    s_CurrentInfo->Previous->Next = s_CurrentInfo->Next;
                
                // Set our next entry to our previous entry
                if (s_CurrentInfo->Next != nullptr)
                    s_CurrentInfo->Next->Previous = s_CurrentInfo->Previous;
                
                // Delete the current entry
                delete s_CurrentInfo;
                break;
            }
        }

    } while (false);

    _mtx_unlock_flags(&m_Mutex, 0);

}

void* CtrlDriver::GetEntryPoint(int32_t p_ProcessId)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_ProcessId <= 0)
        return nullptr;
    
    _mtx_lock_flags(&m_Mutex, 0);

    void* s_Found = nullptr;

    do
    {
        MiraTrainerProcessInfo* s_CurrentInfo = m_Head;
        while (s_CurrentInfo != nullptr)
        {
            if (s_CurrentInfo->ProcessId == p_ProcessId)
            {
                s_Found = s_CurrentInfo->EntryPoint;
                break;
            }

            s_CurrentInfo = s_CurrentInfo->Next;
        }
    } while (false);
    
    _mtx_unlock_flags(&m_Mutex, 0);

    return s_Found;
}