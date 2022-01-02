#include "MiraCtrl.hpp"
#include <Utils/Logger.hpp>
#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

#include <OrbisOS/MountManager.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/proc.h>
};

using namespace Mira::Driver::v1;

int32_t MiraCtrl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    return EINVAL;
}


int32_t MiraCtrl::OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
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
}
