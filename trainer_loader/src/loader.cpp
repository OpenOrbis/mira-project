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
int (*snprintf)(char *str, size_t size, const char *format, ...) = nullptr;
size_t (*strlen)(const char *s) = nullptr;
int (*strcmp)(const char *s1, const char *s2) = nullptr;
int (*fprintf)(FILE *stream, const char *format, ...) = nullptr;
FILE *(*fopen)(const char *filename, const char *mode) = nullptr;
size_t (*fread)(void *ptr, size_t size, size_t count, FILE *stream) = nullptr;
size_t (*fwrite)(const void * ptr, size_t size, size_t count, FILE *stream ) = nullptr;
int (*fclose)(FILE *stream) = nullptr;

static void IterateDirectory(const char* p_Path, void* p_Args, void(*p_Callback)(void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type))
{
    if (p_Callback == nullptr)
        return;
    
    DIR* s_Directory = opendir(p_Path);
    if (s_Directory == nullptr)
        return;
    
    struct dirent* s_CurrentDir = nullptr;
    while ((s_CurrentDir = readdir(s_Directory)) != nullptr)
        p_Callback(p_Args, p_Path, s_CurrentDir->d_name, s_CurrentDir->d_type);
    closedir(s_Directory);
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
    s_Ret = stub_ioctl(s_DriverDescriptor, MIRA_TRAINERS_LOAD, 0);
    if (s_Ret != 0)
    {
        *(uint8_t*)0x1444 = 0x0;
        return;
    }

    // Close access to the driver
    stub_close(s_DriverDescriptor);

    // Resolve libkernel.sprx
    int32_t s_LibKernelHandle = -1;
    stub_load_prx("libkernel.sprx", &s_LibKernelHandle);
    if (s_LibKernelHandle == -1)
    {
        stub_load_prx("libkernel_web.prx", &s_LibKernelHandle);
        if (s_LibKernelHandle == -1)
        {
            stub_load_prx("libkernel_sys.sprx", &s_LibKernelHandle);
        }
    }
    
    // Resolve sceKernelLoadStartModule
    stub_dlsym(s_LibKernelHandle, "sceKernelLoadStartModule", &sceKernelLoadStartModule);
    if (sceKernelLoadStartModule)
    {
        // Load the libc module
        int32_t s_LibcModuleId = sceKernelLoadStartModule("libSceLibcInternal.sprx", 0, NULL, 0, 0, 0);

        // Resolve the needed functions to iterate
        stub_dlsym(s_LibcModuleId, "opendir", &opendir);
        stub_dlsym(s_LibcModuleId, "readdir", &readdir);
        stub_dlsym(s_LibcModuleId, "closedir", &closedir);
        stub_dlsym(s_LibcModuleId, "snprintf", &snprintf);
        stub_dlsym(s_LibcModuleId, "strlen", &strlen);
        stub_dlsym(s_LibcModuleId, "strcmp", &strcmp);
        stub_dlsym(s_LibcModuleId, "fprintf", &fprintf);
        stub_dlsym(s_LibcModuleId, "fopen", &fopen);
        stub_dlsym(s_LibcModuleId, "fread", &fread);
        stub_dlsym(s_LibcModuleId, "fwrite", &fwrite);
        stub_dlsym(s_LibcModuleId, "fclose", &fclose);

        

        // Iterate _mira directory
        IterateDirectory("/_mira", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
        {
            // Get the fullpath
            char s_PrxPath[260];
            snprintf(s_PrxPath, sizeof(s_PrxPath), "%s/%s", p_BasePath, p_Name);

            // s_PrxPath = /_mira/whatever.prx
            FILE* s_File = fopen("/dev/deci_stdout", "rw");
            fprintf(s_File, "HELLO WORLD FROM TRAINER LOADER STUB\n");
            fprintf(s_File, s_PrxPath);
            fprintf(s_File, s_PrxPath);
            fclose(s_File);

            // Check to make sure the file name ends in .prx
            auto s_Length = strlen(s_PrxPath);
            if (s_Length < 4)
                return;
            
            // Check if the end file path ends with ".prx"
            if (strcmp(&s_PrxPath[s_Length - 4], ".prx") != 0)
                return;
            
            // Load the trainer
            int32_t s_TrainerModuleId = sceKernelLoadStartModule(s_PrxPath, 0, NULL, 0, 0, 0);
            
            int32_t(*trainer_load)() = nullptr;
            stub_dlsym(s_TrainerModuleId, "trainer_load", &trainer_load);

            if (trainer_load)
                (void)trainer_load();
        });

        // Iterate _substitute directory
        IterateDirectory("/_substitute", nullptr, [](void* p_Args, const char* p_BasePath, char* p_Name, int32_t p_Type)
        {
            // Get the fullpath
            char s_PrxPath[260];
            snprintf(s_PrxPath, sizeof(s_PrxPath), "%s/%s", p_BasePath, p_Name);

            // s_PrxPath = /_substitute/whatever.prx

            // Check to make sure the file name ends in .prx
            auto s_Length = strlen(s_PrxPath);
            if (s_Length < 4)
                return;
            
            // Check if the end file path ends with ".prx"
            if (strcmp(&s_PrxPath[s_Length - 4], ".prx") != 0)
                return;
            
            // Load the trainer
            int32_t s_TrainerModuleId = sceKernelLoadStartModule(s_PrxPath, 0, NULL, 0, 0, 0);
            
            int32_t(*trainer_load)() = nullptr;
            stub_dlsym(s_TrainerModuleId, "trainer_load", &trainer_load);

            if (trainer_load != nullptr)
                (void)trainer_load();
        });
    }

    ((void(*)(uint64_t, uint64_t))s_EntryPoint)(p_Rdi, p_Rsi);
}