#include "MountManager.hpp"
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/proc.h>
    #include <sys/mutex.h>
    #include <sys/filedesc.h>
    #include <sys/fcntl.h>
};

using namespace Mira::OrbisOS;

MountManager::MountManager() :
    m_MountCount(0),
    m_MaxMountCount(MaxMountPoints),
    m_Mounts(nullptr)
{
    // Create our new array of mount points
    m_Mounts = new MountPoint[m_MaxMountCount];

    // Validate allocation succeeded
    if (m_Mounts == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%d) mount points, MountManager disabled.", m_MaxMountCount);
        return;
    }

    // Zero all allocations
    memset(m_Mounts, 0, sizeof(MountPoint) * m_MaxMountCount);

    // Iterate and set the process/thread id to invalid
    for (uint32_t i = 0; i < m_MaxMountCount; ++i)
    {
        MountPoint* l_MountPoint = &m_Mounts[i];

        l_MountPoint->ThreadId = InvalidId;
        l_MountPoint->ProcessId = InvalidId;
    }
}

MountManager::~MountManager()
{

}

bool MountManager::OnProcessExit(struct proc* p_Process)
{
    //auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	//auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);

    // Check to see if we have initalized
    if (!IsInitialized())
    {
        WriteLog(LL_Debug, "MountManager not initialized.");
        return false;
    }
    
    // Validate process
    if (p_Process == nullptr)
    {
        WriteLog(LL_Error, "invalid process (%p) skipping unmounting.");
        return false;
    }

    // TODO: Determine if the process is locked already
    //PROC_LOCK(p_Process);
    int32_t s_ProcessId = p_Process->p_pid;
    //PROC_UNLOCK(p_Process);

    do
    {
        for (uint32_t l_MountIndex = 0; l_MountIndex < m_MaxMountCount; ++l_MountIndex)
        {
            MountPoint* s_MountPoint = &m_Mounts[l_MountIndex];
            if (s_MountPoint->ProcessId == s_ProcessId)
            {
                WriteLog(LL_Debug, "pid (%d) is closing, unmounting (%s)->(%s).", s_ProcessId, s_MountPoint->SourceDirectory, s_MountPoint->MntSandboxDirectory);

                // TODO: Unmount
                ClearMountPoint(l_MountIndex);
            }
        }
    } while (false);    

    return true;
}

bool MountManager::CreateMountInSandbox(const char* p_SourceDirectory, const char* p_FolderName, const struct thread* p_Thread)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    auto vn_fullpath = (int(*)(struct thread *td, struct vnode *vp, char **retbuf, char **freebuf))kdlsym(vn_fullpath);

    // Validate source directory
    if (p_SourceDirectory == nullptr)
    {
        WriteLog(LL_Error, "invalid source directory.");
        return false;
    }

    // Validate folder name
    if (p_FolderName == nullptr)
    {
        WriteLog(LL_Error, "invalid folder name");
        return false;
    }
    
    // Validate thread
    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return false;
    }

    // Get a handle to the mira framework
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return false;
    }

    // Get the main thread
	auto s_MiraMainThread = s_Framework->GetMainThread();
    if (s_MiraMainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get mira main thread.");
        return false;
    }

    // Check to see if the mount manager is initalized
    if (!IsInitialized())
    {
        WriteLog(LL_Error, "cannot create mount in sandbox MountManager not intialized.");
        return false;
    }

    // Get an index to see if it's free
    int32_t s_FreeIndex = FindFreeMountPointIndex();
    if (s_FreeIndex == -1)
    {
        WriteLog(LL_Error, "no available/free mount points.");
        return false;
    }

    const struct proc* s_Process = p_Thread->td_proc;
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "Thread has no process attached wtf?");
        return false;
    }

    auto s_ProcessDescriptor = s_Process->p_fd;
    if (s_ProcessDescriptor == nullptr)
    {
        WriteLog(LL_Error, "Process has no file descriptor wtf?");
        return false;
    }

    char* s_SandboxPath = nullptr;
    char* s_FreePath = nullptr;

    auto s_Result = vn_fullpath(s_MiraMainThread, s_ProcessDescriptor->fd_jdir, &s_SandboxPath, &s_FreePath);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not get the full path (%d).", s_Result);
        return false;
    }

	// s_SandboxPath = "/mnt/sandbox/NPXS20001_000"
    // Validate that we got something back
    if (s_SandboxPath == nullptr)
    {
        WriteLog(LL_Error, "could not get the sandbox path.");

        if (s_FreePath != nullptr)
        {
            WriteLog(LL_Debug, "deleting freepath.");
            delete s_FreePath;
        }
        
        return false;
    }

    WriteLog(LL_Debug, "SandboxPath: (%s).", s_SandboxPath);
	if (s_FreePath)
		WriteLog(LL_Debug, "FreePath: (%s).", s_FreePath);


    int32_t s_ThreadId = p_Thread->td_tid;
    int32_t s_ProcessId = s_Process->p_pid;

    do
    {
        // Get the mount point address
        MountPoint* s_MountPoint = &m_Mounts[s_FreeIndex];

        // Set the thread and process id
        s_MountPoint->ThreadId = s_ThreadId;
        s_MountPoint->ProcessId = s_ProcessId;

        // s_SandboxPath = "/mnt/sandbox/NPXS20001_000"
        // s_MountPoint->MntSandboxDirectory = "/mnt/sandbox/NPXS20001_000/_substitute"
		// p_FolderName = "_mira"
        auto s_Result = snprintf(s_MountPoint->MntSandboxDirectory, 
                                sizeof(s_MountPoint->MntSandboxDirectory),
                                "%s/%s",
                                s_SandboxPath,
                                p_FolderName);
        if (s_Result < 0)
        {
            ClearMountPoint(s_FreeIndex);
            break;
        }
        
        // Get the "source path"
        s_Result = snprintf(s_MountPoint->SourceDirectory,
                            sizeof(s_MountPoint->SourceDirectory),
                            "%s",
                            p_SourceDirectory);
        
        if (s_Result < 0)
        {
            ClearMountPoint(s_FreeIndex);
            break;
        }
        
        // Check to see if the real path directory actually exists
        auto s_DirectoryHandle = kopen_t(s_MountPoint->SourceDirectory, O_RDONLY | O_DIRECTORY, 0511, s_MiraMainThread);
        if (s_DirectoryHandle < 0)
        {
			WriteLog(LL_Error, "could not open directory (%s) (%d).", s_MountPoint->SourceDirectory, s_DirectoryHandle);
			break;
        }

        // Close the directory once we know it exists
        kclose_t(s_DirectoryHandle, s_MiraMainThread);

        // Mount the nullfs overlay
        if (!MountNullFs(s_MountPoint->SourceDirectory, p_SourceDirectory, 0))
        {
            WriteLog(LL_Error, "could not mount nullfs.");
            krmdir_t(s_MountPoint->MntSandboxDirectory, s_MiraMainThread);
            ClearMountPoint(s_FreeIndex);
            break;
        }

        // Update the access permissions to 0777
        s_Result = kchmod_t(s_MountPoint->MntSandboxDirectory, 0777, s_MiraMainThread);
        if (s_Result != 0)
            WriteLog(LL_Warn, "chmod failed on (%s) (%d).", s_MountPoint->MntSandboxDirectory, s_Result);
        
        // Increment our mount count
        m_MountCount++;
    } while (false);

    // If we have an allocated free path, then free it
    if (s_FreePath != nullptr)
    {
        WriteLog(LL_Debug, "deleting freepath (%p).", s_FreePath);
        delete s_FreePath;
    }

    // Return success
    return true;
}

bool MountManager::MountNullFs(const char* p_Where, const char* p_What, int p_Flags)
{
    if (p_Where == nullptr || p_What == nullptr)
    {
        WriteLog(LL_Error, "invalid where/what cannot mount nullfs.");
        return false;
    }

    auto mount_argf = (struct mntarg*(*)(struct mntarg *ma, const char *name, const char *fmt, ...))kdlsym(mount_argf);
    auto kernel_mount = (int(*)(struct mntarg	*ma, int flags))kdlsym(kernel_mount);

    struct mntarg* s_MountArgs = nullptr;

    s_MountArgs = mount_argf(s_MountArgs, "fstype", "%s", "nullfs");
    if (s_MountArgs == nullptr)
    {
        WriteLog(LL_Error, "could not set fstype.");
        return false;
    }
    s_MountArgs = mount_argf(s_MountArgs, "fspath", "%s", p_Where);
    if (s_MountArgs == nullptr)
    {
        WriteLog(LL_Error, "could not set fspath.");
        return false;
    }
    s_MountArgs = mount_argf(s_MountArgs, "target", "%s", p_What);
    if (s_MountArgs == nullptr)
    {
        WriteLog(LL_Error, "could not set target.");
        return false;
    }

    if (s_MountArgs == nullptr) {
    	WriteLog(LL_Error, "Something is wrong, mount args value is null after argument");
    	return false;
    }

    // NOTE: (From FreeBSD docs) Additionally, the kernel_mount() function always calls the free_mntarg() function.
    // If the MNT_UPDATE flag
    // is	set in mp-_mnt_flag then the file system should	update its internal
    // state from	the value of mp-_mnt_flag.  This can be	used, for instance, to
    // convert a read-only file system to	read-write.
    auto s_Ret = kernel_mount(s_MountArgs, p_Flags);
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "kernel_mount returned: (%d).", s_Ret);
        return false;
    }
    return true;
}

bool MountManager::ClearMountPoint(uint32_t p_Index)
{
    // Bounds check our index
    if (p_Index >= m_MaxMountCount)
        return false;
    
    // Get a pointer to the mount point
    MountPoint* s_MountPoint = &m_Mounts[p_Index];

    // Zero the entire mount point
    memset(s_MountPoint, 0, sizeof(*s_MountPoint));

    // Invalidate the process id and thread id
    s_MountPoint->ProcessId = InvalidId;
    s_MountPoint->ThreadId = InvalidId;

    return true;
}

int32_t MountManager::FindFreeMountPointIndex()
{
    // If we aren't initialized then fail
    if (!IsInitialized())
        return -1;
    
    // If we do not have any mounts then skip
    if (m_MountCount >= m_MaxMountCount)
        return -1;
    
    // Iterate over all of our current mounts
    for (uint32_t l_MountIndex = 0; l_MountIndex < m_MaxMountCount; ++l_MountIndex)
    {
        const MountPoint* l_MountPoint = &m_Mounts[l_MountIndex];
        if (l_MountPoint->ProcessId == -1 &&
            l_MountPoint->ThreadId == -1)
            return l_MountIndex;
    }

    // We did not find any mount index
    return -1;
}