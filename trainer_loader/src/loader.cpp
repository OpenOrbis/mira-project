#include "_syscalls.hpp"
#include <mira/Driver/DriverCmds.hpp>

// struct LoaderGlobals
// {
//     bool EntryBP;
// };

// static struct LoaderGlobals g_Globals = {
//     .EntryBP = false
// };

extern "C" uint64_t _g_rsi;
extern "C" uint64_t _g_rdi;

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

int stub_dup2(int from, int to)
{
    return (int32_t)(int64_t)syscall2(SYS_DUP2, (void*)(int64_t)from, (void*)(int64_t)to);
}

extern "C" void loader_entry(uint64_t p_Rdi, uint64_t p_Rsi)
{
    _g_rdi = p_Rdi;
    _g_rsi = p_Rsi;

    /*int s_Console = stub_open("/dev/console", 1, 0);
    if (s_Console < 0)
        return;
    
    stub_dup2(s_Console, 1);
    stub_dup2(1, 2);*/

    // Open Mira driver
    int s_DriverDescriptor = stub_open("/dev/mira", 0, 0);
    if (s_DriverDescriptor < 0)
    {
        *(uint8_t*)0x1111 = 0x0;
        return;
    }

    // Request for original start point
    void* s_EntryPoint = (void*)NULL;
    int s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_ORIG_EP, (uint64_t)&s_EntryPoint);
    if (s_Ret != 0)
    {
        *(uint8_t*)0x1222 = 0x0;
        return;
    }

    // Validate the entry point
    if (s_EntryPoint == NULL)
    {
        *(uint8_t*)0x1333 = 0x0;
        return;
    }

    // Request to load all available trainers (logic is done in kernel, is this bad idea? probably...)
    /*s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_LOAD, 0);
    if (s_Ret != 0)
    {
        *(uint8_t*)0x1444 = 0x0;
        return;
    }
    
    .text:0000000000028A2C                 pxor    mm0, mm0
.text:0000000000028A2F                 pxor    mm1, mm1
.text:0000000000028A32                 pxor    mm2, mm2
.text:0000000000028A35                 pxor    mm3, mm3
.text:0000000000028A38                 pxor    mm4, mm4
.text:0000000000028A3B                 pxor    mm5, mm5
.text:0000000000028A3E                 pxor    mm6, mm6
.text:0000000000028A41                 pxor    mm7, mm7
.text:0000000000028A44                 emms
.text:0000000000028A46                 vzeroall
.text:0000000000028A49                 xor     r15, r15
.text:0000000000028A4C                 xor     r14, r14
.text:0000000000028A4F                 xor     r13, r13
.text:0000000000028A52                 xor     r12, r12
.text:0000000000028A55                 xor     r11, r11
.text:0000000000028A58                 xor     r10, r10
.text:0000000000028A5B                 xor     r9, r9
.text:0000000000028A5E                 xor     r8, r8
.text:0000000000028A61                 xor     rdx, rdx
.text:0000000000028A64                 xor     rcx, rcx
.text:0000000000028A67                 xor     rbx, rbx
.text:0000000000028A6A                 xor     rax, rax
.text:0000000000028A6D                 jmp     [rsp+10h+var_18]
*/

    // Prepare to jump yeehaw
    /*__asm__(
        ".intex_syntax\n"
        "pxor mm0, mm0\n"
    );*/

    // Close access to the driver
    stub_close(s_DriverDescriptor);

    ((void(*)(uint64_t, uint64_t))s_EntryPoint)(p_Rdi, p_Rsi);
    
    // If debugger, then we wait for attachment and also grab save/send the information we got from the ioctl (original EP + process information)
}