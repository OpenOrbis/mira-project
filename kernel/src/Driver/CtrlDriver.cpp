// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CtrlDriver.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Vector.hpp>

#include <Plugins/Substitute/Substitute.hpp>

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

    struct intstr {
        const char *s;
        int i;
    };
};

using namespace Mira::Driver;

void* orig_ioctl;

CtrlDriver::CtrlDriver() :
    m_processStartHandler(nullptr),
    m_DeviceSw { 0 },
    m_Device(nullptr)
{
    auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);
    auto make_dev_p = (int(*)(int _flags, struct cdev **_cdev, struct cdevsw *_devsw, struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode, const char *_fmt, ...))kdlsym(make_dev_p);

    // Set up our device driver information
    m_DeviceSw.d_version = D_VERSION;
    m_DeviceSw.d_name = "mira";
    m_DeviceSw.d_open = OnOpen;
    m_DeviceSw.d_close = OnClose;
    m_DeviceSw.d_ioctl = OnIoctl;

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

    m_processStartHandler = EVENTHANDLER_REGISTER(process_exec, reinterpret_cast<void*>(OnProcessStart), nullptr, EVENTHANDLER_PRI_ANY);
}

void CtrlDriver::OnProcessStart(void *arg, struct proc *p)
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

    int32_t s_ErrorIoctl = kioctl_t(s_DevFS, 0x80EC4402, (char*)&s_Dr, s_ProcessThread);
    if (s_ErrorIoctl < 0) {
        WriteLog(LL_Error, "unable to apply devfs rule add request (%d).", s_ErrorIoctl);
        return;
    }

    kclose_t(s_DevFS, s_ProcessThread);
    WriteLog(LL_Info, "Mira device allowed to this jailed process");
}

CtrlDriver::~CtrlDriver()
{
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
    auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    WriteLog(LL_Debug, "destroying driver");

    // Disable the eventhandler
    if (m_processStartHandler) {
        EVENTHANDLER_DEREGISTER(process_exec, m_processStartHandler);
        m_processStartHandler = nullptr;
    }

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
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    switch (IOCGROUP(p_Command)) {
        case SUBSTITUTE_IOCTL_BASE:
            return Mira::Plugins::Substitute::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
        case MIRA_IOCTL_BASE:
        {
            // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
                // Get the requested process information
                case MIRA_GET_PROC_INFORMATION:
                {
                    MiraProcessInformation s_Input;
                    auto s_Result = copyin(p_Data, &s_Input, sizeof(MiraProcessInformation));
                    if (s_Result != 0)
                    {
                        WriteLog(LL_Error, "could not copyin enough data.");
                        return -EFAULT;
                    }

                    // Get the process ID
                    auto s_ProcessId = s_Input.ProcessId;

                    // GetProcessInfo allocates
                    MiraProcessInformation* s_Output = nullptr;
                    if (!GetProcessInfo(s_ProcessId, s_Output))
                    {
                        WriteLog(LL_Error, "could not get process information.");
                        return -ENOMEM;
                    }

                    if (s_Output == nullptr)
                    {
                        WriteLog(LL_Error, "invalid process information.");
                        return -ENOMEM;
                    }

                    // Check the output size
                    if (s_Input.Size < s_Output->Size)
                    {
                        WriteLog(LL_Error, "Output data not large enough (%d) < (%d).", s_Input.Size, s_Output->Size);
                        delete s_Output;
                        return -EMSGSIZE;
                    }
                    
                    // Copy out the data if the size is large enough
                    s_Result = copyout(s_Output, p_Data, s_Output->Size);
                    if (s_Result != 0)
                    {
                        WriteLog(LL_Error, "could not copyout (%d).", s_Result);
                        delete s_Output;
                        return (s_Result < 0 ? s_Result : -s_Result);
                    }

                    delete s_Output;
                    return 0;
                }
                case MIRA_GET_PID_LIST:
                {
                    MiraProcessList s_Input;
                    auto s_Result = copyin(p_Data, &s_Input, sizeof(MiraProcessList));
                    if (s_Result != 0)
                    {
                        WriteLog(LL_Error, "could not copyin process list.");
                        return (s_Result < 0 ? s_Result : -s_Result);
                    }

                    MiraProcessList* s_Output = nullptr;
                    if (!GetProcessList(s_Output))
                    {
                        WriteLog(LL_Error, "could not get the process list.");
                        return -ENOMEM;
                    }

                    if (s_Output == nullptr)
                    {
                        WriteLog(LL_Error, "could not allocate the output.");
                        return -ENOMEM;
                    }

                    if (s_Input.Size < s_Output->Size)
                    {
                        WriteLog(LL_Error, "input size (%d) < output size (%d).", s_Input.Size, s_Output->Size);
                        delete s_Output;
                        return -EMSGSIZE;
                    }

                    s_Result = copyout(s_Output, p_Data, s_Output->Size);
                    if (s_Result != 0)
                    {
                        WriteLog(LL_Error, "could not copyuout data (%d).", s_Result);
                        delete s_Output;
                        return (s_Result < 0 ? s_Result : -s_Result);
                    }

                    delete s_Output;
                    return 0;
                }

                // Get/set the thread credentials
                case MIRA_GET_PROC_THREAD_CREDENTIALS:
                    break;
            }
        }

        default: {
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
        }
    }

    return -1;
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

    return true;
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