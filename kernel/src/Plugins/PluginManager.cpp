// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PluginManager.hpp"

// Built in plugins
#include <Plugins/Debugging/Debugger.hpp>
#include <Plugins/FakeSelf/FakeSelfManager.hpp>
#include <Plugins/FakePkg/FakePkgManager.hpp>
#include <Plugins/PrivCheck/PrivCheckPlugin.hpp>
#include <Plugins/SyscallGuard/SyscallGuardPlugin.hpp>
#include <Plugins/LogServer/LogManager.hpp>

// Utility functions
#include <Utils/Logger.hpp>
#include <Utils/Span.hpp>

extern "C"
{
    #include <sys/syslimits.h>
};

using namespace Mira::Plugins;

PluginManager::PluginManager() :
    m_Debugger(nullptr),
    m_Logger(nullptr),
    m_FakeSelfManager(nullptr),
    m_FakePkgManager(nullptr),
    m_EmuRegistry(nullptr),
    m_SyscallGuard(nullptr),
    m_PrivCheck(nullptr)
{
    m_Logger = nullptr;
}

PluginManager::~PluginManager()
{
}

bool PluginManager::OnLoad()
{
    WriteLog(LL_Debug, "loading all plugins");

    bool s_Success = true;
    do
    {
        // Initialize debugger
        m_Debugger = new Mira::Plugins::Debugger();
        if (m_Debugger == nullptr)
        {
            WriteLog(LL_Error, "could not allocate debugger.");
            s_Success = false;
            break;
        }

        // Initialize Logger
        m_Logger = new Mira::Plugins::LogManagerExtent::LogManager();
        if (m_Logger == nullptr)
        {
            WriteLog(LL_Error, "could not allocate log manager.");
            return false;
        }
        if (!m_Logger->OnLoad())
            WriteLog(LL_Error, "could not load logmanager");

        // Initialize the fself manager
        m_FakeSelfManager = new Mira::Plugins::FakeSelfManager();
        if (m_FakeSelfManager == nullptr)
        {
            WriteLog(LL_Error, "could not allocate fake self manager.");
            s_Success = false;
            break;
        }

        // Initialize the fpkg manager
        m_FakePkgManager = new Mira::Plugins::FakePkgManager();
        if (m_FakePkgManager == nullptr)
        {
            WriteLog(LL_Error, "could not allocate fake pkg manager.");
            s_Success = false;
            break;
        }

        m_PrivCheck = new Mira::Plugins::PrivCheckPlugin();
        if (m_PrivCheck == nullptr)
        {
            WriteLog(LL_Error, "could not allocate priv check plugin.");
            s_Success = false;
            break;
        }
    } while (false);

    if (m_Debugger)
    {
        if (!m_Debugger->OnLoad())
            WriteLog(LL_Error, "could not load debugger.");
    }

    // if (m_FakeSelfManager)
    // {
    //     if (!m_FakeSelfManager->OnLoad())
    //         WriteLog(LL_Error, "could not load fake self manager.");
    // }

    // if (m_FakePkgManager)
    // {
    //     if (!m_FakePkgManager->OnLoad())
    //         WriteLog(LL_Error, "could not load fake pkg manager.");
    // }

    if (m_EmuRegistry)
    {
        if (!m_EmuRegistry->OnLoad())
            WriteLog(LL_Error, "could not load emulated registry.");
    }

    // if (m_PrivCheck)
    // {
    //     if (!m_PrivCheck->OnLoad())
    //         WriteLog(LL_Error, "could not load priv check.");
    // }

    return s_Success;
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

    if (m_SyscallGuard)
    {
        WriteLog(LL_Debug, "unloading syscall guard");
        if (!m_SyscallGuard->OnUnload())
            WriteLog(LL_Error, "syscall guard could not unload");

        delete m_SyscallGuard;
        m_SyscallGuard = nullptr;
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
    // NOTE: Don't unload before the debugger for catch error if something wrong
    if (m_Debugger)
    {
        WriteLog(LL_Debug, "unloading debugger");
        if (!m_Debugger->OnUnload())
            WriteLog(LL_Error, "debugger could not unload");

        // Free the debugger
        delete m_Debugger;
        m_Debugger = nullptr;
    }

    if (m_PrivCheck)
    {
        WriteLog(LL_Error, "unloading priv check.");
        if (!m_PrivCheck->OnUnload())
            WriteLog(LL_Error, "could not unload priv check.");
        
        delete m_PrivCheck;
        m_PrivCheck = nullptr;
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
    if (m_FakeSelfManager)
    {
        if (!m_FakeSelfManager->OnSuspend())
            WriteLog(LL_Error, "fake self manager suspend failed");
    }

    if (m_EmuRegistry)
    {
        if (!m_EmuRegistry->OnSuspend())
            WriteLog(LL_Error, "emuRegistry suspend failed");
    }

    // Suspend both of the loggers (cleans up the sockets)
    if (m_Logger)
    {
        if (!m_Logger->OnSuspend())
            WriteLog(LL_Error, "log manager suspend failed");
    }

    // Nota: Don't suspend before the debugger for catch error if something when wrong
    if (m_Debugger)
    {
        if (!m_Debugger->OnSuspend())
            WriteLog(LL_Error, "debugger suspend failed");
    }

    // Return final status
    return s_AllSuccess;
}

bool PluginManager::OnResume()
{
    // Hold our "all success" status
    bool s_AllSuccess = true;

    WriteLog(LL_Debug, "Resuming all plugins");

    WriteLog(LL_Debug, "resuming debugger");
    if (m_Debugger)
    {
        if (!m_Debugger->OnResume())
            WriteLog(LL_Error, "debugger resume failed");
    }

    // Resume both of the loggers (up the sockets)
    WriteLog(LL_Debug, "resuming log manager");
    if (m_Logger)
    {
        if (!m_Logger->OnResume())
            WriteLog(LL_Error, "log manager resume failed");
    }

    WriteLog(LL_Debug, "resuming emuRegistry");
    if (m_EmuRegistry)
    {
        if (!m_EmuRegistry->OnResume())
            WriteLog(LL_Error, "emuRegistry resume failed");
    }

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


    WriteLog(LL_Debug, "resuming fake self manager");
    if (m_FakeSelfManager)
    {
        if (!m_FakeSelfManager->OnResume())
            WriteLog(LL_Error, "fake self manager resume failed");
    }

    WriteLog(LL_Debug, "resume all %s", s_AllSuccess ? "successfully" : "un-successfully");

    // Return final status
    return s_AllSuccess;
}

bool PluginManager::OnProcessExec(struct proc* p_Process)
{
    if (p_Process == nullptr)
        return false;

    if (m_Debugger)
    {
        if (!m_Debugger->OnProcessExec(p_Process))
            WriteLog(LL_Error, "debugger process exec failed.");
    }

    return true;
}

bool PluginManager::OnProcessExecEnd(struct proc* p_Process)
{
    if (p_Process == nullptr)
        return false;

    return true;
}

bool PluginManager::OnProcessExit(struct proc* p_Process)
{
    if (p_Process == nullptr)
        return false;
    
    if (m_Debugger)
    {
        if (!m_Debugger->OnProcessExit(p_Process))
            WriteLog(LL_Error, "debugger process exit failed.");
    }

    return true;
}