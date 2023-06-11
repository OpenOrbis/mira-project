// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CtrlDriver.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Vector.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Plugins/Substitute/Substitute.hpp>

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

    WriteLog(LL_Debug, "MIRA_MOUNT_IN_SANDBOX: 0x%08x", MIRA_MOUNT_IN_SANDBOX);
    WriteLog(LL_Debug, "MIRA_UNMOUNT_IN_SANDBOX: 0x%08x", MIRA_UNMOUNT_IN_SANDBOX);
    WriteLog(LL_Debug, "MIRA_GET_PROC_THREAD_CREDENTIALS: 0x%08x", MIRA_GET_PROC_THREAD_CREDENTIALS);
    WriteLog(LL_Debug, "MIRA_GET_PID_LIST: 0x%08x", MIRA_GET_PID_LIST);
    WriteLog(LL_Debug, "MIRA_GET_PROC_INFORMATION: 0x%08x", MIRA_GET_PROC_INFORMATION);

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
    //auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    //auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

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
                    return OnMiraGetProcInformation(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                case MIRA_GET_PID_LIST:
                    return OnMiraGetProcList(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                case MIRA_MOUNT_IN_SANDBOX:
                    return OnMiraMountInSandbox(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                case MIRA_UNMOUNT_IN_SANDBOX:
                    return OnMiraUnmountInSandbox(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                // Get/set the thread credentials
                case MIRA_GET_PROC_THREAD_CREDENTIALS:
                    return OnMiraThreadCredentials(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
            }
        }

        default: {
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
        }
    }

    return -1;
}

int32_t CtrlDriver::OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    MiraProcessInformation s_ProcInfo = { 0 };
    auto s_Result = copyin(p_Data, &s_ProcInfo, sizeof(MiraProcessInformation));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin enough data.");
        return -EFAULT;
    }

    // Get the process ID
    // If set to 0, assume the caller wants its own process info
    const auto s_ProcessId = s_ProcInfo.ProcessId > 0 ? s_ProcInfo.ProcessId
	    : p_Thread->td_proc->p_pid;

    // Fetch process info
    if (!GetProcessInfo(s_ProcessId, &s_ProcInfo))
    {
        WriteLog(LL_Error, "could not get process information.");
        return -ENOMEM;
    }

    // Copy out the data
    s_Result = copyout(&s_ProcInfo, p_Data, sizeof(MiraProcessInformation));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyout (%d).", s_Result);
    }

    return (s_Result < 0 ? s_Result : -s_Result);
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
        delete [] s_Output;
        return -EMSGSIZE;
    }

    s_Result = copyout(s_Output, p_Data, s_Output->Size);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyuout data (%d).", s_Result);
        delete [] s_Output;
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    delete [] s_Output;
    return 0;
}

int32_t CtrlDriver::OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    //auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);


    if (p_Device == nullptr)
    {
        WriteLog(LL_Error, "invalid device.");
        return -1;
    }

    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return -1;
    }

    auto s_TargetProc = p_Thread->td_proc;
    if (s_TargetProc == nullptr)
    {
        WriteLog(LL_Error, "thread does not have a parent process wtf?");
        return -1;
    }

    auto s_Descriptor = s_TargetProc->p_fd;
    if (s_Descriptor == nullptr)
    {
        WriteLog(LL_Error, "could not get the file descriptor for proc.");
        return -1;
    }

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get mira main thread.");
        return -1;
    }

    // Read in the mount point and the flags
    MiraMountInSandbox s_Input = { 0 };
    auto s_Result = copyin(p_Data, &s_Input, sizeof(s_Input));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin all data (%d).", s_Result);
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    // Check to make sure that there's some kind of path
    if (s_Input.Path[0] == '\0')
    {
        WriteLog(LL_Error, "invalid input path.");
        return -EACCES;
    }


    // Check to make sure that there's some kind of name
    if (s_Input.Name[0] == '\0')
    {
        WriteLog(LL_Error, "invalid input path.");
        return -EACCES;
    }


    // Get the jailed path
    char* s_SandboxPath = nullptr;
    char* s_FreePath = nullptr;
    s_Result = vn_fullpath(s_MainThread, s_Descriptor->fd_jdir, &s_SandboxPath, &s_FreePath);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not get the full path (%d).", s_Result);
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    // Validate that we got something back
    if (s_SandboxPath == nullptr)
    {
        WriteLog(LL_Error, "could not get the sandbox path.");

        if (s_FreePath != nullptr)
            delete s_FreePath;

        return -1;
    }

    char s_InSandboxPath[PATH_MAX] = { 0 };
    char s_RealPath[PATH_MAX] = { 0 };

    do
    {
        s_Result = snprintf(s_InSandboxPath, sizeof(s_InSandboxPath), "%s/%s", s_SandboxPath, s_Input.Name);
        if (s_Result <= 0)
            break;

        s_Result = snprintf(s_RealPath, sizeof(s_RealPath), s_Input.Path);
        if (s_Result <= 0)
            break;

        // Check to see if the real path directory actually exists
        auto s_DirectoryHandle = kopen_t(s_RealPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
        if (s_DirectoryHandle < 0)
        {
            WriteLog(LL_Error, "could not open directory (%s) (%d).", s_RealPath, s_DirectoryHandle);
            break;
        }

        // Close the directory once we know it exists
        kclose_t(s_DirectoryHandle, s_MainThread);

        // Create the new folder inside of the sandbox
        s_Result = kmkdir_t(s_InSandboxPath, 0511, s_MainThread);
        if (s_Result < 0)
        {
            WriteLog(LL_Error, "could not create the directory for mount (%s) (%d).", s_InSandboxPath, s_Result);
            break;
        }

        // In order for the mount call, it uses the calling thread to see if it has permissions
        auto s_CurrentThreadCred = curthread->td_proc->p_ucred;
        auto s_CurrentThreadFd = curthread->td_proc->p_fd;

        // Validate that our cred and descriptor are valid
        if (s_CurrentThreadCred == nullptr || s_CurrentThreadFd == nullptr)
        {
            WriteLog(LL_Error, "the cred and/or fd are nullptr.");
            s_Result = -EACCES;
            break;
        }

        // Save backups of the original fd and credentials
        auto s_OriginalThreadCred = *s_CurrentThreadCred;
        auto s_OriginalThreadFd = *s_CurrentThreadFd;

        // Set maximum permissions
        s_CurrentThreadCred->cr_uid = 0;
        s_CurrentThreadCred->cr_ruid = 0;
        s_CurrentThreadCred->cr_rgid = 0;
        s_CurrentThreadCred->cr_groups[0] = 0;

        s_CurrentThreadCred->cr_prison = *(struct prison**)kdlsym(prison0);
        s_CurrentThreadFd->fd_rdir = s_CurrentThreadFd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);

        // Try and mount using the current credentials
        s_Result = Mira::OrbisOS::Utilities::MountNullFS(s_InSandboxPath, s_RealPath, MNT_RDONLY);
        if (s_Result < 0)
        {
            WriteLog(LL_Error, "could not mount fs inside sandbox (%s). (%d).", s_InSandboxPath, s_Result);
            krmdir_t(s_InSandboxPath, s_MainThread);

            // Restore credentials and fd
            *s_CurrentThreadCred = s_OriginalThreadCred;
            *s_CurrentThreadFd = s_OriginalThreadFd;

            break;
        }

        // Restore credentials and fd
        *s_CurrentThreadCred = s_OriginalThreadCred;
        *s_CurrentThreadFd = s_OriginalThreadFd;

        s_Result = 0;
    } while (false);

    // Cleanup the freepath
    if (s_FreePath != nullptr)
        delete s_FreePath;

    return s_Result;
}

int32_t CtrlDriver::OnMiraUnmountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    //auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);


    if (p_Device == nullptr)
    {
        WriteLog(LL_Error, "invalid device.");
        return -1;
    }

    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return -1;
    }

    auto s_TargetProc = p_Thread->td_proc;
    if (s_TargetProc == nullptr)
    {
        WriteLog(LL_Error, "thread does not have a parent process wtf?");
        return -1;
    }

    auto s_Descriptor = s_TargetProc->p_fd;
    if (s_Descriptor == nullptr)
    {
        WriteLog(LL_Error, "could not get the file descriptor for proc.");
        return -1;
    }

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get mira main thread.");
        return -1;
    }

    // Read in the mount point and the flags
    MiraUnmountInSandbox s_Input = { 0 };
    auto s_Result = copyin(p_Data, &s_Input, sizeof(s_Input));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin all data (%d).", s_Result);
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    // Check to make sure that there's some kind of name
    if (s_Input.Name[0] == '\0')
    {
        WriteLog(LL_Error, "invalid input path.");
        return -EACCES;
    }


    // Get the jailed path
    char* s_SandboxPath = nullptr;
    char* s_FreePath = nullptr;
    s_Result = vn_fullpath(s_MainThread, s_Descriptor->fd_jdir, &s_SandboxPath, &s_FreePath);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not get the full path (%d).", s_Result);
        return (s_Result < 0 ? s_Result : -s_Result);
    }

    // Validate that we got something back
    if (s_SandboxPath == nullptr)
    {
        WriteLog(LL_Error, "could not get the sandbox path.");

        if (s_FreePath != nullptr)
            delete s_FreePath;

        return -1;
    }

    char s_InSandboxPath[PATH_MAX] = { 0 };

    do
    {
        s_Result = snprintf(s_InSandboxPath, sizeof(s_InSandboxPath), "%s/%s", s_SandboxPath, s_Input.Name);
        if (s_Result <= 0)
            break;

        // Check to see if the real path directory actually exists
        auto s_DirectoryHandle = kopen_t(s_InSandboxPath, O_RDONLY | O_DIRECTORY, 0777, s_MainThread);
        if (s_DirectoryHandle < 0)
        {
            WriteLog(LL_Error, "could not open directory (%s) (%d).", s_InSandboxPath, s_DirectoryHandle);
            break;
        }

        // Close the directory once we know it exists
        kclose_t(s_DirectoryHandle, s_MainThread);


        // In order for the mount call, it uses the calling thread to see if it has permissions
        auto s_CurrentThreadCred = curthread->td_proc->p_ucred;
        auto s_CurrentThreadFd = curthread->td_proc->p_fd;

        // Validate that our cred and descriptor are valid
        if (s_CurrentThreadCred == nullptr || s_CurrentThreadFd == nullptr)
        {
            WriteLog(LL_Error, "the cred and/or fd are nullptr.");
            s_Result = -EACCES;
            break;
        }

        // Save backups of the original fd and credentials
        auto s_OriginalThreadCred = *s_CurrentThreadCred;
        auto s_OriginalThreadFd = *s_CurrentThreadFd;

        // Set maximum permissions
        s_CurrentThreadCred->cr_uid = 0;
        s_CurrentThreadCred->cr_ruid = 0;
        s_CurrentThreadCred->cr_rgid = 0;
        s_CurrentThreadCred->cr_groups[0] = 0;

        s_CurrentThreadCred->cr_prison = *(struct prison**)kdlsym(prison0);
        s_CurrentThreadFd->fd_rdir = s_CurrentThreadFd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);

        // Unmount folder if the folder exist
        int ret = kunmount_t(s_InSandboxPath, MNT_FORCE, s_MainThread);
        if (ret < 0) {
            WriteLog(LL_Error, "could not unmount folder (%s) (%d), Trying to remove anyway ...", s_InSandboxPath, ret);
        }

        // Remove folder
        ret = krmdir_t(s_InSandboxPath, s_MainThread);
        if (ret < 0) {
            WriteLog(LL_Error, "could not remove substitute folder (%s) (%d).", s_InSandboxPath, ret);
            break;
        }

        // Restore credentials and fd
        *s_CurrentThreadCred = s_OriginalThreadCred;
        *s_CurrentThreadFd = s_OriginalThreadFd;

        s_Result = 0;
    } while (false);

    // Cleanup the freepath
    if (s_FreePath != nullptr)
        delete s_FreePath;

    return s_Result;
}

bool CtrlDriver::GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation* p_Result)
{
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    // Check the process id (we don't allow querying kernel)
    if (p_ProcessId <= 0)
    {
        WriteLog(LL_Error, "ProcessId (%d) is invalid.", p_ProcessId);
        return false;
    }
    if (p_Result == nullptr)
    {
        WriteLog(LL_Error, "Result is null.");
        return false;
    }

    auto s_Proc = pfind(p_ProcessId);
    if (s_Proc == nullptr)
    {
        WriteLog(LL_Error, "could not get process.");
        return false;
    }
    // Proc is locked

    // Copy over all of the process information
    p_Result->ProcessId = s_Proc->p_pid;
    p_Result->OpPid = s_Proc->p_oppid;
    p_Result->DebugChild = s_Proc->p_dbg_child;
    p_Result->ExitThreads = s_Proc->p_exitthreads;
    p_Result->SigParent = s_Proc->p_sigparent;
    p_Result->Signal = s_Proc->p_sig;
    p_Result->Code = s_Proc->p_code;
    p_Result->Stops = s_Proc->p_stops;
    p_Result->SType = s_Proc->p_stype;

    // Make sure out buffer has enough size for the data
    static_assert(sizeof(p_Result->Name) >= sizeof(s_Proc->p_comm));
    static_assert(sizeof(p_Result->TitleId) >= sizeof(s_Proc->p_titleid));
    static_assert(sizeof(p_Result->ContentId) >= sizeof(s_Proc->p_contentid));
    static_assert(sizeof(p_Result->RandomizedPath) >= sizeof(s_Proc->p_randomized_path));
    static_assert(sizeof(p_Result->ElfPath) >= sizeof(s_Proc->p_elfpath));

    strlcpy(p_Result->Name, s_Proc->p_comm, sizeof(p_Result->Name));
    strlcpy(p_Result->TitleId, s_Proc->p_titleid, sizeof(p_Result->TitleId));
    strlcpy(p_Result->ContentId, s_Proc->p_contentid, sizeof(p_Result->ContentId));
    strlcpy(p_Result->RandomizedPath, s_Proc->p_randomized_path, sizeof(p_Result->RandomizedPath));
    strlcpy(p_Result->ElfPath, s_Proc->p_elfpath, sizeof(p_Result->ElfPath));

    _mtx_unlock_flags(&s_Proc->p_mtx, 0);
    // Proc is unlocked, don't touch it anymore

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

    if (s_Input.ThreadId <= 0)
        s_Input.ThreadId = p_Thread->td_tid;

    if (s_Input.ProcessId <= 0)
        s_Input.ProcessId = p_Thread->td_proc->p_pid;

    MiraThreadCredentials* s_Output = nullptr;

    switch (s_Input.State)
    {
    case MiraThreadCredentials::GSState::Get:
    {
        // Get the thread credentials
        if (!GetThreadCredentials(s_Input.ProcessId, s_Input.ThreadId, s_Output))
        {
            WriteLog(LL_Error, "could not get thread credentials.");
            return -1;
        }

        if (s_Output == nullptr)
        {
            WriteLog(LL_Error, "invalid output");
            return -1;
        }

        // Copyout the data back to userland
        s_Result = copyout(s_Output, p_Data, sizeof(*s_Output));

        // Free the resource
        delete s_Output;
        s_Output = nullptr;

        return (s_Result < 0 ? s_Result : -s_Result);
    }
    case MiraThreadCredentials::GSState::Set:
    {
        // Set the thread credentials
        if (!SetThreadCredentials(s_Input.ProcessId, s_Input.ThreadId, s_Input))
        {
            WriteLog(LL_Error, "could not set thread credentials.");
            return -1;
        }

        return 0;
    }
    default:
        WriteLog(LL_Error, "undefined state (%d).", s_Input.State);
        return -1;
    }

    return -1;
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
                if (p_ThreadId == -1 || l_Thread->td_tid == p_ThreadId)
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
                        l_ThreadCredential->cr_prison = *(struct prison**)kdlsym(prison0);

                    l_ThreadCredential->cr_sceAuthID = p_Input.SceAuthId;

                    // TODO: Static assert that these are equal
                    memcpy(l_ThreadCredential->cr_sceCaps, p_Input.Capabilities, sizeof(l_ThreadCredential->cr_sceCaps));

                    // TODO: Static assert that sizeof are equal
                    memcpy(l_ThreadCredential->cr_sceAttr, p_Input.Attributes, sizeof(l_ThreadCredential->cr_sceAttr));

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
    FOREACH_THREAD_IN_PROC(s_Process, l_Thread)
    {
        bool s_TidFound = false;

        if (l_Thread == nullptr)
            continue;

        // Lock the thread
        thread_lock(l_Thread);

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
