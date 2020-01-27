#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

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
            Mira::Utils::IModule* m_Debugger;
            Mira::Utils::IModule* m_FileManager;
            Mira::Utils::IModule* m_FakeSelfManager;
            Mira::Utils::IModule* m_FakePkgManager;

        public:
            Mira::Utils::IModule* GetDebugger() { return m_Debugger; }
            Mira::Utils::IModule* GetFakeSelfManager() { return m_FakeSelfManager; }
        };
    }
}