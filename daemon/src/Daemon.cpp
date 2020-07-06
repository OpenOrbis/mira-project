#include "Daemon.hpp"
#include <mutex>

using namespace Mira;

Daemon::Daemon()
{

}

Daemon::~Daemon()
{
    if (m_RpcServer)
        m_RpcServer.reset();
    
    if (m_Debugger)
        m_Debugger.reset();
    
    if (m_FtpServer)
        m_FtpServer.reset();
}

bool Daemon::OnLoad()
{
    if (m_Debugger)
    {
        if (!m_Debugger->OnLoad())
        {
            fprintf(stderr, "err: could not load debugger.\n");
            return false;
        }
    }
    if (m_RpcServer)
    {
        if (!m_RpcServer->OnLoad())
        {
            fprintf(stderr, "err: could not load rpc server.\n");
            return false;
        }
    }

    return true;
}