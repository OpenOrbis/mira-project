// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Debugger2.hpp"
#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;

const char Debugger2::hexchars[] = "0123456789abcdef";

Debugger2::Debugger2(uint16_t p_Port) :
    m_TrapFatalHook(nullptr),
    m_ServerAddress { 0 },
    m_Socket(-1),
    m_Port(p_Port),
    m_OnProcessExitTag(nullptr),
    m_AttachedPid(-1)
{
    // auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    //auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);

    // mtx_init(&m_Mutex, "DbgLock", nullptr, MTX_SPIN);

    // Registers process exiting
    //m_OnProcessExitTag = EVENTHANDLER_REGISTER(process_exit, reinterpret_cast<void*>(OnProcessExit), this, EVENTHANDLER_PRI_FIRST);
}

Debugger2::~Debugger2()
{
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
	auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    if (m_OnProcessExitTag != nullptr)
    {
        EVENTHANDLER_DEREGISTER(process_exit, m_OnProcessExitTag);
        m_OnProcessExitTag = nullptr;
    }
}

bool Debugger2::OnLoad()
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

bool Debugger2::OnUnload()
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

bool Debugger2::OnSuspend()
{
    return true;
}

bool Debugger2::OnResume()
{
    return true;
}