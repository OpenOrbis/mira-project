#include "Daemon.hpp"

#include <flatbuffers/flatbuffers.h>
#include <External/flatbuffers/rpc_generated.h>

#include <Rpc/Manager.hpp>
#include <Rpc/Server.hpp>

#include <Debugging/Debugger.hpp>

#include <cstdio>

using namespace Mira;

Daemon::Daemon() :
    m_Debugger(nullptr),
    m_FtpServer(nullptr),
    m_MessageManager(nullptr),
    m_RpcServer(nullptr)
{
    // Initialize the networking
    sceNetInit();
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
    // Create the message manager if it isn't already
    if (!m_MessageManager)
        m_MessageManager = std::make_shared<Rpc::Manager>();
    
    // Create the debugger if it isn't already
    if (!m_Debugger)
        m_Debugger = std::make_shared<Debugging::Debugger>();
    
    // Load the debugger
    if (!m_Debugger->OnLoad())
    {
        fprintf(stderr, "err: could not load debugger.\n");
        return false;
    }

    // Create a new rpc server instance if it hasn't already
    if (!m_RpcServer)
        m_RpcServer = std::make_shared<Rpc::Server>();

    // Load the rpc server instance
    if (!m_RpcServer->OnLoad())
    {
        fprintf(stderr, "err: could not load rpc server.\n");
        return false;
    }

    return true;
}