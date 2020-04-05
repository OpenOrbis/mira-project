#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

struct thread;
struct prison;

namespace Mira
{
    namespace OrbisOS
    {
        /**
         * @brief I'm overall sick of having threads just randomly created everywhere
         * 
         * This should give some consistency, as well as tracking for when threads need to be killed or not
         */
        class ThreadManager : 
            public Utils::IModule
        {
        protected:
            enum { ThreadManager_MaxThreads = 10 };
            
            bool m_ThreadManagerRunning;
            bool m_IoThreadRunning;
            bool m_DebugThreadRunning;

            struct thread* m_IoThread;
            struct thread* m_DebugThread;

        public:
            typedef enum _ThreadStatus
            {
                None = 0,
                Suspended = 1,
                Running = 2,
                Completed = 3
            } ThreadStatus;

            ThreadManager();
            virtual ~ThreadManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

            bool CreateKernelThread();
            bool CreateUserThread(int32_t p_ProcessId);
            
        public:
            /**
             * @brief Helper function to check if SceSysCore is alive
             * 
             * @return true If syscore exists
             * @return false If syscore died
             */
            bool IsSyscoreAlive();

            /**
             * @brief Helper function to check if SceShellCore is alive
             * 
             * @return true If shellcore exists
             * @return false If shellcore died
             */
            bool IsShellcoreAlive();

            /**
             * @brief Get the Syscore Main Thread
             * 
             * @return struct thread* nullptr on error
             */
            struct thread* GetSyscoreMainThread();

            /**
             * @brief Get the Shellcore Main Thread object
             * 
             * @return struct thread* 
             */
            struct thread* GetShellcoreMainThread();

            /**
             * @brief Gets the main thread for Mira
             * 
             * @return struct thread* nullptr on error
             */
            struct thread* GetMiraMainThread();

            /**
             * @brief Gets the debugger thread
             * 
             * This will be set to decid privs, unjailed and unsandboxed
             * 
             * @return struct thread* nullptr on error
             */
            struct thread* GetDebuggerThread();
            
            /**
             * @brief Get the File Io Thread object
             * This is to be used by all hooks or Mira that needs file access. It will be unjailed/unsandboxed with root privs by default.
             * 
             * This way, from multiple hooks can use this
             * @return struct thread* 
             */
            struct thread* GetFileIoThread();

        private:
            static void FileIoThread(void* p_Argument);
            static void DebuggerThread(void* p_Argument);
        };
    }
}