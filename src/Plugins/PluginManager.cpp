#include "PluginManager.hpp"

// Built in plugins
#include <Plugins/Debugger/Debugger2.hpp>
#include <Plugins/LogServer/LogManager.hpp>
#include <Plugins/FileManager/FileManager.hpp>
#include <Plugins/FakeSelf/FakeSelfManager.hpp>
#include <Plugins/FakePkg/FakePkgManager.hpp>
#include <Plugins/EmuRegistry/EmuRegistryPlugin.hpp>
#include <Plugins/Substitute/Substitute.hpp>

// Utility functions
#include <Utils/Logger.hpp>
#include <Utils/Span.hpp>

using namespace Mira::Plugins;

PluginManager::PluginManager() :
	m_Logger(nullptr),
    m_Debugger(nullptr),
    m_FileManager(nullptr),
    m_FakeSelfManager(nullptr),
    m_FakePkgManager(nullptr),
    m_EmuRegistry(nullptr),
    m_Substitute(nullptr)
{
    // Hushes error: private field 'm_FileManager' is not used [-Werror,-Wunused-private-field]
	m_Logger = nullptr;
    m_FileManager = nullptr;
}

PluginManager::~PluginManager()
{
    
}

bool PluginManager::OnLoad()
{
    WriteLog(LL_Debug, "loading all plugins");
    
    // Initialize debugger
    m_Debugger = new Mira::Plugins::Debugger2();
    if (m_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not allocate debugger.");
        return false;
    }
    if (!m_Debugger->OnLoad())
        WriteLog(LL_Error, "could not load debugger.");

    // Initialize Logger
    m_Logger = new Mira::Plugins::LogManagerExtent::LogManager();
    if (m_Logger == nullptr)
    {
        WriteLog(LL_Error, "could not allocate log manager.");
        return false;
    }
    if (!m_Logger->OnLoad())
        WriteLog(LL_Error, "could not load logmanager");

    // Initialize file manager
    m_FileManager = new Mira::Plugins::FileManagerExtent::FileManager();
    if (m_FileManager == nullptr)
    {
        WriteLog(LL_Error, "could not allocate file manager.");
        return false;
    }
    if (!m_FileManager->OnLoad())
        WriteLog(LL_Error, "could not load filemanager");

    // Initialize the fself manager
    m_FakeSelfManager = new Mira::Plugins::FakeSelfManager();
    if (m_FakeSelfManager == nullptr)
    {
        WriteLog(LL_Error, "could not allocate fake self manager.");
        return false;
    }
    if (!m_FakeSelfManager->OnLoad())
        WriteLog(LL_Error, "could not load fake self manager.");
    
    // Initialize the fpkg manager
    m_FakePkgManager = new Mira::Plugins::FakePkgManager();
    if (m_FakePkgManager == nullptr)
    {
        WriteLog(LL_Error, "could not allocate fake pkg manager.");
        return false;
    }
    if (!m_FakePkgManager->OnLoad())
        WriteLog(LL_Error, "could not load fake pkg manager.");
    
    // Initialize emu-registry
    m_EmuRegistry = new Mira::Plugins::EmuRegistryPlugin();
    if (m_EmuRegistry == nullptr)
    {
        WriteLog(LL_Error, "could not allocate emulated registry.");
        return false;
    }
    if (!m_EmuRegistry->OnLoad())
        WriteLog(LL_Error, "could not load emulated registry.");

    // Initialize Substitute
    m_Substitute = new Mira::Plugins::Substitute();
    if (m_Substitute == nullptr)
    {
        WriteLog(LL_Error, "could not allocate substitute.");
        return false;
    }
    if (!m_Substitute->OnLoad())
        WriteLog(LL_Error, "could not load substitute.");

    return true;
}

bool PluginManager::OnUnload()
{
    WriteLog(LL_Debug, "unloading all plugins");
    bool s_AllUnloadSuccess = true;

    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        WriteLog(LL_Debug, "unloading plugin idx: (%d).", i);
        auto l_Plugin = m_Plugins[i];

        // Skip any blank spots
        if (l_Plugin == nullptr)
            continue;
        
        WriteLog(LL_Debug, "unloading plugin: (%s).", l_Plugin->GetName());
        // TODO: Handle multiple unloads
        auto s_UnloadResult = l_Plugin->OnUnload();
        if (!s_UnloadResult)
            s_AllUnloadSuccess = false;

        WriteLog(LL_Info, "plugin (%s) unloaded %s", 
            l_Plugin->GetName(), s_UnloadResult ? "successfully" : "unsuccessfully");
        
        WriteLog(LL_Debug, "freeing plugin");
        // Delete the plugin
        delete l_Plugin;

        WriteLog(LL_Debug, "clearing entry");
        m_Plugins[i] = nullptr;
    }

    WriteLog(LL_Debug, "clearing array");
    // Clear out all entries
    m_Plugins.clear();

    // Free the default plugins
    // Delete the file manager
    if (m_FileManager)
    {
        WriteLog(LL_Debug, "unloading file manager");

        if (!m_FileManager->OnUnload())
            WriteLog(LL_Error, "filemanager could not unload");

        // Free the file manager
        delete m_FileManager;
        m_FileManager = nullptr;
    }	

    // Delete the fake self manager
    if (m_FakeSelfManager)
    {
        WriteLog(LL_Debug, "unloading fake self manager");
        if (!m_FakeSelfManager->OnUnload())
            WriteLog(LL_Error, "fake self manager could not unload");
        
        delete m_FakeSelfManager;
        m_FakeSelfManager = nullptr;
    }

    // Delete the fake pkg manager
    if (m_FakePkgManager)
    {
        WriteLog(LL_Debug, "unloading fake pkg manager");
        if (!m_FakePkgManager->OnUnload())
            WriteLog(LL_Error, "fake pkg manager could not unload");
        
        delete m_FakePkgManager;
        m_FakePkgManager = nullptr;
    }

    // Delete the emulated registry
    if (m_EmuRegistry)
    {
        WriteLog(LL_Debug, "unloading emulated registry");
        if (!m_EmuRegistry->OnUnload())
            WriteLog(LL_Error, "emuRegistry could not unload");
        
        delete m_EmuRegistry;
        m_EmuRegistry = nullptr;
    }

    // Delete the log server
    if (m_Logger)
    {
        WriteLog(LL_Debug, "unloading log manager");

        if (!m_Logger->OnUnload())
            WriteLog(LL_Error, "logmanager could not unload");

        // Free the file manager
        delete m_Logger;
        m_Logger = nullptr;
    }

    // Delete the debugger
    if (m_Debugger)
    {
        WriteLog(LL_Debug, "unloading debugger");
        if (!m_Debugger->OnUnload())
            WriteLog(LL_Error, "debugger could not unload");

        // Free the debugger
        delete m_Debugger;
        m_Debugger = nullptr;
    }

    // Delete Substitute
    if (m_Substitute)
    {
        WriteLog(LL_Debug, "unloading substitute");
        if (!m_Substitute->OnUnload())
            WriteLog(LL_Error, "substitute could not unload");

        // Free Substitute
        delete m_Substitute;
        m_Substitute = nullptr;
    }
    
    WriteLog(LL_Debug, "All Plugins Unloaded %s.", s_AllUnloadSuccess ? "successfully" : "un-successfully");
    return s_AllUnloadSuccess;
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
    if (!m_FileManager->OnSuspend())
        WriteLog(LL_Error, "file manager suspend failed");
	
    if (!m_FakeSelfManager->OnSuspend())
        WriteLog(LL_Error, "fake self manager suspend failed");
    
    if (!m_EmuRegistry->OnSuspend())
        WriteLog(LL_Error, "emuRegistry suspend failed");

    if (!m_Logger->OnSuspend())
        WriteLog(LL_Error, "log manager suspend failed");
    
    if (!m_Debugger->OnSuspend())
        WriteLog(LL_Error, "debugger suspend failed");
    
    if (!m_Substitute->OnSuspend())
        WriteLog(LL_Error, "substitute suspend failed");

    // Return final status
    return s_AllSuccess;
}

bool PluginManager::OnResume()
{
    // Hold our "all success" status
    bool s_AllSuccess = true;
    WriteLog(LL_Debug, "Resuming all plugins");

    WriteLog(LL_Debug, "resuming debugger");
    if (!m_Debugger->OnResume())
        WriteLog(LL_Error, "debugger resume failed");

    WriteLog(LL_Debug, "resuming log manager");
    if (!m_Logger->OnResume())
        WriteLog(LL_Error, "log manager resume failed"); 
    
    WriteLog(LL_Debug, "resuming emuRegistry");
    if (!m_EmuRegistry->OnResume())
        WriteLog(LL_Error, "emuRegistry resume failed");

    WriteLog(LL_Debug, "resuming substitute");
    if (!m_Substitute->OnResume())
        WriteLog(LL_Error, "substitute resume failed");

    // Iterate through all of the plugins
    for (auto i = 0; i < m_Plugins.size(); ++i)
    {
        // Validate that this plugin still exists
        WriteLog(LL_Error, "Resuming plugin idx: (%d).", i);
        auto l_Plugin = m_Plugins[i];
        if (l_Plugin == nullptr)
            continue;
        
        // Resume the plugin
        WriteLog(LL_Info, "resuming plugin (%s).", l_Plugin->GetName());
        auto s_ResumeResult = l_Plugin->OnResume();
        if (!s_ResumeResult)
            s_AllSuccess = false;
        
        // Debugging status
        WriteLog(LL_Info, "plugin (%s) resume %s", 
            l_Plugin->GetName(), 
            s_ResumeResult ? "success" : "failure");
    }

    WriteLog(LL_Debug, "resuming file manager");
    if (!m_FileManager->OnResume())
        WriteLog(LL_Error, "file manager resume failed");   
 
    WriteLog(LL_Debug, "resuming fake self manager");
    if (!m_FakeSelfManager->OnResume())
        WriteLog(LL_Error, "fake self manager resume failed");
    
    WriteLog(LL_Debug, "resume all %s", s_AllSuccess ? "successfully" : "un-successfully");
    // Return final status
    return s_AllSuccess;
}