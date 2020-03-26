#include "Substitute.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>

#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

extern "C"
{
    #include <sys/dirent.h>
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <sys/unistd.h>
    #include <sys/uio.h>
    #include <sys/proc.h>
    #include <sys/errno.h>
    #include <vm/vm.h>
    #include <vm/pmap.h>
    #include <sys/kthread.h>
    #include <sys/filedesc.h>
    #include <unistd.h>
    #include <sys/fcntl.h>
    #include <sys/mman.h>
    #include <sys/mount.h>
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
    #include <sys/syscall.h>
    #include <sys/syslimits.h>
};

using namespace Mira::Plugins;

Substitute::Substitute() :
    m_processStartHandler(nullptr)
{
}

Substitute::~Substitute()
{
}

bool Substitute::OnLoad()
{
    auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);

    WriteLog(LL_Info, "Loading Substitute ...");

    m_processStartHandler = EVENTHANDLER_REGISTER(process_exec_end, reinterpret_cast<void*>(OnProcessStart), nullptr, EVENTHANDLER_PRI_ANY);
    m_processEndHandler = EVENTHANDLER_REGISTER(process_exit, reinterpret_cast<void*>(OnProcessExit), nullptr, EVENTHANDLER_PRI_ANY);

    // Get sys_nmount address
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    struct sysent* sysents = sv->sv_table;
    void* nmount_addr = (void*)sysents[378].sy_call;
    WriteLog(LL_Info, "sys_nmount: %p", nmount_addr);


    return true;
}

bool Substitute::OnUnload()
{
    WriteLog(LL_Error, "Unloading Substitute ...");
    return true;
}

bool Substitute::OnSuspend()
{
    WriteLog(LL_Error, "Suspending Substitute ...");
    return true;
}

bool Substitute::OnResume()
{
    WriteLog(LL_Error, "Resuming Substitute ...");
    return true;
}

int Substitute::MountNullFS(char* where, char* what)
{
    return kernel_vmount(MNT_RDONLY, "fstype", "nullfs", "fspath", where, "target", what, NULL);
}

void Substitute::OnProcessStart(void *arg, struct proc *p)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // Get process information
    struct thread* s_SceShellCoreThread = NULL;
    struct proc* s_SceShellCoreProc = proc_find_by_name("SceShellCore");
    if (s_SceShellCoreProc) {
        s_SceShellCoreThread = FIRST_THREAD_IN_PROC(s_SceShellCoreProc);
    }

    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    WriteLog(LL_Info, "Exec for Titleid %s ...", s_TitleId);

    char s_SprxDirPath[255];
    snprintf(s_SprxDirPath, 255, "/data/mira/substitute/%s/", s_TitleId);

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr || s_SceShellCoreThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);
        WriteLog(LL_Error, "[%s] SceShellCore thread: %p", s_TitleId, s_SceShellCoreThread);

        return;
    }

    // Getting jailed path for the process
    struct filedesc* fd = p->p_fd;

    char* s_SandboxPath = nullptr;
    char* freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &freepath);

    if (s_SandboxPath == nullptr) {
        WriteLog(LL_Error, "[%s] could not get jailed path.", s_TitleId);
        return;
    } else {
        WriteLog(LL_Info, "[%s] Sandbox path: %s", s_TitleId, s_SandboxPath);
    }

    // Mounting substitute folder into the process
    char s_RealSprxFolderPath[PATH_MAX];
    char s_substituteFullMountPath[PATH_MAX];

    snprintf(s_substituteFullMountPath, PATH_MAX, "/data/testsubsutute");
    snprintf(s_RealSprxFolderPath, PATH_MAX, "/data/mira/substitute/%s", s_TitleId);

    // Opening substitute folder
    auto s_DirectoryHandle = kopen_t(s_SprxDirPath, 0x0000 | 0x00020000, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        WriteLog(LL_Error, "could not open directory (%s) (%d).", s_SprxDirPath, s_DirectoryHandle);
        return;
    }

    // Create folder
    int ret = kmkdir_t(s_substituteFullMountPath, 0511, s_SceShellCoreThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not create the directory for mount (%s) (%d).", s_substituteFullMountPath, ret);
        return;
    }

    // Mounting
    ret = MountNullFS(s_substituteFullMountPath, s_RealSprxFolderPath);
    if (ret > 0) {
        WriteLog(LL_Error, "could not mount folder (%s => %s) (%d).", s_RealSprxFolderPath, s_substituteFullMountPath, ret);
        return;
    } else {
        WriteLog(LL_Error, "mount folder success (%s => %s) (%d).", s_RealSprxFolderPath, s_substituteFullMountPath, ret);
    }

    /*
    // Reading folder and search for sprx ...
    uint64_t s_DentCount = 0;
    char s_Buffer[0x1000] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));
    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), s_MainThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);
            s_DentCount++;

            // Check if the sprx is legit
            if (strstr(l_Dent->d_name, ".sprx")) {
                char s_RelativeSprxPath[255];
                snprintf(s_RelativeSprxPath, 255, "/substitute/%s", l_Dent->d_name);

                // Create relative path for the load
                WriteLog(LL_Info, "[%s] Trying to load  %s ...", s_TitleId, s_RelativeSprxPath);

                // Load the SPRX
                int moduleID = 0;
                int ret = kdynlib_load_prx_t(s_RelativeSprxPath, 0, NULL, 0, NULL, &moduleID, s_ProcessThread);

                WriteLog(LL_Info, "kdynlib_load_prx_t: ret: %i (0x%08x) moduleID: %i (0x%08x)", ret, ret, moduleID, moduleID);
            }

            l_Pos += l_Dent->d_reclen;
        }
    }
    */

    // Closing substrate folder
    kclose_t(s_DirectoryHandle, s_MainThread);

    WriteLog(LL_Info, "Exec for Titleid %s : Done.", s_TitleId);

}

void Substitute::OnProcessExit(void *arg, struct proc *p) {
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    //auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // Get process information
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    WriteLog(LL_Info, "Exit for Titleid %s ...", s_TitleId);

    // Getting needed thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr || s_ProcessThread == nullptr)
    {
        WriteLog(LL_Error, "[%s] Could not get main or process thread", s_TitleId);
        WriteLog(LL_Error, "[%s] Main thread: %p", s_TitleId, s_MainThread);
        WriteLog(LL_Error, "[%s] Process thread: %p", s_TitleId, s_ProcessThread);
        return;
    }

    // Getting jailed path for the process
    struct filedesc* fd = p->p_fd;

    char* s_SandboxPath = nullptr;
    char* freepath = nullptr;
    vn_fullpath(s_MainThread, fd->fd_jdir, &s_SandboxPath, &freepath);

    if (s_SandboxPath == nullptr) {
        WriteLog(LL_Error, "[%s] could not get jailed path.", s_TitleId);
        return;
    }

    // Finding substitute folder
    char s_substituteFullMountPath[255];
    snprintf(s_substituteFullMountPath, 255, "%s/substitute", s_SandboxPath);

    auto s_DirectoryHandle = kopen_t(s_substituteFullMountPath, 0x0000 | 0x00020000, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
        return;
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    WriteLog(LL_Info, "[%s] Cleaning substitute ...", s_TitleId);

    // Unmount substitute folder if the folder exist
    int ret = kunmount_t(s_substituteFullMountPath, MNT_FORCE, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not unmount folder (%s) (%d), Trying to remove anyway ...", s_substituteFullMountPath, ret);
    }

    // Remove substitute folder
    ret = krmdir_t(s_substituteFullMountPath, s_MainThread);
    if (ret < 0) {
        WriteLog(LL_Error, "could not remove substitute folder (%s) (%d).", s_substituteFullMountPath, ret);
        return;
    }

    WriteLog(LL_Info, "[%s] Substitute have been cleaned.", s_TitleId);
}

Substitute* Substitute::GetPlugin()
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return nullptr;
    
    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
        return nullptr;
    
    auto s_SubstitutePlugin = static_cast<Substitute*>(s_PluginManager->GetSubstitute());
    if (s_SubstitutePlugin == nullptr)
        return nullptr;
    
    return s_SubstitutePlugin;
}
