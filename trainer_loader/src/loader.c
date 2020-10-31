#include "_syscalls.hpp"

static struct LoaderGlobals
{

};

int stub_open(char* path, int flags, int mode)
{
    return (int)syscall3(SYS_OPEN, path, (void*)flags, (void*)mode);
}

int stub_ioctl(int fd, unsigned long com, unsigned long long data)
{
    return (int)syscall3(SYS_IOCTL, (void*)fd, (void*)com, (void*)data);
}

int stub_close(int fd)
{
    return (int)syscall1(SYS_CLOSE, (void*)fd);
}

void main()
{
    // TODO: Open Mira driver
    int s_DriverDescriptor = stub_open("/dev/mira", 0, 0);
    if (s_DriverDescriptor <= 0)
        return;
    
    int s_Ret = stub_ioctl(s_DriverDescriptor, 0, 0);

    // TODO: Query the mira driver for active trainers

    // TODO: Request to load all available

    // TODO: Request for original start point

    // TODO: Jump to original start point OR wait for debugger
    
    // If debugger, then we wait for attachment and also grab save/send the information we got from the ioctl (original EP + process information)
}