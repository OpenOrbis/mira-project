#include "Mira.hpp"
#include <Messaging/MessageManager.hpp>
#include <Rpc/Server.hpp>

Mira::Framework* Mira::Framework::m_Instance = nullptr;

Mira::Framework* Mira::Framework::GetFramework()
{
    if (m_Instance == nullptr)
        m_Instance = new Mira::Framework();
    
    return m_Instance;
}

Mira::Framework::Framework()
{
}

bool Mira::Framework::Initialize()
{
    m_MessageManager = new Mira::Messaging::MessageManager();

    m_Server = new Mira::Messaging::Rpc::Server();
    if (m_Server)
        m_Server->OnLoad();

    return true;
}