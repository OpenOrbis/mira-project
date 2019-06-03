#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

namespace Mira
{
    namespace Plugins
    {
        class PluginManager
        {
        private:
            Vector<Mira::Utils::IModule*> m_Plugins;

        public:
            PluginManager();
            ~PluginManager();

            bool InstallDefaultPlugins();

        private:

        };
    }
}