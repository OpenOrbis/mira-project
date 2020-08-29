#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

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
        private:
            static const char* c_ShmPrefix;

        public:
            TrainerManager();
            virtual ~TrainerManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

            virtual bool OnProcessExit(struct proc* p_Process) override;
            virtual bool OnProcessExecEnd(struct proc* p_Process) override;

        protected:
            // TODO: Get HDD Trainers Folder
            // TODO: Get USB Trainers Folder
            // TODO: Get Networked Trainers Folder
            
            // Generate a new randomized alphanumeric Shm Id (p_OutputStringLength must be > 5) strlen("_shm_")
            bool GenerateShmId(char* p_OutputString, uint32_t p_OutputStringLength);
            // TODO: Create Shm
            // TODO: Delete Shm
            // TODO: Get Shm
        };
    }
}