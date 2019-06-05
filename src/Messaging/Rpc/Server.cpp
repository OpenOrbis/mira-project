#include "Server.hpp"
#include "Connection.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>

using namespace Mira::Messaging::Rpc;

Server::Server(uint16_t p_Port) :
    m_Socket(-1),
    m_Thread(nullptr),
    m_Running(false),
    m_NextConnectionId(1)
{
    // Zero out the address
    memset(&m_Address, 0, sizeof(m_Address));

    // Initialize the mutex
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    mtx_init(&m_Lock, "RpcSrvMtx", nullptr, 0);
}

Server::~Server()
{
    // TODO: implement
}

bool Server::Startup()
{
    //auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

    //kthread_add(nullptr, nullptr, nullptr, nullptr, 0, 0, "");
    return true;
}

bool Server::Teardown()
{
    return true;
}

void Server::OnHandleConnection(Server* p_Instance, shared_ptr<Rpc::Connection> p_Connection)
{

}

void Server::OnConnectionDisconnected(Server* p_Instance, shared_ptr<Rpc::Connection> p_Connection)
{

}

int32_t Server::GetSocketById(uint32_t p_Id)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int32_t s_SocketId = -1;
    _mtx_lock_flags(&m_Lock, 0, __FILE__, __LINE__);
    for (uint32_t i = 0; i < m_Connections.size(); ++i)
    {
        auto l_Connection = m_Connections[i];
        if (!l_Connection)
            continue;
        
        if (!l_Connection->IsRunning())
            continue;
        
        if (p_Id != l_Connection->GetId())
            continue;
        
        s_SocketId = l_Connection->GetSocket();
        break;
    }
    _mtx_unlock_flags(&m_Lock, 0, __FILE__, __LINE__);

    return s_SocketId;
}