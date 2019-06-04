#include "PluginManager.hpp"

// Built in plugins
#include <Plugins/Debugger/Debugger.hpp>

// Utility functions
#include <Utils/Logger.hpp>
#include <Utils/Span.hpp>

using namespace Mira::Plugins;

PluginManager::PluginManager() :
    m_Debugger(nullptr),
    m_FileManager(nullptr)
{
    // Hushes error: private field 'm_FileManager' is not used [-Werror,-Wunused-private-field]
    m_FileManager = nullptr;
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

    m_Debugger = new Mira::Plugins::Debugger();
    if (m_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not allocate debugger.");
        return false;
    }
    
    return true;
}