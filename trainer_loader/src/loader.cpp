#include "_syscalls.hpp"
#include <mira/Driver/DriverCmds.hpp>

// struct LoaderGlobals
// {
//     bool EntryBP;
// };

// static struct LoaderGlobals g_Globals = {
//     .EntryBP = false
// };

int64_t stub_open(const char* path, int flags, int mode)
{
    return (int64_t)syscall3(SYS_OPEN, (void*)path, (void*)(int64_t)flags, (void*)(uint64_t)mode);
}

int64_t stub_ioctl(int fd, unsigned long com, unsigned long long data)
{
    return (int64_t)syscall3(SYS_IOCTL, (void*)(int64_t)fd, (void*)(uint64_t)com, (void*)data);
}

int64_t stub_close(int fd)
{
    return (int64_t)syscall1(SYS_CLOSE, (void*)(int64_t)fd);
}

extern "C" void loader_entry()
{
    // TODO: Open Mira driver
    int s_DriverDescriptor = stub_open("/dev/mira", 0, 0);
    if (s_DriverDescriptor <= 0)
        return;

    // Request for original start point
    void* s_EntryPoint = NULL;

    int s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_ORIG_EP, (uint64_t)&s_EntryPoint);
    if (s_Ret != 0)
        return;

    if (s_EntryPoint == NULL)
        return;

    // TODO: Request to load all available
    s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_LOAD, 0);
    if (s_Ret != 0)
        return;

    // TODO: Jump to original start point OR wait for debugger
    if (s_EntryPoint)
    {
        for (;;)
            __asm__("nop");
    }

    ((void(*)())s_EntryPoint)();
    
    // If debugger, then we wait for attachment and also grab save/send the information we got from the ioctl (original EP + process information)
}