#include "_syscalls.hpp"
#include <mira/Driver/DriverCmds.hpp>

#include <sys/types.h>

// struct LoaderGlobals
// {
//     bool EntryBP;
// };

// static struct LoaderGlobals g_Globals = {
//     .EntryBP = false
// };

extern "C" uint64_t _g_rsi;
extern "C" uint64_t _g_rdi;
extern "C" uint32_t _g_consoleHandle;
extern "C" uint8_t _g_launchMode;

typedef enum _LaunchMode : uint8_t
{
    Normal = 0,
    Injected = 1,
    COUNT = 2
} LaunchMode;

// Libkernel
int (*sceKernelLoadStartModule)(const char *name, size_t argc, const void *argv, unsigned int flags, int, int) = nullptr;

// Libc
typedef struct DIR DIR;
typedef struct FILE FILE;
struct dirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t d_type;
    uint8_t d_namlen;
    char d_name[255 + 1];
};
DIR *(*opendir)(const char *filename) = nullptr;
struct dirent *(*readdir)(DIR *dirp) = nullptr;
int (*closedir)(DIR *dirp) = nullptr;
int (*printf)(const char * format, ...) = nullptr;
int (*snprintf)(char *str, size_t size, const char *format, ...) = nullptr;
size_t (*strlen)(const char *s) = nullptr;
int (*strcmp)(const char *s1, const char *s2) = nullptr;
int* (*__error)() = nullptr;

int64_t stub_open(const char* path, int flags, int mode)
{
    return (int64_t)syscall3(SYS_OPEN, (void*)path, (void*)(int64_t)flags, (void*)(uint64_t)mode);
}

int64_t stub_getdents(int fd, void* buf, uint64_t count) {
    return (int64_t)syscall3(SYS_GETDENTS, (void*)(int64_t)fd, buf, (void*)count);
}

int64_t stub_getdirentries(int fd, char  *buf, size_t nbytes, uint64_t* basep) {
    return (int64_t)syscall4(SYS_GETDIRENTRIES, (void*)(int64_t)fd, (void*)buf, (void*)nbytes, (void*)basep);
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

int64_t stub_loadprx(const char* p_PrxPath, int* p_OutModuleId)
{
    return (int64_t)syscall4(594, reinterpret_cast<void*>(const_cast<char*>(p_PrxPath)), 0, p_OutModuleId, 0);
}

int64_t stub_dlsym(int64_t p_PrxId, const char* p_FunctionName, void* p_DestinationFunctionOffset)
{
    return (int64_t)syscall3(591, (void*)p_PrxId, (void*)p_FunctionName, p_DestinationFunctionOffset);
}

int64_t stub_load_prx(const char* p_PrxPath, int* p_OutModuleId)
{
    return (int64_t)syscall4(594, reinterpret_cast<void*>(const_cast<char*>(p_PrxPath)), 0, p_OutModuleId, 0);
}

int64_t stub_unload_prx(int64_t p_PrxId)
{
    return (int64_t)syscall1(595, (void*)p_PrxId);
}

unsigned int my_strlen(const char *s)
{
    unsigned int count = 0;
    while(*s!='\0')
    {
        count++;
        s++;
    }
    return count;
}

int64_t stub_debug_log(const char* debug_message) {
    if (printf)
        printf(debug_message);
    
    if (_g_consoleHandle >= 0)
        syscall3(3, (void*)(int64_t)_g_consoleHandle, (void*)debug_message, (void*)(int64_t)my_strlen(debug_message));

    return (int64_t)syscall3(601, (void*)0x7, reinterpret_cast<void*>(const_cast<char*>(debug_message)), (void*)0x0);
}

static void IterateDirectory(const char* p_Path, void* p_Args, void(*p_Callback)(void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type))
{
    if (p_Callback == nullptr) {
        stub_debug_log("IterateDirectory: Callback is null !\n");
        return;
    }

    DIR* s_Directory = opendir(p_Path);
    if (s_Directory == nullptr) {
        char data[100];
        int errnbr = -100;
        int* error_ptr = __error();
        if (error_ptr)
            errnbr = *error_ptr;

        snprintf(data, 100, "IterateDirectory: s_Directory is null (%p) (error: %i)!\n", s_Directory, errnbr);
        stub_debug_log(data);
        return;
    }
    
    struct dirent* s_CurrentDir = nullptr;
    while ((s_CurrentDir = readdir(s_Directory)) != nullptr) {
        char data[100];
        snprintf(data, 100, "IterateDirectory: Name = %s \n", s_CurrentDir->d_name);
        stub_debug_log(data);
        p_Callback(p_Args, p_Path, s_CurrentDir->d_name, s_CurrentDir->d_type);
    }
    closedir(s_Directory);
    stub_debug_log("IterateDirectory: Done.\n");
}

extern "C" void loader_entry(uint64_t p_Rdi, uint64_t p_Rsi)
{    
    _g_rdi = p_Rdi;
    _g_rsi = p_Rsi;

    int32_t s_LibKernelHandle = -1;
    int32_t s_LibcModuleId = -1;
    int32_t s_RetOpenPoint = -1;
    int32_t s_Ret = -1;
    _g_consoleHandle = -1;
    void* s_EntryPoint = (void*)nullptr;
    char s_LogBuffer[100];
    //MiraSetThreadPrivMask s_PrivCheck;

    stub_debug_log("[+] trainer_loader entry reached!\n");

    // Debug log the launch mode type
    if (_g_launchMode == LaunchMode::Normal)
        stub_debug_log("[=] launching in normal mode.\n");
    else if (_g_launchMode == LaunchMode::Injected)
        stub_debug_log("[=] launching in injected mode.\n");
    else
        stub_debug_log("[-] unknown launch mode, results may vary.\n");

    // Open up stdout
    _g_consoleHandle = stub_open("/dev/console", 0x0002, 0);

    stub_debug_log("[+] /dev/console opened.\n");

    // Open Mira driver
    int s_DriverDescriptor = stub_open("/dev/mira", 0, 0);
    if (s_DriverDescriptor < 0)
    {
        stub_debug_log("[-] could not open /dev/mira.\n");
        *(uint8_t*)0x1337 = 0x0;
        goto jmp_orig;
    }

    stub_debug_log("[+] Hello from Loader land !\n");

    if (_g_launchMode == LaunchMode::Normal)
    {
        stub_debug_log("[=] launch mode set to normal.\n");

        // Request for original start point
        s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_ORIG_EP, (uint64_t)&s_EntryPoint);
        if (s_Ret != 0)
        {
            stub_debug_log("[-] No entry point found ?\n");
            goto jmp_orig;
        }

        // Validate the entry point
        if (s_EntryPoint == nullptr)
        {
            stub_debug_log("[-] Entry point is null 2!\n");
            goto jmp_orig;
        }
    }

    // Request to load all available trainers (logic is done in kernel, is this bad idea? probably...)
    stub_debug_log("[=] requesting trainer load via ioctl.\n");
    s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_LOAD, 0);
    if (s_Ret != 0)
    {
        stub_debug_log("[-] Unable to load trainer !\n");
        goto jmp_orig;
    }
    
    // s_PrivCheck.IsGet = true;
    // s_PrivCheck.ThreadId = -1;

    // s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_PRIV_CHECK, (uint64_t)&s_PrivCheck);
    // if (s_Ret != 0)
    // {
    //     snprintf(s_LogBuffer, sizeof(s_LogBuffer), "[-] priv check failed (%d).\n", s_Ret);
    //     stub_debug_log(s_LogBuffer);
    //     goto jmp_orig;
    // }
    // snprintf(s_LogBuffer, sizeof(s_LogBuffer), "MIRA_PRIV_CHECK GET returned (%d).\n", s_Ret);
    // stub_debug_log(s_LogBuffer);

    stub_debug_log("[+] Loading kernel library ...\n");

    // Resolve libkernel.sprx
    stub_load_prx("libkernel.sprx", &s_LibKernelHandle);
    if (s_LibKernelHandle == -1)
    {
        stub_load_prx("libkernel_web.prx", &s_LibKernelHandle);
        if (s_LibKernelHandle == -1)
        {
            stub_load_prx("libkernel_sys.sprx", &s_LibKernelHandle);
        }
    }

    stub_debug_log("[+] Checking error ...\n");

    // Before we do anything find error
    stub_dlsym(s_LibKernelHandle, "__error", &__error);
    if (__error == nullptr)
    {
        stub_debug_log("[-] __error is nullptr !\n");
        goto jmp_orig;
    }
    
    // Resolve sceKernelLoadStartModule
    stub_dlsym(s_LibKernelHandle, "sceKernelLoadStartModule", &sceKernelLoadStartModule);
    if (sceKernelLoadStartModule == nullptr)
    {
        stub_debug_log("[-] sceKernelLoadStartModule is null !\n");
        goto jmp_orig;
    }

    stub_debug_log("[+] sceKernelLoadStartModule found ! loading libc ...\n");

    // Load the libc module
    s_LibcModuleId = sceKernelLoadStartModule("libSceLibcInternal.sprx", 0, nullptr, 0, 0, 0);
    if (s_LibcModuleId == -1) {
        stub_debug_log("[-] Libc not found !\n");
        goto jmp_orig;
    }

    // Resolve the needed functions to iterate
    stub_dlsym(s_LibcModuleId, "opendir", &opendir);
    stub_dlsym(s_LibcModuleId, "readdir", &readdir);
    stub_dlsym(s_LibcModuleId, "closedir", &closedir);
    stub_dlsym(s_LibcModuleId, "snprintf", &snprintf);
    stub_dlsym(s_LibcModuleId, "printf", &printf);
    stub_dlsym(s_LibcModuleId, "strlen", &strlen);
    stub_dlsym(s_LibcModuleId, "strcmp", &strcmp);

    stub_debug_log("[+] Iterate over '.' ...\n");

    s_RetOpenPoint = stub_open(".", 0, 0);
    snprintf(s_LogBuffer, 100, "open(., o_directory): %d \n", s_RetOpenPoint);
    stub_debug_log(s_LogBuffer);

    IterateDirectory(".", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
    {
        stub_debug_log("'.': OK.\n");
    });


    stub_debug_log("[+] Iterate over / ...\n");

    IterateDirectory("/", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
    {
        stub_debug_log("/: OK.\n");
    });

    stub_debug_log("[+] Iterate over _mira ...\n");

    // Iterate _mira directory
    IterateDirectory("/_mira", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
    {
        // Get the fullpath
        char s_PrxPath[260];
        snprintf(s_PrxPath, sizeof(s_PrxPath), "%s/%s", p_BasePath, p_Name);

        // Check to make sure the file name ends in .prx
        auto s_Length = strlen(s_PrxPath);
        if (s_Length < 4) {
            stub_debug_log("_mira: lowen than 4 characters.\n");
            return;
        }
        
        // Check if the end file path ends with ".prx"
        if (strcmp(&s_PrxPath[s_Length - 4], ".prx") != 0) {
            stub_debug_log("_mira:not a prx.\n");
            return;
        }
        
        // Load the trainer
        int32_t s_TrainerModuleId = sceKernelLoadStartModule(s_PrxPath, 0, nullptr, 0, 0, 0);
        stub_debug_log("[+] _mira: loading prx ...\n");

        int32_t(*s_TrainerLoadEntrypoint)() = nullptr;
        stub_dlsym(s_TrainerModuleId, "trainer_load", &s_TrainerLoadEntrypoint);

        if (s_TrainerLoadEntrypoint != nullptr) {
            (void)s_TrainerLoadEntrypoint();
            stub_debug_log("[+] _mira: trainer launched :D.\n");
        }
    });

    stub_debug_log("[+] Iterate over _substitute ...\n");

    // Iterate _substitute directory
    IterateDirectory("/_substitute", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
    {
        // Get the fullpath
        char s_PrxPath[260];
        snprintf(s_PrxPath, sizeof(s_PrxPath), "%s/%s", p_BasePath, p_Name);

        // s_PrxPath = /_substitute/whatever.prx

        // Check to make sure the file name ends in .prx
        auto s_Length = strlen(s_PrxPath);
        if (s_Length < 4) {
            stub_debug_log("_substitute: lowen than 4 characters.\n");
            return;
        }
        
        // Check if the end file path ends with ".prx"
        if (strcmp(&s_PrxPath[s_Length - 4], ".prx") != 0) {
            stub_debug_log("_substitute: not a prx.\n");
            return;
        }
        
        // Load the trainer
        int32_t s_TrainerModuleId = sceKernelLoadStartModule(s_PrxPath, 0, nullptr, 0, 0, 0);
        stub_debug_log("_substitute: loading prx ...\n");

        int32_t(*s_TrainerLoadEntrypoint)() = nullptr;
        stub_dlsym(s_TrainerModuleId, "trainer_load", &s_TrainerLoadEntrypoint);

        if (s_TrainerLoadEntrypoint != nullptr) {
            (void)s_TrainerLoadEntrypoint();
            stub_debug_log("_substitute: trainer launched ! :D\n");
        }
    });

    stub_debug_log("[+] End of PRX search !\n");

    stub_debug_log("[+] Goodbye ! Return to entrypoint :D\n");

    // Close access to the driver
    stub_close(s_DriverDescriptor);

jmp_orig:
    if (_g_launchMode == LaunchMode::Normal)
    {
        if (s_EntryPoint != nullptr)
        {
            snprintf(s_LogBuffer, sizeof(s_LogBuffer), "[=] Jumping to entry point (%p)\n", s_EntryPoint);
            stub_debug_log(s_LogBuffer);
            ((void(*)(uint64_t, uint64_t))s_EntryPoint)(p_Rdi, p_Rsi);
        }
    }
}