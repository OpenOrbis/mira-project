#include "TrainerManager.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Driver/CtrlDriver.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/mman.h>
    #include <sys/proc.h>
    #include <sys/stat.h>
    #include <sys/dirent.h>
    #include <sys/fcntl.h>
    #include <sys/sysent.h>
    #include <sys/imgact.h>
    #include <sys/imgact_elf.h>
};

using namespace Mira::Trainers;

extern uint8_t* _trainer_loader_start;
extern uint8_t* _trainer_loader_end;

uint32_t g_TrainerLoaderSize = 0;

const char* TrainerManager::c_ShmPrefix = "_shm_";
TrainerManager::sv_fixup_t TrainerManager::g_sv_fixup = nullptr;

TrainerManager::TrainerManager()
{
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);

    WriteLog(LL_Debug, "Original sv_fixup: %p (%x).", sv->sv_fixup, (reinterpret_cast<uint64_t>(sv->sv_fixup) - reinterpret_cast<uint64_t>(gKernelBase)));

    // Back up the original sv_fixup
    g_sv_fixup = reinterpret_cast<sv_fixup_t>(sv->sv_fixup);

    // Overwrite our sv_fixup
    sv->sv_fixup = (int(*)(register_t **, struct image_params *))OnSvFixup;

    // We need to calculate the elf size on the fly, otherwise it's 0 for some odd reason
	g_TrainerLoaderSize = (uint64_t)&_trainer_loader_end - (uint64_t)&_trainer_loader_start;
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

        auto s_Driver = Mira::Framework::GetFramework()->GetDriver();
        if (s_Driver != nullptr)
        {
            WriteLog(LL_Debug, "Entrypoint for pid (%d) being removed.", p_Process->p_pid);
            s_Driver->RemoveEntryPoint(p_Process->p_pid);
        }

    } while (false);
    

    // Unlock the process
    PROC_UNLOCK(p_Process);

    return true;
}

bool TrainerManager::OnProcessExecEnd(struct proc* p_Process)
{
    return true;

    /*if (p_Process == nullptr)
        return false;
    
    WriteLog(LL_Debug, "TrainerManager, Pid: (%d).", p_Process->p_pid);

    //auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	//auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
	auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    // Validate process
    if (p_Process == nullptr)
        return false;
    
    // Make sure that we have the eboot.bin
    if (strncmp("eboot.bin", p_Process->p_comm, sizeof("eboot.bin")) != 0)
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

    return true;*/
}

int TrainerManager::OnSvFixup(register_t** stack_base, struct image_params* imgp)
{
    WriteLog(LL_Debug, "OnSvFixup called.");
    //auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	//auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
	auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    if (g_sv_fixup == nullptr)
    {
        WriteLog(LL_Error, "Yo what the hell are you doing?, you need to save sv_fixup before trying to use it >_>.");
        return -1; //g_sv_fixup(stack_base, imgp);
    }

    // If we don't have any image params bail
    if (imgp == nullptr)
        return g_sv_fixup(stack_base, imgp);

    do
    {
        // Get the process
        auto s_Process = imgp->proc;
        if (s_Process == nullptr)
        {
            WriteLog(LL_Error, "there is no process.");
            break;
        }

        WriteLog(LL_Debug, "execPath: (%s), execPath2: (%s).", imgp->execpath, imgp->execpath2);

        WriteLog(LL_Debug, "TrainerManager, Pid: (%d).", s_Process->p_pid);

        auto s_Driver = Mira::Framework::GetFramework()->GetDriver();
        if (s_Driver == nullptr)
        {
            WriteLog(LL_Error, "no device driver found, what?");
            break;
        }

        // Get the original entry point
        Elf64_Auxargs* s_AuxArgs = static_cast<Elf64_Auxargs*>(imgp->auxargs);
        WriteLog(LL_Debug, "auxArgs: %p.", s_AuxArgs);
        if (s_AuxArgs == nullptr)
        {
            WriteLog(LL_Error, "could not get auxargs.");
            break;
        }

        auto s_TrainerLoaderEntry = InjectTrainerLoader(s_Process);

        WriteLog(LL_Debug, "TrainerLoader EntryPoint: (%p).", s_TrainerLoaderEntry);

        s_AuxArgs->entry = reinterpret_cast<Elf64_Size>(s_TrainerLoaderEntry);

        // Update our driver
        s_Driver->AddOrUpdateEntryPoint(s_Process->p_pid, reinterpret_cast<void*>(s_AuxArgs->entry));
        
        // Make sure that we have the eboot.bin
        auto s_EbootPath = "/app0/eboot.bin";
        if (strncmp(s_EbootPath, imgp->execpath, sizeof(s_EbootPath)) != 0)
        {
            WriteLog(LL_Error, "skipping non eboot process (%s).", s_Process->p_comm);
            break;
        }
        
        // Check if the file exists
        /*auto s_ExamplePath = "/mnt/usb0/mira/trainers/test.prx";
        if (FileExists(s_ExamplePath))
        {
            WriteLog(LL_Debug, "injecting %s.", s_ExamplePath);
            if (!ThreadInjection(s_ExamplePath, s_Process, imgp))
                WriteLog(LL_Error, "could not inject (%s).", s_ExamplePath);
        }*/

    } while (false);

    // Call the original
    return g_sv_fixup(stack_base, imgp);
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

void PrintDirectory(const char* p_Path, bool p_Recursive, struct thread* p_Thread, uint8_t p_Indent = 0)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    auto s_DirectoryHandle = kopen_t(p_Path, O_RDONLY | O_DIRECTORY, 0777, p_Thread);
    if (s_DirectoryHandle < 0)
        return;        
    
    WriteLog(LL_Debug, "Dir: %s", p_Path);
    // Switch this to use stack
    char s_Buffer[PATH_MAX] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));

    char s_CurrentFullPath[PATH_MAX] = { 0 };

    char s_Indents[256] = { 0 };
    memset(s_Indents, 0, sizeof(s_Indents));
    memset(s_Indents, '\t', p_Indent);

    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), p_Thread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);

            snprintf(s_CurrentFullPath, sizeof(s_CurrentFullPath), "%s/%s", p_Path, l_Dent->d_name);
            
            WriteLog(LL_Debug, "%s[%s] (%s).",s_Indents, (l_Dent->d_type == DT_DIR ? "D" : "F"), s_CurrentFullPath);

            if (l_Dent->d_type == DT_DIR && p_Recursive)
                PrintDirectory(s_CurrentFullPath, p_Recursive, p_Thread, p_Indent + 1);
            
            l_Pos += l_Dent->d_reclen;
        }
    }
    kclose_t(s_DirectoryHandle, p_Thread);
}

uint8_t* TrainerManager::InjectTrainerLoader(struct proc* p_TargetProcess)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);

    // Validate target process
    if (p_TargetProcess == nullptr)
        return nullptr;
    
    // Get the main thread of the target process
    auto s_MainThread = p_TargetProcess->p_singlethread ? p_TargetProcess->p_singlethread : p_TargetProcess->p_threads.tqh_first;
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get process (%s) (%d) main thread.", p_TargetProcess->p_comm, p_TargetProcess->p_pid);
        return nullptr;
    }

    // Allocate new memory inside of our target process
    auto s_Address = kmmap_t(nullptr, g_TrainerLoaderSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PREFAULT_READ, -1, 0, s_MainThread);
    if (s_Address == nullptr || s_Address == MAP_FAILED)
    {
        WriteLog(LL_Error, "could not allocate new target process memory of size (%d).", g_TrainerLoaderSize);
        return nullptr;
    }

    WriteLog(LL_Debug, "Allocated Address: (%p).", s_Address);

    // Write out the trainer loader into the process address space
    auto s_Result = copyout(&_trainer_loader_start, s_Address, g_TrainerLoaderSize);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not write trainer loader data to process (%d).", s_Result);
        return nullptr;
    }
    
    // Set the protection to RWX (does this require a patch)
    /*s_Result = kmprotect_t(s_Address, g_TrainerLoaderSize, PROT_READ | PROT_EXEC, s_MainThread);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not set RWX protection on trainer loader (%d).", s_Result);
        return nullptr;
    }*/

    return reinterpret_cast<uint8_t*>(s_Address);
}

// "/mnt/usb0/mira/trainers/test.prx"
bool TrainerManager::ThreadInjection(const char* p_TrainerPrxPath, struct proc* p_TargetProc, struct image_params* p_Params)
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

    //PrintDirectory("/", false, s_TargetProcMainThread);

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
    char s_HostMountDirectory[260] = { 0 };
    
    // Copy the string to stack, "/mnt/usb0/_mira/trainers"
    memcpy(s_HostMountDirectory, p_TrainerPrxPath, s_HostPathLength);

    WriteLog(LL_Info, "host mount directory: (%s).", s_HostMountDirectory);

    // Mount the host directory in the sandbox
    char s_MountedSandboxDirectory[260] = { 0 };
    auto s_Result = OrbisOS::Utilities::MountInSandbox(s_HostMountDirectory, "/_substitute", s_MountedSandboxDirectory, s_TargetProcMainThread);
    if (s_Result < 0)
    {
        WriteLog(LL_Error, "could not mount (%s) into the sandbox in (_substitute).", s_Result);
        return false;
    }

    // s_MountedSandboxDirectory = "/mnt/sandbox/NPXS22010_000/_substitute"
    WriteLog(LL_Info, "host directory mounted to (%s).", s_MountedSandboxDirectory);

    auto s_Success = false;

    // Lock the process
    PROC_LOCK(p_TargetProc);

    do
    {
        char s_SandboxTrainerPath[260] = { 0 };
        snprintf(s_SandboxTrainerPath, sizeof(s_SandboxTrainerPath), "%s%s", s_MountedSandboxDirectory, s_TrainerFileName); // Should print <sandboxpath>/_substitute/test.prx

        // s_SandboxTrainerPath = "/mnt/sandbox/NPXS22010_000/_substitute/test.prx"
        WriteLog(LL_Info, "sandbox trainer path: (%s).", s_SandboxTrainerPath);
        
        // PrintDirectory("/_substitute", false, s_TargetProcMainThread);
        // PrintDirectory("/mnt", false, s_TargetProcMainThread);
        //PrintDirectory("/app0", false, s_TargetProcMainThread);

        /* code */
        // TODO: Iterate the trainer directory for prx files
        // TODO: Load all prx files
        int32_t s_PrxHandle = -1;
        auto s_Ret = kdynlib_load_prx_t((char*)"/_substitute/test.prx", 0, &s_PrxHandle, s_TargetProcMainThread);
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
        s_Ret = kdynlib_dlsym_t(s_PrxHandle, "_Z19testLibraryFunctionv", &s_ModuleStart, s_TargetProcMainThread);
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

        // TODO: Overwrite imgp->auxshit
        // Create a new thread
        /*s_Ret = OrbisOS::Utilities::CreatePOSIXThread(p_TargetProc, s_ModuleStart);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not create posix thread (%d).", s_Ret);
            break;
        }*/
        auto s_AuxArgs = static_cast<Elf64_Auxargs*>(p_Params->auxargs);
        if (s_AuxArgs)
        {
            WriteLog(LL_Debug, "modifying auxargs.");
            s_AuxArgs->entry = (Elf64_Size)s_ModuleStart;
        }

        s_Success = s_Ret == 0;
    } while (false);
    
    // Unlock the process
    PROC_UNLOCK(p_TargetProc);

    return s_Success;
}

void ParseDirectory(const char* p_Path)
{
    // Debug output
    WriteLog(LL_Debug, "ParsingDirectory: (%s).", p_Path);

    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    auto s_MiraThread = Mira::Framework::GetFramework()->GetMainThread();

    auto s_DirectoryHandle = kopen_t(p_Path, 0x0000 | 0x00020000, 0777, s_MiraThread);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", p_Path, s_DirectoryHandle);
        return;
    }

    uint64_t s_DentCount = 0;

    // Switch this to use stack
    const uint32_t c_BufferSize = 0x1000;
    char* s_Buffer = new char[c_BufferSize];
    if (s_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate buffer, path: (%s).", p_Path);
        kclose_t(s_DirectoryHandle, s_MiraThread);
        return;
    }
    memset(s_Buffer, 0, c_BufferSize);

    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, c_BufferSize);
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, c_BufferSize, s_MiraThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            // Get the new directory entry
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);

            // Increase the dent count
            s_DentCount++;

            // Allocate a new recursive directory path name
            char* s_FullPath = new char[_MAX_PATH];

            // Validate our allocation
            if (s_FullPath == nullptr)
            {
                WriteLog(LL_Error, "could not allocate full path.");
                l_Pos += l_Dent->d_reclen;
                continue;
            }

            // Zero out the buffer and build the string
            memset(s_FullPath, 0, _MAX_PATH);
            snprintf(s_FullPath, _MAX_PATH, "%s/%s", p_Path, l_Dent->d_name);

            

            // Based on the type either recurse or spit out
            switch (l_Dent->d_type)
            {
                case DT_DIR:
                {
                    // TODO: Check via TitleId
                    // TODO: Check via ProcessName
                    WriteLog(LL_Debug, "DIRECTORY: (%s).", s_FullPath);

                    // Call the recursive function
                    ParseDirectory(s_FullPath);
                    break;
                }
                case DT_REG:
                    WriteLog(LL_Debug, "FILE: (%s).", s_FullPath);
                break;
            }

            // When completed free the memory we allocated
            if (s_FullPath)
            {
                memset(s_FullPath, 0, _MAX_PATH);
                delete [] s_FullPath;
            }
            
            l_Pos += l_Dent->d_reclen;
        }
    }
    kclose_t(s_DirectoryHandle, s_MiraThread);

    // Free the allocated memory
    if (s_Buffer)
        delete [] s_Buffer;
}

bool TrainerManager::LoadTrainers(struct proc* p_TargetProcess)
{
    // Validate target process
    if (p_TargetProcess == nullptr)
    {
        WriteLog(LL_Error, "could not load trainers for invalid process.");
        return false;
    }

    // We will need to be doing file io so we need the rooted process
    auto s_MiraThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MiraThread == nullptr)
    {
        WriteLog(LL_Error, "could not get mira main thread.");
        return false;
    }

    // We will get the title id and version information
    // The flash drive/hdd should be formatted as such

    // /mira/trainers/<TitleId>/<Version>/blah.prx
    // /mira/trainers/<ProcName>/blah.prx
    if (!DirectoryExists("/mnt/usb0/mira/trainers"))
    {
        WriteLog(LL_Error, "could not find usb trainers directory...");
        return false;
    }

    // TODO: Determine which root path we are loading from, for now we will hardcode
    const char* s_TrainersPath = "/mnt/usb0/mira/trainers";

    ParseDirectory(s_TrainersPath);

    return true;
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