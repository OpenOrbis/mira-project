// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CtrlDriver.hpp"
#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>
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

// v1 of Mira Ioctls
#include "v1/v1.hpp"

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
    m_MaxApiInfoCount(MaxApiInfoCount),
    m_ApiInfos(nullptr),
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

    WriteLog(LL_Debug, "MIRA_TRAINER_LOAD: (%x).", MIRA_TRAINER_LOAD);

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

    m_ApiInfos = new ApiInfo[m_MaxApiInfoCount];
    if (m_ApiInfos == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%d) api infos.");
        return;
    }

    for (uint32_t l_ApiIndex = 0; l_ApiIndex < m_MaxApiInfoCount; ++l_ApiIndex)
        ClearApiInfo(l_ApiIndex);
}

void CtrlDriver::ClearApiInfo(uint32_t p_Index)
{
    if (!IsInitialized())
        return;

    if (p_Index >= m_MaxApiInfoCount)
        return;

    ApiInfo* s_ApiInfo = &m_ApiInfos[p_Index];
    s_ApiInfo->ThreadId = InvalidId;
    s_ApiInfo->Version = CtrlDriverFlags::Latest;
}

void CtrlDriver::DestroyAllApiInfos()
{
    // TODO: Implement, not needed
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
        WriteLog(LL_Debug, "ctrl driver opened from tid: (%d) pid: (%d) (%s).", p_Thread->td_tid, p_Thread->td_proc->p_pid, p_Thread->td_proc->p_comm);

    // Validate that our flags are in bounds
    if (p_OFlags < CtrlDriverFlags::Latest || p_OFlags >= CtrlDriverFlags::COUNT)
        return (EINVAL);
    
    CtrlDriverFlags s_VersionFlags = (CtrlDriverFlags)p_OFlags;

    switch (s_VersionFlags)
    {
        // We do this to mask out Latest to the proper version
        case CtrlDriverFlags::v1:
        case CtrlDriverFlags::Latest:
            WriteLog(LL_Info, "Process requested v1 API.");
            s_VersionFlags = CtrlDriverFlags::v1;
            break;
        default:
            WriteLog(LL_Error, "invalid selection for version flags.");
            return (EINVAL);
    };

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return (ENOMEM);
    }

    auto s_Driver = s_Framework->GetDriver();
    if (s_Driver == nullptr)
    {
        WriteLog(LL_Error, "could not get driver.");
        return (ENOMEM);
    }

    // Check to see if this device has already been opened on this thread,
    // I have no clue why anyone would do this, but we need to support this
    // edge case in case of stupidity
    int32_t s_ExistingIndex = s_Driver->FindApiInfoByThreadId(p_Thread->td_tid);
    if (s_ExistingIndex != -1)
    {
        WriteLog(LL_Warn, "device driver opened from same thread multiple times, fix your code damnit.");
        return 0;
    }

    int32_t s_FreeApiInfoIndex = s_Driver->FindFreeApiInfoIndex();
    if (s_FreeApiInfoIndex == -1)
    {
        WriteLog(LL_Error, "there are no available api info slots.");
        return (ENOMEM);
    }

    ApiInfo* s_ApiInfo = &s_Driver->m_ApiInfos[s_FreeApiInfoIndex];
    s_ApiInfo->ThreadId = p_Thread->td_tid;
    s_ApiInfo->Version = s_VersionFlags;

    return 0;
}

int32_t CtrlDriver::FindFreeApiInfoIndex() const
{
    if (!IsInitialized())
        return -1;
    
    for (uint32_t l_Index = 0; l_Index < m_MaxApiInfoCount; ++l_Index)
    {
        ApiInfo* l_Info = &m_ApiInfos[l_Index];
        if (l_Info->ThreadId == InvalidId)
            return l_Index;
    }

    return -1;
}

int32_t CtrlDriver::FindApiInfoByThreadId(int32_t p_ThreadId)
{
    if (!IsInitialized())
        return -1;
    
    for (uint32_t l_Index = 0; l_Index < m_MaxApiInfoCount; ++l_Index)
    {
        ApiInfo* l_Info = &m_ApiInfos[l_Index];
        if (l_Info->ThreadId == p_ThreadId)
            return l_Index;
    }

    return -1;
}

int32_t CtrlDriver::OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread)
{
    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver closed from tid: (%d) pid: (%d) (%s).", p_Thread->td_tid, p_Thread->td_proc->p_pid, p_Thread->td_proc->p_comm);
    
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return 0;
    }

    auto s_Driver = s_Framework->GetDriver();
    if (s_Driver == nullptr)
    {
        WriteLog(LL_Error, "could not get driver.");
        return 0;
    }

    auto s_ApiInfoIndex = s_Driver->FindApiInfoByThreadId(p_Thread->td_tid);
    if (s_ApiInfoIndex == -1)
    {
        WriteLog(LL_Warn, "driver closure with no api info tid: (%d).", p_Thread->td_tid);
        return 0;
    }

    // Clear out the previous api info
    s_Driver->ClearApiInfo(s_ApiInfoIndex);

    return 0;
}

int32_t CtrlDriver::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    //auto copyinstr = (int(*)(const void *uaddr, void *kaddr, size_t len, size_t *done))kdlsym(copyinstr);

    //auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    p_Command = p_Command & 0xFFFFFFFF; // Clear the upper32

    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d) (%s).", p_Thread->td_tid, p_Thread->td_proc->p_pid, p_Thread->td_proc->p_comm);

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get mira framework.");
        return ENOMEM;
    }

    auto s_Driver = s_Framework->GetDriver();
    if (s_Driver == nullptr)
    {
        WriteLog(LL_Error, "could not get mira driver.");
        return ENOMEM;
    }

    auto s_ApiInfoIndex = s_Driver->FindApiInfoByThreadId(p_Thread->td_tid);
    if (s_ApiInfoIndex == -1)
    {
        WriteLog(LL_Error, "could not find api info.");
        return ENOENT;
    }

    auto s_Version = s_Driver->m_ApiInfos[s_ApiInfoIndex].Version;

    switch (IOCGROUP(p_Command)) 
    {
        case MIRA_IOCTL_BASE:
        {
            switch (s_Version)
            {
                case CtrlDriverFlags::v1:
                    return v1::v1Ctrl::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                default:
                    WriteLog(LL_Error, "unknown driver version.");
                    break;
            }
        }

        default:
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
    }

    return EINVAL;
}

/*
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
*/