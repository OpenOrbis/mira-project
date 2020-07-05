#include "Daemon.hpp"

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

    return true;
}