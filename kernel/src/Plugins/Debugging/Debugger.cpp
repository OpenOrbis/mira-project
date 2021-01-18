// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Debugger.hpp"
#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;


Debugger::Debugger()
{

}

Debugger::~Debugger()
{

}

bool Debugger::OnLoad()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
	// Create the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = new Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(OnTrapFatal));
    
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        m_TrapFatalHook->Enable();
    }
#endif
    return true;
}

bool Debugger::OnUnload()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
    WriteLog(LL_Info, "deleting trap fatal hook");
	if (m_TrapFatalHook != nullptr)
    {
        if (m_TrapFatalHook->IsEnabled())
            m_TrapFatalHook->Disable();
        
        delete m_TrapFatalHook;
        m_TrapFatalHook = nullptr;
    }
#endif

    return true;
}