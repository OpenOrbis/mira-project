#pragma once
#include <Boot/InitParams.hpp>
#include <OrbisOS/ThreadManager.hpp>

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
        static Framework* m_Instance;
        Mira::Boot::InitParams m_InitParams;

        bool m_EventHandlersInstalled;
        struct eventhandler_entry* m_SuspendTag;
        struct eventhandler_entry* m_ResumeTag;
        struct eventhandler_entry* m_ShutdownTag;

        Mira::OrbisOS::ThreadManager* m_ThreadManager;
        Mira::Plugins::PluginManager* m_PluginManager;
        Mira::Messaging::MessageManager* m_MessageManager;
        Mira::Messaging::Rpc::Server* m_RpcServer;

        Mira::Driver::CtrlDriver* m_CtrlDriver;

    public:
        static Framework* GetFramework();

    protected:
        Framework();
        ~Framework();

        bool InstallEventHandlers();
        bool RemoveEventHandlers();

    public:
        bool SetInitParams(Mira::Boot::InitParams* p_Params);
        Mira::Boot::InitParams* GetInitParams() { return &m_InitParams; }

        bool Initialize();
        bool Terminate();

        Mira::OrbisOS::ThreadManager* GetThreadManager() { return m_ThreadManager; }
        Mira::Plugins::PluginManager* GetPluginManager() { return m_PluginManager; }
        Mira::Messaging::MessageManager* GetMessageManager() { return m_MessageManager; }
        Mira::Messaging::Rpc::Server* GetRpcServer() { return m_RpcServer; }

        struct thread* GetMainThread();
        
    private:
        static void OnMiraSuspend(void* __unused p_Reserved);
        static void OnMiraResume(void* __unused p_Reserved);
        static void OnMiraShutdown(void* __unused p_Reserved);

        static void OnSceSblSysVeri(void* __unused p_Reserved);

        // Credits: LM (need to move this to a better location, but stopgap fix for now.)
        static int OnIoctl(struct thread* p_Thread, struct ioctl_args* p_Uap);
        static int OnWorkaround8849(struct thread* p_Thread, uint32_t* p_Uap);

    };
}