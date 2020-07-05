#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/mutex.h>
}

namespace Mira
{
    namespace Plugins
    {
        class PluginManager : public Mira::Utils::IModule
        {
        private:
            Vector<Mira::Utils::IModule*> m_Plugins;

        public:
            PluginManager();
            virtual ~PluginManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;
        private:
            Mira::Utils::IModule* m_Logger;
            Mira::Utils::IModule* m_LoggerConsole;
            Mira::Utils::IModule* m_Debugger;
            Mira::Utils::IModule* m_FileManager;
            Mira::Utils::IModule* m_FakeSelfManager;
            Mira::Utils::IModule* m_FakePkgManager;
            Mira::Utils::IModule* m_EmuRegistry;
            Mira::Utils::IModule* m_Substitute;
            Mira::Utils::IModule* m_BrowserActivator;
            Mira::Utils::IModule* m_MorpheusEnabler;
            Mira::Utils::IModule* m_SyscallGuard;

        public:
            Mira::Utils::IModule* GetDebugger() { return m_Debugger; }
            Mira::Utils::IModule* GetFakeSelfManager() { return m_FakeSelfManager; }
            Mira::Utils::IModule* GetEmulatedRegistry() { return m_EmuRegistry; }
            Mira::Utils::IModule* GetSubstitute() { return m_Substitute; }
            Mira::Utils::IModule* GetBrowserActivator() { return m_BrowserActivator; }
            Mira::Utils::IModule* GetMorpheusEnabler() { return m_MorpheusEnabler; }
            Mira::Utils::IModule* GetSyscallGuard() { return m_SyscallGuard; }
        };
    }
}
