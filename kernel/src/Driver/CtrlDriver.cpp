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

#include <Plugins/PluginManager.hpp>
#include <Plugins/PrivCheck/PrivCheckPlugin.hpp>
#include <Plugins/Debugging/Debugger.hpp>
#include <OrbisOS/MountManager.hpp>

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
    //auto copyinstr = (int(*)(const void *uaddr, void *kaddr, size_t len, size_t *done))kdlsym(copyinstr);

    //auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    p_Command = p_Command & 0xFFFFFFFF; // Clear the upper32

    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get mira framework.");
        return ENOMEM;
    }

    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager.");
        return ENOMEM;
    }

    switch (IOCGROUP(p_Command)) 
    {
        case MIRA_IOCTL_BASE:
        {
            // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
                case MIRA_READ_PROCESS_MEMORY:
                case MIRA_WRITE_PROCESS_MEMORY:
                //case MIRA_GET_PROC_INFORMATION:
                case MIRA_GET_PID_LIST:
                case MIRA_ALLOCATE_PROCESS_MEMORY:
                case MIRA_FREE_PROCESS_MEMORY:
                    return SystemDriverCtl::OnSystemDriverCtlIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_MOUNT_IN_SANDBOX:
                    return OnMiraMountInSandbox(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                // Get/set the thread credentials
                case MIRA_GET_PROC_THREAD_CREDENTIALS:
                    return OnMiraThreadCredentials(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                
                case MIRA_TRAINERS_CREATE_SHM:
                case MIRA_TRAINERS_GET_SHM:
                case MIRA_TRAINERS_LOAD:
                case MIRA_TRAINERS_ORIG_EP:
                    return Mira::Trainers::TrainerManager::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                case MIRA_GET_CONFIG:
                    return OnMiraGetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_SET_CONFIG:
                    return OnMiraSetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_SET_THREAD_PRIV_MASK:
                    return Plugins::PrivCheckPlugin::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_FIND_JMPSLOT:
                    return Plugins::Debugger::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                default:
                    WriteLog(LL_Debug, "mira base unknown command: (0x%llx).", p_Command);
                    break;
            }
        }

        default:
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
    }

    return EINVAL;
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

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return EBADF;
    }

    auto s_MountManager = s_Framework->GetMountManager();
    if (s_MountManager == nullptr)
    {
        WriteLog(LL_Error, "could not get mount manager.");
        return EBADF;
    }

    auto s_MainThread = s_Framework->GetMainThread();
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

    // Create the mount point
    if (!s_MountManager->CreateMountInSandbox(s_Input.HostPath, s_Input.SandboxPath, p_Thread))
        return (EACCES);

    return  0;

    // TODO: Once mountmanager is verified working remove comment below
    //OrbisOS::Utilities::MountInSandbox(s_Input.HostPath, s_Input.SandboxPath, nullptr, p_Thread);
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
    case GSState::Get:
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
    case GSState::Set:
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
    if (p_Input.State != GSState::Set)
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