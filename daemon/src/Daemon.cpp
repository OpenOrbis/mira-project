#include "Daemon.hpp"

#include <Rpc/Manager.hpp>

#include <Debugging/Debugger.hpp>
#include <Utils/Logger.hpp>

using namespace Mira;

std::shared_ptr<Daemon> Daemon::GetInstance()
{
    static std::shared_ptr<Daemon> s_Instance = std::make_shared<Daemon>();
    return s_Instance;
}

Daemon::Daemon() :
    m_Debugger(nullptr),
    m_FtpServer(nullptr),
    m_RpcManager(nullptr)
{
#if defined(PS4)
    // Initialize the networking
    sceNetInit();
#endif
}

Daemon::~Daemon()
{
    if (m_RpcManager)
        m_RpcManager.reset();
    
    if (m_Debugger)
        m_Debugger.reset();
    
    if (m_FtpServer)
        m_FtpServer.reset();
}

bool Daemon::OnLoad()
{
    // Create the message manager if it isn't already
    if (!m_RpcManager)
        m_RpcManager = std::make_shared<Rpc::Manager>();
    
    // Create the debugger if it isn't already
    if (!m_Debugger)
        m_Debugger = std::make_shared<Debugging::Debugger>();
    
    // Load the debugger
    if (!m_Debugger->OnLoad())
    {
        WriteLog(LL_Error, "could not load debugger.");
        return false;
    }

    return true;
}