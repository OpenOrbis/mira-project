#include "Server.hpp"
#include "Connection.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>

#include <Utils/SysWrappers.hpp>

#include <sys/proc.h>
#include <sys/socket.h>

#include <Mira.hpp>

using namespace Mira::Messaging::Rpc;



Server::Server(uint16_t p_Port) :
    m_Socket(-1),
    m_Port(p_Port),
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

bool Server::OnLoad()
{
    WriteLog(LL_Info, "starting rpc server");
    return Startup();
}

bool Server::OnUnload()
{
    WriteLog(LL_Error, "unloading rpc server");
    return Teardown();
}

bool Server::OnSuspend()
{
    WriteLog(LL_Error, "suspending rpc server");
    return Teardown();
}

bool Server::OnResume()
{
    WriteLog(LL_Error, "resuming rpc server");
    return Startup();
}

bool Server::Startup()
{
    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

    // Create a socket
    m_Socket = ksocket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket < 0)
    {
        WriteLog(LL_Error, "could not initialize socket (%d).", m_Socket);
        return false;
    }
    WriteLog(LL_Info, "socket created: (%d).", m_Socket);

    // Set our server to listen on 0.0.0.0:<port>
    memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
    m_Address.sin_addr.s_addr = htonl(INADDR_ANY);
    m_Address.sin_port = htons(m_Port);
    m_Address.sin_len = sizeof(m_Address);

    // Bind to port
    auto s_Ret = kbind(m_Socket, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address));
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not bind socket (%d).", s_Ret);
        kclose(m_Socket);
        m_Socket = -1;
        return false;
    }
    WriteLog(LL_Info, "socket (%d) bound to port (%d).", m_Socket, m_Port);

    // Listen on the port for new connections
    s_Ret = klisten(m_Socket, RpcServer_MaxConnections);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not listen on socket (%d).", s_Ret);
		kclose(m_Socket);
		m_Socket = -1;
		return false;
    }

    // Create the new server processing thread
    s_Ret = kthread_add(Server::ServerThread, this, Mira::Framework::GetFramework()->GetInitParams()->process, reinterpret_cast<thread**>(&m_Thread), 0, 0, "RpcServer");
    WriteLog(LL_Debug, "rpcserver kthread_add returned (%d).", s_Ret);

    return s_Ret == 0;
}

bool Server::Teardown()
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    // Set that we no longer want to run this server instance
    m_Running = false;

    // Iterate through all connections and disconnect them
    _mtx_lock_flags(&m_Lock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Connections.size(); ++i)
    {
        // Check that we have a valid connection
        auto l_Connection = m_Connections[i];
        if (!l_Connection)
            continue;
        
        // Disconnect the client, the client will fire the Server::OnConnectionDisconnected
        l_Connection->Disconnect();
    }
    _mtx_unlock_flags(&m_Lock, 0, __FILE__, __LINE__);

    // Close the server socket
    if (m_Socket > 0)
    {
        kshutdown(m_Socket, 0);
        kclose(m_Socket);
        m_Socket = -1;
    }

    return true;
}

void Server::ServerThread(void* p_UserArgs)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    // Check for invalid usage
    Server* s_Server = static_cast<Server*>(p_UserArgs);
    if (s_Server == nullptr)
    {
        WriteLog(LL_Error, "invalid usage, userargs are null.");
        kthread_exit();
        return;
    }

    // Set our running state
    s_Server->m_Running = true;

    // Create our timeout
    struct timeval s_Timeout
    {
        .tv_sec = 3,
        .tv_usec = 0
    };

    while (s_Server->m_Running)
    {
        struct sockaddr_in l_ClientAddress;
        size_t l_ClientAddressLen = sizeof(l_ClientAddress);
        memset(&l_ClientAddress, 0, l_ClientAddressLen);
        l_ClientAddress.sin_len = l_ClientAddressLen;

        // Accept a new client
        auto l_ClientSocket = kaccept(s_Server->m_Socket, reinterpret_cast<struct sockaddr*>(&l_ClientAddress), &l_ClientAddressLen);
        if (l_ClientSocket < 0)
        {
            WriteLog(LL_Error, "could not accept socket (%d).", l_ClientSocket);
            break;
        }

        // Set the send recv and linger timeouts
        /*int32_t result = ksetsockopt(l_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (caddr_t)&s_Timeout, sizeof(s_Timeout));
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set recv timeout (%d).", result);
            kclose(l_ClientSocket);
            continue;
        }

        result = ksetsockopt(l_ClientSocket, SOL_SOCKET, SO_SNDTIMEO, (caddr_t)&s_Timeout, sizeof(s_Timeout));
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set send timeout (%d).", result);
            kclose(l_ClientSocket);
            continue;
        }*/

        // SO_LINGER
        s_Timeout.tv_sec = 0;
        auto result = ksetsockopt(l_ClientSocket, SOL_SOCKET, SO_LINGER, (caddr_t)&s_Timeout, sizeof(s_Timeout));
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set send timeout (%d).", result);
            kclose(l_ClientSocket);
            continue;
        }

        uint32_t l_Addr = (uint32_t)l_ClientAddress.sin_addr.s_addr;

		WriteLog(LL_Debug, "got new rpc connection (%d) from IP (%03d.%03d.%03d.%03d).", l_ClientSocket, 
			(l_Addr & 0xFF),
			(l_Addr >> 8) & 0xFF,
			(l_Addr >> 16) & 0xFF,
			(l_Addr >> 24) & 0xFF);
        
        auto l_Connection = shared_ptr<Rpc::Connection>(new Rpc::Connection(s_Server, s_Server->m_NextConnectionId, l_ClientSocket, l_ClientAddress));
        if (!l_Connection)
        {
            WriteLog(LL_Error, "could not allocate new connection for socket (%d).", l_ClientSocket);
            kclose(l_ClientSocket);
            break;
        }

        _mtx_lock_flags(&s_Server->m_Lock, 0, __FILE__, __LINE__);
        s_Server->m_Connections.push_back(l_Connection);
        _mtx_unlock_flags(&s_Server->m_Lock, 0, __FILE__, __LINE__);

        // Send off for new client thread creation
        OnHandleConnection(s_Server, l_Connection.get());
    }

    // Disconnects all clients, set running to false
    WriteLog(LL_Debug, "rpcserver tearing down");
    s_Server->Teardown();

    WriteLog(LL_Debug, "rpcserver exiting cleanly");
    kthread_exit();
}

void Server::OnHandleConnection(Server* p_Instance, Rpc::Connection* p_Connection)
{
    // This does not need kthread_exit as it is called "flatly"
    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

    // Validate server instance
    if (p_Instance == nullptr)
    {
        WriteLog(LL_Error, "invalid server instance");
        return;
    }

    // Validate connection instance
    if (!p_Connection)
    {
        WriteLog(LL_Error, "invalid connection instance");
        return;
    }

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework");
        return;
    }

    auto s_Process = s_Framework->GetInitParams()->process;
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "could not get mira process");
        return;
    }

    // Get connection
    auto s_Ret = kthread_add(Connection::ConnectionThread, p_Connection, s_Process, reinterpret_cast<thread **>(p_Connection->Internal_GetThread()), 0, 0, "RpcConn");
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not start new connection thread (%d).", s_Ret);
        return;
    }
}

void Server::OnConnectionDisconnected(Server* p_Instance, Rpc::Connection* p_Connection)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);


    if (p_Instance == nullptr || p_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid instance");
        return;
    }

    WriteLog(LL_Debug, "client disconnect (%p).", p_Connection);

    // Iterate through all connections and disconnect them
    _mtx_lock_flags(&p_Instance->m_Lock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < p_Instance->m_Connections.size(); ++i)
    {
        // Check that we have a valid connection
        auto l_Connection = p_Instance->m_Connections[i];
        if (!l_Connection)
            continue;
        
        if (l_Connection.get() != p_Connection)
            continue;

        WriteLog(LL_Debug, "freeing connection at (%d).", i);

        // Disconnect the client, the client will fire the Server::OnConnectionDisconnected
        l_Connection.reset();
        p_Instance->m_Connections[i] = shared_ptr<Rpc::Connection>(nullptr);
        break;
    }
    _mtx_unlock_flags(&p_Instance->m_Lock, 0, __FILE__, __LINE__);
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

int32_t Server::Deprecated_GetSocketByThread(struct thread* p_Thread)
{
    if (p_Thread == nullptr)
        return -1;
    
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
        
        if (p_Thread != l_Connection->GetConnectionThread())
            continue;
        
        s_SocketId = l_Connection->GetSocket();
        break;
    }
    _mtx_unlock_flags(&m_Lock, 0, __FILE__, __LINE__);

    return s_SocketId;
}