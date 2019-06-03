#include "PluginManager.hpp"

#include <Utils/Logger.hpp>
#include <Utils/Span.hpp>

using namespace Mira::Plugins;

PluginManager::PluginManager()
{
    WriteLog(LL_Debug, "creating span of 32");
    Span<uint8_t> span(32);

    for (auto i = 0; i < 10; ++i)
    {
        auto s_Address = span.get_struct<uint32_t>();
        WriteLog(LL_Debug, "span: %p", s_Address);
    }

    WriteLog(LL_Debug, "span done");
}

PluginManager::~PluginManager()
{
    WriteLog(LL_Info, "pluginmanager destroy: unloading all plugins");
    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        auto l_Plugin = m_Plugins[i];
        // Skip any blank spots
        if (l_Plugin == nullptr)
            continue;
        
        // TODO: Handle multiple unloads
        WriteLog(LL_Info, "plugin %S unloaded %s", l_Plugin->GetName(), l_Plugin->OnUnload() ? "successfully" : "unsuccessfully");
        
        // Delete the plugin
        delete l_Plugin;
        m_Plugins[i] = nullptr;
    }

    // Clear out all entries
    m_Plugins.clear();
}

bool PluginManager::InstallDefaultPlugins()
{
    WriteLog(LL_Debug, "installing default plugins");

    return true;
}