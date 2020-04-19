#include "CtrlDriver.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/_Syscall.hpp>

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
    auto kmake_dev_p = (int(*)(int _flags, struct cdev **_cdev, struct cdevsw *_devsw, struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode, const char *_fmt, ...))kdlsym(make_dev_p);

    // Set up our device driver information
    m_DeviceSw.d_version = D_VERSION;
    m_DeviceSw.d_name = "mira";
    m_DeviceSw.d_open = OnOpen;
    m_DeviceSw.d_close = OnClose;
    m_DeviceSw.d_ioctl = OnIoctl;

    // Add the device driver

    int32_t s_ErrorDev = kmake_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK,
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
    struct devfs_rule dr;

    memset(&dr, 0, sizeof(struct devfs_rule));
    dr.dr_id = 0;
    dr.dr_magic = DEVFS_MAGIC;
    
    snprintf(dr.dr_pathptrn, DEVFS_MAXPTRNLEN, "mira");
    dr.dr_icond |= DRC_PATHPTRN;
    dr.dr_iacts |= DRA_BACTS;
    dr.dr_bacts |= DRB_UNHIDE;

    // Open devfs device (with is /dev/ ????)
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

    int32_t s_ErrorIoctl = kioctl_t(s_DevFS, 0x80EC4402, (char*)&dr, s_ProcessThread);
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
    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    switch (IOCGROUP(p_Command)) {
        case SUBSTITUTE_IOCTL_BASE: {
            return Mira::Plugins::Substitute::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
        }

        default: {
            WriteLog(LL_Debug, "unknown base (0x%02x) command: (0x%llx).", IOCGROUP(p_Command), p_Command);
            break;
        }
    }

    return 0;
}