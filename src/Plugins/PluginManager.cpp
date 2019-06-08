#include "PluginManager.hpp"

// Built in plugins
#include <Plugins/Debugger/Debugger.hpp>
#include <Plugins/FileManager/FileManager.hpp>

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
    
}

bool PluginManager::OnLoad()
{
    WriteLog(LL_Debug, "loading all plugins");

    // Initialize debugger
    m_Debugger = new Mira::Plugins::Debugger();
    if (m_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not allocate debugger.");
        return false;
    }
    if (!m_Debugger->OnLoad())
        WriteLog(LL_Error, "could not load debugger.");

    // Initialize file manager
    m_FileManager = new Mira::Plugins::FileManager();
    if (m_FileManager == nullptr)
    {
        WriteLog(LL_Error, "could not allocate file manager.");
        return false;
    }
    if (!m_FileManager->OnLoad())
        WriteLog(LL_Error, "could not load filemanager");
    
    return true;
}

bool PluginManager::OnUnload()
{
    WriteLog(LL_Debug, "unloading all plugins");
    bool s_AllUnloadSuccess = true;

    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        auto l_Plugin = m_Plugins[i];
        // Skip any blank spots
        if (l_Plugin == nullptr)
            continue;
        
        // TODO: Handle multiple unloads
        auto s_UnloadResult = l_Plugin->OnUnload();
        if (!s_UnloadResult)
            s_AllUnloadSuccess = false;

        WriteLog(LL_Info, "plugin (%s) unloaded %s", 
            l_Plugin->GetName(), s_UnloadResult ? "successfully" : "unsuccessfully");
        
        // Delete the plugin
        delete l_Plugin;
        m_Plugins[i] = nullptr;
    }

    // Clear out all entries
    m_Plugins.clear();

    // Free the default plugins
    if (m_FileManager)
    {
        if (!m_FileManager->OnUnload())
        WriteLog(LL_Error, "filemanager could not unload");
    
        // Free the file manager
        delete m_FileManager;
        m_FileManager = nullptr;
    }
    
    if (m_Debugger)
    {
        if (!m_Debugger->OnUnload())
            WriteLog(LL_Error, "debugger could not unload");
        
        // Free the debugger
        delete m_Debugger;
        m_Debugger = nullptr;
    }
    

    return true;
}

bool PluginManager::OnSuspend()
{
    // Hold our "all success" status
    bool s_AllSuccess = true;

    // Iterate through all of the plugins
    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        // Validate that this plugin still exists
        auto l_Plugin = m_Plugins[i];
        if (l_Plugin == nullptr)
            continue;
        
        // Suspend the plugin
        auto s_SuspendResult = l_Plugin->OnSuspend();
        if (!s_SuspendResult)
            s_AllSuccess = false;
        
        // Debugging status
        WriteLog(LL_Info, "plugin (%s) suspend %s", 
            l_Plugin->GetName(), 
            s_SuspendResult ? "success" : "failure");
    }

    // Suspend the built in plugins
    if (!m_Debugger->OnSuspend())
        WriteLog(LL_Error, "debugger suspend failed");

    if (!m_FileManager->OnSuspend())
        WriteLog(LL_Error, "file manager suspend failed");

    // Return final status
    return s_AllSuccess;
}

bool PluginManager::OnResume()
{
    // Hold our "all success" status
    bool s_AllSuccess = true;
    WriteLog(LL_Debug, "Yerrrrrrrr");

    // Iterate through all of the plugins
    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        // Validate that this plugin still exists
        WriteLog(LL_Error, "Yerrrrrrrr");
        auto l_Plugin = m_Plugins[i];
        if (l_Plugin == nullptr)
            continue;
        
        // Resume the plugin
        WriteLog(LL_Info, "Yerrrrrrrr");
        auto s_ResumeResult = l_Plugin->OnResume();
        if (!s_ResumeResult)
            s_AllSuccess = false;
        
        // Debugging status
        WriteLog(LL_Warn, "Yerrrrrrrr");

        WriteLog(LL_Info, "plugin (%s) resume %s", 
            l_Plugin->GetName(), 
            s_ResumeResult ? "success" : "failure");
    }

    WriteLog(LL_Debug, "ABC1");

    if (!m_Debugger->OnResume())
        WriteLog(LL_Error, "debugger resume failed");

    WriteLog(LL_Debug, "ABC2");
    
    if (!m_FileManager->OnResume())
        WriteLog(LL_Error, "file manager resume failed");

    WriteLog(LL_Debug, "ABC3");
    // Return final status
    return s_AllSuccess;
}