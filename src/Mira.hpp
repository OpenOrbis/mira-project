#pragma once
#include <Boot/InitParams.hpp>

struct eventhandler_entry;
struct eventhandler_list;
typedef eventhandler_entry* eventhandler_tag;

namespace Mira
{
    namespace Plugins
    {
        class PluginManager;
    }

    class Framework
    {
    private:
        static Framework* m_Instance;
        Mira::Boot::InitParams m_InitParams;

        bool m_EventHandlersInstalled;
        struct eventhandler_entry* m_SuspendTag;
        struct eventhandler_entry* m_ResumeTag;

        Mira::Plugins::PluginManager* m_PluginManager;

    public:
        static Framework* GetFramework();

    protected:
        Framework();
        ~Framework();

        bool InstallEventHandlers();
        bool RemoveEventHandlers();

    public:
        bool SetInitParams(Mira::Boot::InitParams& p_Params);
        Mira::Boot::InitParams* GetInitParams() { return &m_InitParams; }

        bool Initialize();
        bool Terminate();

    private:
        static void OnMiraSuspend(Mira::Framework* p_Framework);
        static void OnMiraResume(Mira::Framework* p_Framework);
    };
}