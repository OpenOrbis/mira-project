#include "TrainerManager.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/mman.h>
    #include <sys/proc.h>
    #include <sys/stat.h>
};

using namespace Mira::Trainers;

const char* TrainerManager::c_ShmPrefix = "_shm_";

TrainerManager::TrainerManager()
{

}

TrainerManager::~TrainerManager()
{

}

bool TrainerManager::OnLoad()
{
    return true;
}

bool TrainerManager::OnUnload()
{
    return true;
}

bool TrainerManager::OnProcessExit(struct proc* p_Process)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
	//auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    // Validate that we have a process
    if (p_Process == nullptr)
        return false;

    // Lock the process whenever we access it
    PROC_LOCK(p_Process);

    do
    {
        // Debug print
        WriteLog(LL_Info, "process is exiting: (%s).", p_Process->p_comm);

    } while (false);
    

    // Unlock the process
    PROC_UNLOCK(p_Process);

    return true;
}

bool TrainerManager::OnProcessExecEnd(struct proc* p_Process)
{
    //auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	//auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
	auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    // Validate process
    if (p_Process == nullptr)
        return false;
    
    // Make sure that we have the eboot.bin
    if (strncmp("eboot.bin", p_Process->p_comm, sizeof("eboot.bin")) == 0)
    {
        WriteLog(LL_Error, "skipping non eboot process (%s).", p_Process->p_comm);
        return false;
    }
    
    // Check if the file exists
    if (FileExists("/mnt/usb0/mira/trainers/test.prx"))
    {
        WriteLog(LL_Debug, "injecting /mira/usb0/mira/trainers/test.prx");
        return ThreadInjection("/mnt/usb0/mira/trainers/test.prx", p_Process);
    }

    return true;
}

bool TrainerManager::GetUsbTrainerPath(char*& p_OutputString, uint32_t& p_OutputStringLength)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    p_OutputString = nullptr;
    p_OutputStringLength = 0;
    
    // Iterate through each of the usb inices
    for (auto l_UsbIndex = 0; l_UsbIndex < 6; ++l_UsbIndex)
    {
        // Construct the usb folder path
        char l_UsbPath[PATH_MAX] = { 0 };
        auto l_Length = snprintf(l_UsbPath, sizeof(l_UsbPath), "/mnt/usb%d/mira/trainers", l_UsbIndex);

        // Check if the directory exists
        if (!DirectoryExists(l_UsbPath))
            continue;
        
        // Allocate the output string
        p_OutputString = new char[l_Length];
        if (p_OutputString == nullptr)
        {
            WriteLog(LL_Error, "could not allocate output string of length (%d).", l_Length);
            return false;
        }
        p_OutputStringLength = l_Length;

        // Copy out our string
        memcpy(p_OutputString, l_UsbPath, l_Length);

        return true;
    }

    return false;
}

bool TrainerManager::GenerateShmId(char* p_OutputString, uint32_t p_OutputStringLength)
{
    // TODO: Get a random source of entropy
    auto rand = [](){ return 4; /* chosen by a fair dice roll */ };
    auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
    auto s_PrefixLength = strlen(c_ShmPrefix);

    // Validate our output string
    if (p_OutputString == nullptr)
    {
        WriteLog(LL_Error, "could not generate shm id: invalid output string.");
        return false;
    }
    
    // Validate that our output string length is enough for the prefix and at least 1 characters
    if (p_OutputStringLength <= s_PrefixLength + 1)
    {
        WriteLog(LL_Error, "could not generate shm id: length (%d) < prefix + 1 length (%d).", p_OutputStringLength, s_PrefixLength + 1);
        return false;
    }

    static const char s_Dict[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    auto s_DictLength = strlen(s_Dict);

    // Zero out our buffer
    memset(p_OutputString, 0, p_OutputStringLength);

    // Copy the prefix
    memcpy(p_OutputString, c_ShmPrefix, s_PrefixLength);

    // Randomly generate the rest of it
    for (auto i = s_PrefixLength; i < p_OutputStringLength; ++i)
        p_OutputString[i] = s_Dict[rand() % (s_DictLength - 1)];
    
    return true;
}

bool TrainerManager::CreateShm(char*& p_OutId)
{
    p_OutId = nullptr;

    // Generate a new shm id
    char s_Id[MaxShmIdLength] = { 0 };
    if (!GenerateShmId(s_Id, sizeof(s_Id)))
    {
        WriteLog(LL_Error, "could not generate shm id.");
        return false;
    }

    WriteLog(LL_Debug, "Generated Shm Id: (%s).", s_Id);

    //auto s_Handle = kshm_open("", 0, 0,);
    return true;
}

bool TrainerManager::DeleteShm(const char* p_Id)
{
    return false;
}

const char *strrchr(const char *s, int c)
{
    const char* ret=nullptr;
    do {
        if( *s == (char)c )
            ret=s;
    } while(*s++);
    return ret;
}


// "/mnt/usb0/mira/trainers/test.prx"
bool TrainerManager::ThreadInjection(const char* p_TrainerPrxPath, struct proc* p_TargetProc)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    // Validate the proc
    if (p_TargetProc == nullptr)
    {
        WriteLog(LL_Error, "invalid proc.");
        return false;
    }

    // Validate the trainer prx path
    if (p_TrainerPrxPath == nullptr)
    {
        WriteLog(LL_Error, "invalid trainer prx path.");
        return false;
    }

    WriteLog(LL_Info, "injecting prx: (%s).", p_TrainerPrxPath);

    // Get the target thread
    auto s_TargetProcMainThread = p_TargetProc->p_threads.tqh_first;
    if (s_TargetProcMainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get proc main thread (%p) (%s).", p_TargetProc, p_TargetProc->p_comm);
        return false;
    }

    WriteLog(LL_Info, "got target proc main thread: (%p).", s_TargetProcMainThread);

    // Get path from the file name
    auto s_TrainerFileName = strrchr(p_TrainerPrxPath, '/'); // "/test.prx"
    if (s_TrainerFileName == nullptr)
    {
        WriteLog(LL_Error, "could not find the filename in (%s).", p_TrainerPrxPath);
        return false;
    }

    WriteLog(LL_Info, "trainer file name with slash: (%s).", s_TrainerFileName);

    // Calculate the host path length
    auto s_HostPathLength = (uint64_t)s_TrainerFileName - (uint64_t)p_TrainerPrxPath;
    if (s_HostPathLength == 0 || s_HostPathLength >= PATH_MAX)
    {
        WriteLog(LL_Error, "invalid host path length (%lx).", s_HostPathLength);
        return false;
    }

    // "/mnt/usb0/_mira/trainers"
    char s_HostMountDirectory[PATH_MAX] = { 0 };
    
    // Copy the string to stack, "/mnt/usb0/_mira/trainers"
    memcpy(s_HostMountDirectory, p_TrainerPrxPath, s_HostPathLength);

    WriteLog(LL_Info, "host mount directory: (%s).", s_HostMountDirectory);

    // Mount the host directory in the sandbox
    auto s_Result = OrbisOS::Utilities::MountInSandbox(s_HostMountDirectory, "_substitute", s_TargetProcMainThread);
    if (s_Result < 0)
    {
        WriteLog(LL_Error, "could not mount (%s) into the sandbox in (_substitute).", s_Result);
        return false;
    }

    WriteLog(LL_Info, "host directory mounted to _substitute");

    auto s_Success = false;

    // Lock the process
    PROC_LOCK(p_TargetProc);

    do
    {
        char s_SandboxTrainerPath[PATH_MAX] = { 0 };
        snprintf(s_SandboxTrainerPath, sizeof(s_SandboxTrainerPath), "/_substitute%s", s_TrainerFileName); // Should print /_substitute/test.prx

        WriteLog(LL_Info, "sandbox trainer path: (%s).", s_SandboxTrainerPath);

        /* code */
        // TODO: Iterate the trainer directory for prx files
        // TODO: Load all prx files
        int32_t s_PrxHandle = -1;
        auto s_Ret = kdynlib_load_prx_t(s_SandboxTrainerPath, 0, 0, 0, 0, &s_PrxHandle, s_TargetProcMainThread);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "dynlib_load_prx return (%d).", s_Ret);
            break;
        }

        WriteLog(LL_Debug, "prxHandle: (%d) ret (%d).", s_PrxHandle, s_Ret);

        // Check the handle value
        if (s_PrxHandle < 0)
        {
            WriteLog(LL_Error, "dynlib_load_prx handle returned (%d).", s_PrxHandle);
            break;
        }

        // Get the address of the entrypoint
        void* s_ModuleStart = nullptr;
        s_Ret = kdynlib_dlsym_t(s_PrxHandle, "module_load", &s_ModuleStart, s_TargetProcMainThread);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "dynlib_dlsym returned (%d).", s_Ret);
            break;
        }

        WriteLog(LL_Error, "module_load: (%p).", s_ModuleStart);

        // Validate the module starting address
        if (s_ModuleStart == nullptr)
        {
            WriteLog(LL_Error, "module start not found.");
            break;
        }

        s_Ret = OrbisOS::Utilities::CreatePOSIXThread(p_TargetProc, s_ModuleStart);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not create posix thread (%d).", s_Ret);
            break;
        }

        s_Success = s_Ret == 0;
    } while (false);
    
    // Unlock the process
    PROC_UNLOCK(p_TargetProc);

    return s_Success;
}


bool TrainerManager::PayloadInjection()
{
    // This must be called from sv_fixup in the sysentvec

    // TODO: Require the payload to inject

    // dlsym the object (TrainerBoot)
    

    return false;
}

bool TrainerManager::FileExists(const char* p_Path)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread.");
        return false;
    }

    struct stat s_Stat = { 0 };
    auto s_Ret = kstat_t(const_cast<char*>(p_Path), &s_Stat, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not stat (%s) ret (%d).", p_Path, s_Ret);
        return false;
    }

    return S_ISREG(s_Stat.st_mode);
}

bool TrainerManager::DirectoryExists(const char* p_Path)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread.");
        return false;
    }

    struct stat s_Stat = { 0 };
    auto s_Ret = kstat_t(const_cast<char*>(p_Path), &s_Stat, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not stat (%s) ret (%d).", p_Path, s_Ret);
        return false;
    }

    return S_ISDIR(s_Stat.st_mode);
}