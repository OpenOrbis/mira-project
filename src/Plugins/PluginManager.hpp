#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

namespace Mira
{
    namespace Plugins
    {
        class Debugger;
        class FileManager;

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
            Mira::Plugins::Debugger* m_Debugger;
            Mira::Plugins::FileManager* m_FileManager;
        };
    }
}