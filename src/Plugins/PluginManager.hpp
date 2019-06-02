#pragma once
#include <Utils/Vector.hpp>
#include <Utils/IModule.hpp>

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
        };
    }
}