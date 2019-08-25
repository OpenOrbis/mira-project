#include "CtrlDriver.hpp"
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>

#include <sys/proc.h>


using namespace Mira::Driver;

CtrlDriver::CtrlDriver() :
    m_DeviceSw { 0 },
    m_Device(nullptr)
{
    // Set up our device driver information
    m_DeviceSw.d_version = D_VERSION;
    m_DeviceSw.d_name = "mira";
    m_DeviceSw.d_open = OnOpen;
    m_DeviceSw.d_close = OnClose;
    m_DeviceSw.d_ioctl = OnIoctl;

    // Add the device driver
    auto kmake_dev_p = (int(*)(int _flags, struct cdev **_cdev, struct cdevsw *_devsw,
		struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode,
		const char *_fmt, ...))kdlsym(make_dev_p);
    int32_t s_Error = kmake_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK,
        &m_Device,
        &m_DeviceSw,
        nullptr,
        UID_ROOT,
        GID_WHEEL,
        0777,
        "mira");
    
    switch (s_Error)
    {
    case 0:
        WriteLog(LL_Debug, "device driver created successfully!");
        break;
    case EEXIST:
        WriteLog(LL_Error, "could not create device driver, device driver already exists.");
        break;
    default:
        WriteLog(LL_Error, "could not create device driver (%d).", s_Error);
        break;
    }
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
    if (p_Thread != nullptr && p_Thread->td_proc)
        WriteLog(LL_Debug, "ctrl driver ioctl from tid: (%d) pid: (%d).", p_Thread->td_tid, p_Thread->td_proc->p_pid);

    return 0;
}