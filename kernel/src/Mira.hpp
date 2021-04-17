#pragma once
#include <Boot/InitParams.hpp>
#include <mira/MiraConfig.hpp>

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

extern "C" 

namespace Mira
{
    namespace OrbisOS
    {
        class ThreadManager;
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
        enum class State
        {
            None = 0,
            Suspend,
            Resume,
            Shutdown,
        };

        static Framework* m_Instance;

        // Configuration
        Mira::Boot::InitParams m_InitParams;
        MiraConfig m_Configuration;

        State m_State;

        // System state events
        struct eventhandler_entry* m_SuspendTag;
        struct eventhandler_entry* m_ResumeTag;

        // Process system events
        struct eventhandler_entry* m_ProcessExec;
        struct eventhandler_entry* m_ProcessExecEnd;
        struct eventhandler_entry* m_ProcessExit;

        // Managers
        Mira::Plugins::PluginManager* m_PluginManager;
        Mira::Messaging::MessageManager* m_MessageManager;
        Mira::Trainers::TrainerManager* m_TrainerManager;

        // Device driver
        Mira::Driver::CtrlDriver* m_CtrlDriver;

    public:
        // Fixing sony's bullshit
        struct sx m_PrintfLock;
        Utils::Hook* m_PrintfHook;

    public:
        static Framework* GetFramework();

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

        Mira::Plugins::PluginManager* GetPluginManager() { return m_PluginManager; }
        Mira::Messaging::MessageManager* GetMessageManager() { return m_MessageManager; }
        Mira::Trainers::TrainerManager* GetTrainerManager() { return m_TrainerManager; }

        Mira::Driver::CtrlDriver* GetDriver() { return m_CtrlDriver; }

        struct thread* GetMainThread();
        struct thread* GetSyscoreThread();
        struct thread* GetShellcoreThread();

    private:
        static void OnMiraSuspend(void* __unused p_Reserved);
        static void OnMiraResume(void* __unused p_Reserved);
        static void OnMiraShutdown(void* __unused p_Reserved);

        static void OnMiraProcessExec(void* p_Framework, struct proc* p_Process);
        static void OnMiraProcessExecEnd(void* p_Framework, struct proc* p_Process);
        static void OnMiraProcessExit(void* p_Framework, struct proc* p_Process);
    };
}