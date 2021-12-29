#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

struct proc;
struct thread;

namespace Mira
{
    namespace OrbisOS
    {
        class MountManager : public Mira::Utils::IModule
        {
        public:
            // MountManager defaults
            enum
            {
                MaxPathLength = 260,
                MaxTitleIdLength = 16,
                MaxMountPoints = 4,

                InvalidId = -1,

                //MountPointSize = sizeof(MountPoint)
            };

            typedef struct _MountPoint
            {
                // Thread id that created this mount point
                int32_t ThreadId;

                // Process id that created this mount point
                int32_t ProcessId;

                // Mount point inside of the sandbox (ex: _substitute)
                char FolderName[MaxPathLength];

                // Mounted rootfs sandbox directory (ex: /mnt/sandbox/NPXS22010_000/<SandboxRootPath>)
                char MntSandboxDirectory[MaxPathLength];

                // Mounted rootfs source directory (ex: /mnt/usb0/_mira/trainers/<titleid>)
                char SourceDirectory[MaxPathLength];

                // Copy of the title id (if there is one)
                char TitleId[MaxTitleIdLength];
            } MountPoint;
            
        private:
            uint32_t m_MountCount;
            uint32_t m_MaxMountCount;
            MountPoint* m_Mounts;

            bool IsInitialized() { return m_Mounts != nullptr && m_MaxMountCount != 0; }
            
            bool MountNullFs(const char* p_Where, const char* p_What, int p_Flags);

            // TODO: Implement
            //bool UnmountNullFs();

            bool ClearMountPoint(uint32_t p_Index);
            int32_t GetMountPointByProcessId(int32_t p_ProcessId);
            int32_t GetMountPointByThreadId(int32_t p_ThreadId);
        public:
            MountManager();
            virtual ~MountManager();

            virtual bool OnProcessExit(struct proc* p_Process) override;

            //bool CreateMount(const char* p_SourceDirectory);

            /**
             * @brief Create a new mount from a source directory into the process sandbox
             * 
             * @param p_SourceDirectory Root source directory (ex: /mnt/usb0/mira)
             * @param p_MountedName Name inside sandbox to mount to (ex: _mira)
             * @param p_Thread Process calling thread
             * @return true on success, false otherwise
             */
            bool CreateMountInSandbox(const char* p_SourceDirectory, const char* p_MountedName, const struct thread* p_Thread);
            bool DestroyMount();

            void DestroyAllMounts();

            /**
             * @brief Finds a free mount point index
             * 
             * @return int32_t -1 on failure, success otherwise
             */
            int32_t FindFreeMountPointIndex();
        };
    }
}