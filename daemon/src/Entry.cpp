#include "Daemon.hpp"

#include <memory>
#include <cstdio>
#include <thread>

#include <Utils/Logger.hpp>

#if defined(PS4)
#include <orbis/libkernel.h>
#endif

std::thread g_LaunchThread;

extern "C" void trainer_load()
{
    g_LaunchThread = std::thread([]()
    {
        WriteLog(LL_Debug, "MiraDaemon Entry Point.");

        // Create a new instance of our daemon
        auto s_Daemon = Mira::Daemon::GetInstance();
        if (s_Daemon == nullptr)
        {
            WriteLog(LL_Error, "could not create daemon.");
            return -1;
        }

        // Try and load the daemon
        if (!s_Daemon->OnLoad())
        {
            WriteLog(LL_Error, "could not load daemon.");
            return -2;
        }
        
        for (;;)
            __asm__("nop");
    });

    g_LaunchThread.detach();
}


// Entry point
int daemon_entry(void)
{
    WriteLog(LL_Debug, "MiraDaemon Entry Point.");

    // Create a new instance of our daemon
    auto s_Daemon = Mira::Daemon::GetInstance();
    if (s_Daemon == nullptr)
    {
        WriteLog(LL_Error, "could not create daemon.");
        return -1;
    }

    // Try and load the daemon
    if (!s_Daemon->OnLoad())
    {
        WriteLog(LL_Error, "could not load daemon.");
        return -2;
    }
    
    for (;;)
        __asm__("nop");
    
    return 0;
}