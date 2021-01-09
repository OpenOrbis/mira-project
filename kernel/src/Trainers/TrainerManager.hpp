#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

#include "MiraShm.hpp"

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
    #include <sys/imgact.h>
}

/*
    TrainerManager rundown

    So there are 3 components to trainers to take advantage of Seremo's Mono work

    A. The Trainer itself
    B. Trainer Manager
    C. ShellUI

    The way that this system works is, we inject a new module into SceShellUI which will at 0.5 second intervals call an ioctl (in Mira kernel driver)
    to get a list of all currently allocated Shm's (or write to the shm once validated that it still exists)

    Once the module in SceShellUI gets that list, it compares against a locally stored list of already created menu's/trainers
    If there is something missing, remove UI + Shm from that list
    If there is something new, create UI + store Shm in that list
    If things are identical, do nothing

    This leads into the Trainer itself next. Which instead of having the kernel write some hackish way in each of the modules, that the module itself
    will call an ioctl to "create shm" on it's behalf. The Mira kernel driver will auto-generate a Shm name and call shm open with the provided
    thread and size of the header (maximum of 1MB) and return the name of the shm

    The trainer will then mmap the shm to it's local trainer header/config block as RO. (SceShellUI should have found/opened for RW access)
    The trainer will next install hooks or spawn another thread in order to keep running

    In the trainers thread/hook, it will read the Shm/trainer header/config block and based on the options there do different things

    This means that SceShellUI can poke values into the Shm, which then the trainer will read.

    OnProcessExit the shm is closed on the trainer's behalf
*/
namespace Mira
{
    namespace Trainers
    {
        class TrainerManager : public Utils::IModule
        {
        public:
            enum
            {
                MaxPrxPath = 256,
                MaxShmIdLength = 10,
                MaxShms = 16
            };

            typedef struct TrainerShm_t
            {
                char Id[MaxShmIdLength];
            } TrainerShm;

            typedef struct TrainerBoot_t
            {
                uint64_t OrigEntry;

                // This is the folder that we will search for .prx files to load
                char TrainerFolder[MaxPrxPath];
            } TrainerBoot;

        private:
            // Total amount of proxied shm's
            TrainerShm Shms[MaxShms];

            // Sleep lock
            struct mtx m_Mutex;

            static const char* c_ShmPrefix; // _shm_
            typedef int(*sv_fixup_t)(register_t **, struct image_params *);
            static sv_fixup_t g_sv_fixup;

        public:
            TrainerManager();
            virtual ~TrainerManager();

            // Load the module
            virtual bool OnLoad() override;

            // Unload the module
            virtual bool OnUnload() override;

            // Handle event for process exit
            virtual bool OnProcessExit(struct proc* p_Process) override;

            bool LoadTrainers(struct proc* p_TargetProcess);

        protected:
            // Get HDD Trainers Folder (ex: /user/mira/trainers )
            bool GetHddTrainerPath(char*& p_OutputString, uint32_t& p_OutputStringLength);

            // Get USB Trainers Folder (ex: /dev/usb/usb0/mira/trainers )
            bool GetUsbTrainerPath(char*& p_OutputString, uint32_t& p_OutputStringLength);
            
            // Generate a new randomized alphanumeric Shm Id (p_OutputStringLength must be > 5) strlen("_shm_")
            bool GenerateShmId(char* p_OutputString, uint32_t p_OutputStringLength);

            // Create a shm
            bool CreateShm(char*& p_OutId);
            
            // Delete a shm and unlink it
            bool DeleteShm(const char* p_Id);

            // Gets if a Shm exists with this id
            bool GetShm(const char* p_Id);

            // Inject a new thread per prx
            static bool ThreadInjection(const char* p_TrainerPrxPath, struct proc* p_Proc, struct image_params* p_Params);

            // Load via payload
            bool PayloadInjection();

            // Checks if a file exists
            static bool FileExists(const char* p_Path);

            // Checks if a directory exists
            bool DirectoryExists(const char* p_Path);

            static uint8_t* InjectTrainerLoader(struct proc* p_TargetProcess);

            static uint8_t* AllocateProcessMemory(struct proc* p_Process, uint32_t p_Size);

        private:
            static void OnSomethingOrAnother();

            static int OnSvFixup(register_t **, struct image_params *);
        };
    }
}