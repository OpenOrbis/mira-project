#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

namespace Mira
{
    namespace Plugins
    {
        class Debugger;
        class FileManager;

        class PluginManager
        {
        private:
            Vector<Mira::Utils::IModule*> m_Plugins;

        public:
            PluginManager();
            ~PluginManager();

            bool InstallDefaultPlugins();

        private:
            Mira::Plugins::Debugger* m_Debugger;
            Mira::Plugins::FileManager* m_FileManager;
        };
    }
}