#pragma once
#include <Boot/InitParams.hpp>
#include <mira/MiraConfig.hpp>

#include <External/subhook/subhook.h>

struct eventhandler_entry;
struct eventhandler_list;
typedef eventhandler_entry* eventhandler_tag;

extern "C"
{
    #include <sys/param.h>
    #include <sys/proc.h>
    #include <sys/sysproto.h> // ioctl_args

    void mira_entry(void* args);
};

namespace Mira
{
    namespace OrbisOS
    {
        class ThreadManager;
        class MountManager;
    }

    namespace Driver
    {
        class CtrlDriver;
    }

    namespace Plugins
    {
        class PluginManager;
    }

    namespace Trainers
    {
        class TrainerManager;
    }

    namespace Messaging
    {
        namespace Rpc
        {
            class Server;
        }

        class MessageManager;
    }

    namespace Utils
    {
        class Hook;
    }

    class Framework
    {
    private:
        /**
         * @brief State of Mira's main process
         * 
         * This is used to assist with properly suspending and resuming
         */
        enum class State
        {
            // Invalid/No state
            None = 0,

            // Set to suspend state
            Suspend,

            // Set to resume state
            Resume,

            // Set to shutdown
            Shutdown,

            COUNT
        };

        // Private instance pointer
        static Framework* m_Instance __attribute__((section(".instance.data")));

        //
        // Configuration
        //
        Mira::Boot::InitParams m_InitParams;
        MiraConfig m_Configuration;

        State m_State;

        //
        // System state events
        //
        struct eventhandler_entry* m_SuspendTag;
        struct eventhandler_entry* m_ResumeTag;

        //
        // Process system events
        //
        struct eventhandler_entry* m_ProcessExec;
        struct eventhandler_entry* m_ProcessExecEnd;
        struct eventhandler_entry* m_ProcessExit;

        //
        // Managers
        //

        // Manager for all static and dynamically loaded plugins
        Mira::Plugins::PluginManager* m_PluginManager;

        // Manager for messaging in between plugins
        Mira::Messaging::MessageManager* m_MessageManager;

        // Manager for trainers
        Mira::Trainers::TrainerManager* m_TrainerManager;

        // Manager for mount points
        Mira::OrbisOS::MountManager* m_MountManager;

        //
        // Device driver
        //
        Mira::Driver::CtrlDriver* m_CtrlDriver;

    public:
        // Fixing sony's bullshit
        struct sx m_PrintfLock;
        typedef int(*vprintf_t)(const char* fmt, void* list);
        static vprintf_t o_vprintf;

        subhook_t m_PrintfHook;

    public:
        static Framework* GetFramework() __attribute__((section(".instance.text")));

    protected:
        Framework();
        ~Framework();

        bool InstallEventHandlers();
        bool RemoveEventHandlers();

    public:
        /**
         * @brief Set the intialization parameters
         * These are usually set from MiraLoader with the addresses of where we are in memory
         * and the created main thread that is running.
         * 
         * @param p_Params Initialization parameters to copy from
         * @return true On Success
         * @return false Failure
         */
        bool SetInitParams(Mira::Boot::InitParams* p_Params);

        /**
         * @brief Get the Init Params object
         * 
         * @return Mira::Boot::InitParams* Pointer to the initialization parameters
         */
        Mira::Boot::InitParams* GetInitParams() { return &m_InitParams; }

        bool SetConfiguration(MiraConfig* p_SourceConfig);
        const MiraConfig* GetConfiguration() { return &m_Configuration; }
        

        bool Initialize();
        bool Terminate();
        void Update();

        void SetResumeFlag() { m_State = State::Resume; }
        void SetSuspendFlag() { m_State = State::Suspend; }
        void ClearFlag() { m_State = State::None; }

        Mira::Plugins::PluginManager* GetPluginManager() const { return m_PluginManager; }
        Mira::Messaging::MessageManager* GetMessageManager() const { return m_MessageManager; }
        Mira::Trainers::TrainerManager* GetTrainerManager() const { return m_TrainerManager; }
        Mira::OrbisOS::MountManager* GetMountManager() const { return m_MountManager; }
        Mira::Driver::CtrlDriver* GetDriver() const { return m_CtrlDriver; }

        struct thread* GetMainThread();
        struct thread* GetSyscoreThread();
        struct thread* GetShellcoreThread();
        struct thread* GetShellUIThread();

    private:
        static void OnMiraSuspend(void* __unused p_Reserved);
        static void OnMiraResume(void* __unused p_Reserved);
        static void OnMiraShutdown(void* __unused p_Reserved);

        static void OnMiraProcessExec(void* p_Framework, struct proc* p_Process);
        static void OnMiraProcessExecEnd(void* p_Framework, struct proc* p_Process);
        static void OnMiraProcessExit(void* p_Framework, struct proc* p_Process);
    };
}