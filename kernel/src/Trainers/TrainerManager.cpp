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

    #include <vm/pmap.h>
    #include <machine/pmap.h>      
    #include <vm/vm_map.h>
    #include <vm/vm.h>
    #include <sys/dynlib.h>
};

using namespace Mira::Trainers;

extern uint8_t* _trainer_loader_start;
extern uint8_t* _trainer_loader_end;

//uint32_t g_TrainerLoaderSize = 0;

const char* TrainerManager::c_ShmPrefix = "_shm_";
TrainerManager::sv_fixup_t TrainerManager::g_sv_fixup = nullptr;

TrainerManager::TrainerManager()
{
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);

    WriteLog(LL_Debug, "Original sv_fixup: %p (%x).", sv->sv_fixup, (reinterpret_cast<uint64_t>(sv->sv_fixup) - reinterpret_cast<uint64_t>(gKernelBase)));

    // Back up the original sv_fixup
    g_sv_fixup = reinterpret_cast<sv_fixup_t>(sv->sv_fixup);

    // Overwrite our sv_fixup
    sv->sv_fixup = (int(*)(register_t **, struct image_params *))OnSvFixup;

    // Initialize process info space
    memset(&m_ProcessInfo, 0, sizeof(m_ProcessInfo));

    // Initalize all of the shared memorys
    memset(&m_Shms, 0, sizeof(m_Shms));

    // Initialize the mutex
    mtx_init(&m_Mutex, "MiraTM", MTX_DEF, 0);
}

TrainerManager::~TrainerManager()
{
    memset(&m_ProcessInfo, 0, sizeof(m_ProcessInfo));
    memset(&m_Shms, 0, sizeof(m_Shms));

    auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);
	mtx_destroy(&m_Mutex);
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
        //WriteLog(LL_Info, "process is exiting: (%s) (%d).", p_Process->p_comm, p_Process->p_pid);

        auto s_TrainerManager = Mira::Framework::GetFramework()->GetTrainerManager();
        if (s_TrainerManager != nullptr)
            s_TrainerManager->RemoveEntryPoint(p_Process->p_pid);

    } while (false);
    

    // Unlock the process
    PROC_UNLOCK(p_Process);

    return true;
}

int TrainerManager::OnSvFixup(register_t** stack_base, struct image_params* imgp)
{
    //WriteLog(LL_Debug, "OnSvFixup called.");
	auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    if (g_sv_fixup == nullptr)
    {
        WriteLog(LL_Error, "Yo what the hell are you doing?, you need to save sv_fixup before trying to use it >_>.");
        return -1;
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

        //WriteLog(LL_Debug, "execPath: (%s), execPath2: (%s).", imgp->execpath, imgp->execpath2);

        //WriteLog(LL_Debug, "TrainerManager, Pid: (%d).", s_Process->p_pid);

        auto s_TrainerManager = Mira::Framework::GetFramework()->GetTrainerManager();
        if (s_TrainerManager == nullptr)
        {
            WriteLog(LL_Error, "no trainer manager found, what?");
            break;
        }

        // Get the original entry point
        Elf64_Auxargs* s_AuxArgs = static_cast<Elf64_Auxargs*>(imgp->auxargs);
        //WriteLog(LL_Debug, "auxArgs: %p.", s_AuxArgs);
        if (s_AuxArgs == nullptr)
        {
            WriteLog(LL_Error, "could not get auxargs.");
            break;
        }

        // Make sure that we have the eboot.bin
        auto s_EbootPath = "/app0/eboot.bin";
        if (strncmp(s_EbootPath, imgp->execpath, sizeof(s_EbootPath)) != 0)
        {
            WriteLog(LL_Error, "skipping non eboot process (%s).", s_Process->p_comm);
            break;
        }

        auto s_EbootShortPath = "eboot.bin";
        if (strcmp(s_EbootShortPath, s_Process->p_comm) != 0)
        {
            WriteLog(LL_Error, "p_comm != eboot.bin");
            break;
        }

        // Allocate the trainer loader inside of the process
        auto s_TrainerLoaderEntry = AllocateTrainerLoader(s_Process);
        if (s_TrainerLoaderEntry == nullptr)
        {
            WriteLog(LL_Error, "invalid trainer entry point.");
            break;
        }

        // Update our driver
        s_TrainerManager->AddOrUpdateEntryPoint(s_Process->p_pid, reinterpret_cast<void*>(s_AuxArgs->entry));

        WriteLog(LL_Debug, "TrainerLoader EntryPoint: (%p) pid (%d).", s_TrainerLoaderEntry, s_Process->p_pid);

        // Set the new entry point inside of the trainer loader
        s_AuxArgs->entry = reinterpret_cast<Elf64_Size>(s_TrainerLoaderEntry);
        
    } while (false);

    // Call the original
    return g_sv_fixup(stack_base, imgp);
}

bool TrainerManager::GetUsbTrainerPath(char* p_OutputString, uint32_t p_OutputStringLength)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    if (p_OutputString == nullptr || p_OutputStringLength == 0)
    {
        WriteLog(LL_Error, "invalid output string or length.");
        return false;
    }

    // Zero out the output buffer
    memset(p_OutputString, 0, p_OutputStringLength);

    // Iterate through each of the usb inices
    for (auto l_UsbIndex = 0; l_UsbIndex < 6; ++l_UsbIndex)
    {
        // Construct the usb folder path
        char l_UsbPath[PATH_MAX] = { 0 };
        // /mnt/usb%d/mira/trainers 
        auto l_Length = snprintf(l_UsbPath, sizeof(l_UsbPath), "/data/mira/trainers", l_UsbIndex);

        // Check if the directory exists
        if (!DirectoryExists(l_UsbPath))
            continue;

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

uint8_t* TrainerManager::AllocateTrainerLoader(struct proc* p_TargetProcess)
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

    auto s_Size = ((uint64_t)&_trainer_loader_end - (uint64_t)&_trainer_loader_start);
    WriteLog(LL_Debug, "TrainerLoaderStart: %p, End: %p, Size: %x", &_trainer_loader_start, &_trainer_loader_end, s_Size);

    // Allocate new memory inside of our target process
    auto s_Address = AllocateProcessMemory(p_TargetProcess, s_Size);
    if (s_Address == nullptr)
    {
        WriteLog(LL_Error, "could not allocate process memory (%x).", s_Size);
        return nullptr;
    }

    WriteLog(LL_Debug, "Allocated Address: (%p).", s_Address);

    // Write out the trainer loader into the process address space
    auto s_Result = copyout(&_trainer_loader_start, s_Address, s_Size);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not write trainer loader data to process (%d).", s_Result);
        return nullptr;
    }

    WriteLog(LL_Info, "Copied TrainerLoader from (%p) to (%p) sz (%x).", &_trainer_loader_start, s_Address, s_Size);

    return reinterpret_cast<uint8_t*>(s_Address);
}

bool TrainerManager::LoadTrainers(struct thread* p_CallingThread)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);

    // Validate target process
    if (p_CallingThread == nullptr)
    {
        WriteLog(LL_Error, "could not load trainers for invalid thread.");
        return false;
    }

    auto s_CallingProc = p_CallingThread->td_proc;
    if (s_CallingProc == nullptr)
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

    // /mnt/usbN/mira/trainers
    const uint32_t c_PathLength = 260;
    char s_BasePath[c_PathLength];
    uint32_t s_BasePathLength = sizeof(s_BasePath);
    if (!GetUsbTrainerPath(s_BasePath, s_BasePathLength))
    {
        WriteLog(LL_Error, "could not find usb trainers directory...");
        return false;
    }

    auto s_ProcessTitleId = ((char*)s_CallingProc) + 0x390; // "CUSA00001";

    // /mnt/usbN/mira/trainers/CUSA00001
    char s_TitleIdPath[c_PathLength];
    snprintf(s_TitleIdPath, sizeof(s_TitleIdPath), "%s/%s", s_BasePath, s_ProcessTitleId);
    if (!DirectoryExists(s_TitleIdPath))
    {
        WriteLog(LL_Error, "could not find trainers directory for title id (%s).", s_ProcessTitleId);
        return false;
    }

    // Determine if we need to mount the _stubstitute path into the sandbox
    if (!DirectoryExists(p_CallingThread, "/_substitute"))
    {
        WriteLog(LL_Info, "Substitute directory not found for proc (%d) (%s).", s_CallingProc->p_pid, s_CallingProc->p_comm);

        // Mount the host directory in the sandbox
        char s_MountedSandboxDirectory[c_PathLength] = { 0 };
        auto s_Result = OrbisOS::Utilities::MountInSandbox(s_TitleIdPath, "/_substitute", s_MountedSandboxDirectory, p_CallingThread);
        if (s_Result < 0)
        {
            WriteLog(LL_Error, "could not mount (%s) into the sandbox in (_substitute).", s_Result);
            return false;
        }

        // s_MountedSandboxDirectory = "/mnt/sandbox/NPXS22010_000/_substitute"
        WriteLog(LL_Info, "host directory mounted to (%s).", s_MountedSandboxDirectory);
    }

    // Get the _mira folder on the usb path
    char s_MiraModulePath[c_PathLength];
    snprintf(s_MiraModulePath, sizeof(s_MiraModulePath), "%s/_mira", s_BasePath);
    if (!DirectoryExists(s_MiraModulePath))
    {
        WriteLog(LL_Error, "could not find trainers directory for mira modules (%s).", s_MiraModulePath);
        return false;
    }

    // Determine if we need to mount the _mira path into the sandbox
    if (!DirectoryExists(p_CallingThread, "/_mira"))
    {
        WriteLog(LL_Info, "mira directory not found for proc (%d) (%s).", s_CallingProc->p_pid, s_CallingProc->p_comm);

        // Mount the host directory in the sandbox
        char s_MountedSandboxDirectory[c_PathLength] = { 0 };
        auto s_Result = OrbisOS::Utilities::MountInSandbox(s_MiraModulePath, "/_mira", s_MountedSandboxDirectory, p_CallingThread);
        if (s_Result < 0)
        {
            WriteLog(LL_Error, "could not mount (%s) into the sandbox in (_mira).", s_Result);
            return false;
        }

        // s_MountedSandboxDirectory = "/mnt/sandbox/NPXS22010_000/_substitute"
        WriteLog(LL_Info, "host directory mounted to (%s).", s_MountedSandboxDirectory);
    }
    return true;
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

    return DirectoryExists(s_MainThread, p_Path);
}

bool TrainerManager::DirectoryExists(struct thread* p_Thread, const char* p_Path)
{
    auto s_MainThread = p_Thread;
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

// This function assumes that the process is already locked
uint8_t* TrainerManager::AllocateProcessMemory(struct proc* p_Process, uint32_t p_Size)
{
    auto _vm_map_lock = (void(*)(vm_map_t map, const char* file, int line))kdlsym(_vm_map_lock);
    //auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	//auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
    auto _vm_map_findspace = (int(*)(vm_map_t map, vm_offset_t start, vm_size_t length, vm_offset_t *addr))kdlsym(_vm_map_findspace);
    auto _vm_map_insert = (int(*)(vm_map_t map, vm_object_t object, vm_ooffset_t offset,vm_offset_t start, vm_offset_t end, vm_prot_t prot, vm_prot_t max, int cow))kdlsym(_vm_map_insert);
    auto _vm_map_unlock = (void(*)(vm_map_t map))kdlsym(_vm_map_unlock);

    if (p_Process == nullptr)
        return nullptr;
    
    WriteLog(LL_Info, "Requested Size: (%x).", p_Size);
    p_Size = round_page(p_Size);
    WriteLog(LL_Info, "Adjusted Size (%x).", p_Size);

    vm_offset_t s_Address = 0;

    // Get the vmspace
    auto s_VmSpace = p_Process->p_vmspace;
    if (s_VmSpace == nullptr)
    {
        WriteLog(LL_Info, "invalid vmspace.");
        return nullptr;
    }

    // Get the vmmap
    vm_map_t s_VmMap = &s_VmSpace->vm_map;

    // Lock the vmmap
    _vm_map_lock(s_VmMap, __FILE__, __LINE__);

    do
    {
        // Find some free space to allocate memory
        auto s_Result = _vm_map_findspace(s_VmMap, s_VmMap->header.start, p_Size, &s_Address);
        if (s_Result != 0)
        {
            WriteLog(LL_Error, "vm_map_findspace returned (%d).", s_Result);
            break;
        }

        WriteLog(LL_Debug, "_vm_map_findspace returned address (%p).", s_Address);

        // Validate the address
        if (s_Address == 0)
        {
            WriteLog(LL_Error, "allocated address is invalid (%p).", s_Address);
            break;
        }

        // Insert the new stuff map
        s_Result = _vm_map_insert(s_VmMap, NULL, 0, s_Address, s_Address + p_Size, VM_PROT_ALL, VM_PROT_ALL, 0);
        if (s_Result != 0)
        {
            WriteLog(LL_Error, "vm_map_insert returned (%d).", s_Result);
            break;
        }

    } while (false);

    _vm_map_unlock(s_VmMap);

    return reinterpret_cast<uint8_t*>(s_Address);
}


void TrainerManager::AddOrUpdateEntryPoint(int32_t p_ProcessId, void* p_EntryPoint)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_ProcessId <= 0)
    {
        WriteLog(LL_Error, "invalid process id (%d).", p_ProcessId);
        return;
    }

    if (p_EntryPoint == nullptr)
    {
        WriteLog(LL_Error, "invalid entry point (%p).", p_EntryPoint);
        return;
    }

    auto s_UpdatedEntryPoint = false;
    auto s_AddedEntryPoint = false;

    _mtx_lock_flags(&m_Mutex, 0);

    do
    {
        // Attempt to update the entry point
        for (auto l_Index = 0; l_Index < MaxTrainerProcInfos; ++l_Index)
        {
            auto& l_Info = m_ProcessInfo[l_Index];
            if (l_Info.ProcessId == p_ProcessId)
            {
                WriteLog(LL_Info, "Updating Entrypoint for pid (%d) to (%p).", p_ProcessId, p_EntryPoint);
                l_Info.EntryPoint = p_EntryPoint;
                s_UpdatedEntryPoint = true;
                break;
            }
        }

        // If we have updated an entry point don't worry about adding it
        if (s_UpdatedEntryPoint)
            break;
        
        // Since there had been no updates, add the new entry point
        for (auto l_Index = 0; l_Index < MaxTrainerProcInfos; ++l_Index)
        {
            auto& l_Info = m_ProcessInfo[l_Index];
            if (l_Info.ProcessId <= 0)
            {
                WriteLog(LL_Info, "Adding Entrypoint for pid (%d) as (%p).", p_ProcessId, p_EntryPoint);
                l_Info.EntryPoint = p_EntryPoint;
                l_Info.ProcessId = p_ProcessId;
                s_AddedEntryPoint = true;
                break;
            }
        }
    } while (false);
    
    _mtx_unlock_flags(&m_Mutex, 0);

    if (!s_AddedEntryPoint && !s_UpdatedEntryPoint)
        WriteLog(LL_Error, "There was an error adding or updating entry point (%p) for pid (%d).", p_EntryPoint, p_ProcessId);
}

void TrainerManager::RemoveEntryPoint(int32_t p_ProcessId)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    auto s_Removed = false;

    _mtx_lock_flags(&m_Mutex, 0);

    do
    {
        for (auto l_Index = 0; l_Index < MaxTrainerProcInfos; ++l_Index)
        {
            auto& l_Info = m_ProcessInfo[l_Index];
            if (p_ProcessId == l_Info.ProcessId)
            {
                WriteLog(LL_Info, "Removing Entrypoint (%p) from (%d).", l_Info.EntryPoint, p_ProcessId);
                l_Info.ProcessId = -1;
                l_Info.EntryPoint = nullptr;
                s_Removed = true;
                break;
            }
        }
    } while (false);

    _mtx_unlock_flags(&m_Mutex, 0);

    //WriteLog(LL_Error, "could not remove Entrypoint for (%d).", p_ProcessId);
}

void* TrainerManager::GetEntryPoint(int32_t p_ProcessId)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_ProcessId <= 0)
        return nullptr;
    
    void* s_Found = nullptr;

    _mtx_lock_flags(&m_Mutex, 0);

    for (auto l_Index = 0; l_Index < MaxTrainerProcInfos; ++l_Index)
    {
        auto& l_Info = m_ProcessInfo[l_Index];
        if (l_Info.ProcessId == p_ProcessId)
        {
            s_Found = l_Info.EntryPoint;
            break;
        }
    }
    
    _mtx_unlock_flags(&m_Mutex, 0);

    return s_Found;
}

void* TrainerManager::FindJmpslotAddress(struct proc* p_Process, const char* p_ModuleName, const char* p_Name, int32_t p_Flags) 
{
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto name_to_nids = (void(*)(const char *name, const char *nids_out))kdlsym(name_to_nids);
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    if (p_Name == nullptr || p_Process == nullptr) 
    {
        WriteLog(LL_Error, "Invalid argument.");
        return 0;   
    }

    char* s_TitleId = (char*)((uint64_t)p_Process + 0x390);

    // Get the nids of the function
    char s_Nids[0xD] = { 0 };
    if ( (p_Flags & SUBSTITUTE_IAT_NIDS) )
        snprintf(s_Nids, sizeof(s_Nids), "%s", p_Name); // nids = name
    else
        name_to_nids(p_Name, s_Nids); // nids calculated by name

    caddr_t s_NidsOffsetFound = 0;

    // Determine if we need to lock the process, then do it if needed
    bool s_ProcessUnlockNeeded = false;
    auto s_ProcessLocked = PROC_LOCKED(p_Process);
    if (s_ProcessLocked == false)
    {
        _mtx_lock_flags(&p_Process->p_mtx, 0);
        s_ProcessUnlockNeeded = true;
    }

    do
    {
        auto s_DynlibObj = p_Process->p_dynlib->objs.slh_first;
        if (s_DynlibObj == nullptr)
        {
            WriteLog(LL_Error, "[%s] The process (%d) (%s) is not dynamically linkable.", s_TitleId, p_Process->p_pid, p_Process->p_comm);
            break;
        }

        // Lock dynlib object (Note: Locking will panic kernel sometime)
        // BUG: Determine if we need to lock at all, or if the lock is already held (unlocking and causing races later)
        struct sx* s_DynlibBindLock = &p_Process->p_dynlib->bind_lock; //(struct sx*)((uint64_t)p->p_dynlib + 0x70);
        A_sx_xlock_hard(s_DynlibBindLock, 0);

        do
        {
            // Get the main dynlib object
            auto s_MainDynlibObj = p_Process->p_dynlib->main_obj;
            if (s_MainDynlibObj == nullptr) 
            {
                WriteLog(LL_Error, "main dynlib object is nullptr.");
                break;
            }

            // Search in all library
            auto s_DynlibObj = s_MainDynlibObj;

            // Check if we not are in the main executable
            if (strncmp(p_ModuleName, SUBSTITUTE_MAIN_MODULE, SUBSTITUTE_MAX_NAME) == 0)
            {
                WriteLog(LL_Debug, "not the main executable bailing.");
                break;
            }

            for (;;) 
            {
                char* s_LibName = (char*)(*(uint64_t*)(s_DynlibObj + 8));

                // If the libname (a path) containt the module name, it's the good object, break it
                if (s_LibName && strstr(s_LibName, p_ModuleName)) {
                    break;
                }

                s_DynlibObj = s_DynlibObj->link.sle_next; //*(uint64_t*)(s_DynlibObj);
                if (s_DynlibObj == nullptr) 
                {
                    WriteLog(LL_Error, "Unable to find the library.");
                    break;
                }
            }

            // Get the main relocbase address, for calculation after
            caddr_t s_RelocBase = s_DynlibObj->realloc_base; //(uint64_t)(*(uint64_t*)(s_DynlibObj + 0x70));

            // Get the pfi
            auto s_Pfi = s_DynlibObj->pfi;
            if (s_Pfi == nullptr)
            {
                WriteLog(LL_Error, "pfi invalid.");
                break;
            }

            caddr_t s_StringTable = s_Pfi->strtab;

            size_t s_PltRelaSize = s_Pfi->pltrelasize;
            caddr_t s_PltRela = s_Pfi->pltrela;

            if (s_PltRela == nullptr || s_PltRelaSize == 0)
            {
                WriteLog(LL_Error, "pltrela (%p) or pltrelasize (%lx) invalid.", s_PltRela, s_PltRelaSize);
                break;
            }

            const Elf64_Rela* s_RelaStart = (const Elf64_Rela*)s_PltRela;
            const Elf64_Rela* s_RelaEnd = (const Elf64_Rela*)(s_PltRela + s_PltRelaSize);

            for (const Elf64_Rela* s_Current = s_RelaStart; s_Current != s_RelaEnd; ++s_Current)
            {
                uint64_t s_SymbolIndex = ELF64_R_SYM(s_Current->r_info); // (*(uint64_t*)(s_Current + 0x8)) >> 32;
                uint64_t s_SymbolOffset = s_SymbolIndex * sizeof(Elf64_Sym);

                // Get the nids offset by looking up the symbol offset
                size_t s_NidStringOffset = 0;

                // Check to make sure that the symbol offset is within the symbol table size
                if (s_Pfi->symtabsize <= s_SymbolOffset) 
                    s_NidStringOffset = 0;
                else 
                {
                    caddr_t s_NidsOffsetAddress = s_Pfi->symtab /* *(uint64_t*)(s_UnkObj + 0x28) */ + s_SymbolOffset;
                    //const Elf64_Sym* s_NidsSymbol = (Elf64_Sym*)s_NidsOffsetAddress;
                    //s_NidsSymbol->st_info;

                    // TODO: Ask TW what this is supposed to be doing ????
                    s_NidStringOffset = (size_t)(*(uint32_t*)(s_NidsOffsetAddress));
                }

                // Make sure the string table offset is within bounds
                size_t s_StringTableSize = s_Pfi->strsize;
                if (s_StringTableSize <= s_NidStringOffset) 
                {
                    WriteLog(LL_Error, "[%s] (%p <= %p) : Error", s_TitleId, (void*)s_StringTableSize, (void*)s_NidStringOffset);
                    continue;
                }

                s_NidStringOffset += (size_t)s_StringTable;

                Elf64_Addr s_Offset = s_Current->r_offset;
                

                char* s_Nidsf = (char*)s_NidStringOffset;

                // If the nids_offset is a valid address
                if (s_Nidsf == nullptr) 
                {
                    WriteLog(LL_Error, "could not get the nids strings.");
                    continue;
                }

                // Check if it's the good nids
                if (strncmp(s_Nidsf, s_Nids, 11) == 0) 
                {
                    s_NidsOffsetFound = s_RelocBase + s_Offset;
                    break;
                }
            }

        } while (false);

        // Unlock dynlib object
        A_sx_xunlock_hard(s_DynlibBindLock);

    } while (false);

    if (s_ProcessUnlockNeeded)
        _mtx_unlock_flags(&p_Process->p_mtx, 0);

    return s_NidsOffsetFound;
}

int TrainerManager::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    if (p_Device == nullptr)
    {
        WriteLog(LL_Error, "invalid device.");
        return 0;
    }

    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return 0;
    }
    return 0;
}