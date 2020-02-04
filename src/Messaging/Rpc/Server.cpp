#include "Server.hpp"
#include "Connection.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>

#include <Utils/SysWrappers.hpp>

#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/filedesc.h>

#include <Mira.hpp>

using namespace Mira::Messaging::Rpc;



Server::Server(uint16_t p_Port) :
    m_Socket(-1),
    m_Port(p_Port),
    m_Thread(nullptr),
    m_Running(false),
    m_NextConnectionId(1),
    m_Connections { 0 }
{
    // Zero out the address
    memset(&m_Address, 0, sizeof(m_Address));
}

Server::~Server()
{
    
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
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);
    // Create a socket
    m_Socket = ksocket_t(AF_INET, SOCK_STREAM, 0, s_MainThread);
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
    auto s_Ret = kbind_t(m_Socket, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address), s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not bind socket (%d).", s_Ret);
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
        m_Socket = -1;
        return false;
    }
    WriteLog(LL_Info, "socket (%d) bound to port (%d).", m_Socket, m_Port);

    // Listen on the port for new connections
    s_Ret = klisten_t(m_Socket, RpcServer_MaxConnections, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not listen on socket (%d).", s_Ret);
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
		m_Socket = -1;
		return false;
    }

    // Create the new server processing thread, 8MiB stack
    s_Ret = kthread_add(Server::ServerThread, this, Mira::Framework::GetFramework()->GetInitParams()->process, reinterpret_cast<thread**>(&m_Thread), 0, 200, "RpcServer");
    WriteLog(LL_Debug, "rpcserver kthread_add returned (%d).", s_Ret);

    return s_Ret == 0;
}

bool Server::Teardown()
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    // Set that we no longer want to run this server instance
    m_Running = false;
    WriteLog(LL_Debug, "we are no longer running...");

    // Iterate through all connections and disconnect them
    for (auto i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        // Check that we have a valid connection
        auto l_Connection = m_Connections[i];
        if (l_Connection == nullptr)
            continue;
        
        // Disconnect the client, the client will fire the Server::OnConnectionDisconnected
        if (l_Connection->IsRunning())
            l_Connection->Disconnect();
    }

    WriteLog(LL_Debug, "all clients have been disconnected");

    // Close the server socket
    if (m_Socket > 0)
    {
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
        m_Socket = -1;
    }

    WriteLog(LL_Debug, "socket is killed");

    return true;
}

void Server::ServerThread(void* p_UserArgs)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "no main thread");
        kthread_exit();
        return;
    }

    //auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

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

    int s_ClientSocket = -1;
    struct sockaddr_in s_ClientAddress = { 0 };
    size_t s_ClientAddressLen = sizeof(s_ClientAddress);
    memset(&s_ClientAddress, 0, s_ClientAddressLen);
    s_ClientAddress.sin_len = s_ClientAddressLen;

    fd_set s_ReadFds;
    FD_ZERO(&s_ReadFds);
    FD_SET(s_Server->m_Socket, &s_ReadFds);
    while ((s_ClientSocket = kaccept_t(s_Server->m_Socket, reinterpret_cast<struct sockaddr*>(&s_ClientAddress), &s_ClientAddressLen, s_MainThread)) > 0)
    {
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
        auto result = ksetsockopt_t(s_ClientSocket, SOL_SOCKET, SO_LINGER, (caddr_t)&s_Timeout, sizeof(s_Timeout), s_MainThread);
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set send timeout (%d).", result);
            kshutdown_t(s_ClientSocket, SHUT_RDWR, s_MainThread);
            kclose_t(s_ClientSocket, s_MainThread);
            continue;
        }

        uint32_t l_Addr = (uint32_t)s_ClientAddress.sin_addr.s_addr;

        WriteLog(LL_Debug, "got new rpc connection (%d) from IP (%03d.%03d.%03d.%03d).", s_ClientSocket, 
            (l_Addr & 0xFF),
            (l_Addr >> 8) & 0xFF,
            (l_Addr >> 16) & 0xFF,
            (l_Addr >> 24) & 0xFF);
        
        auto l_Connection = new Rpc::Connection(s_Server, s_Server->m_NextConnectionId, s_ClientSocket, s_ClientAddress);
        if (!l_Connection)
        {
            WriteLog(LL_Error, "could not allocate new connection for socket (%d).", s_ClientSocket);

            kshutdown_t(s_ClientSocket, SHUT_RDWR, s_MainThread);
            kclose_t(s_ClientSocket, s_MainThread);
            break;
        }

        auto s_FreeIndex = s_Server->GetFreeConnectionIndex();
        if (s_FreeIndex < 0)
        {
            WriteLog(LL_Error, "could not get free connection index");

            kshutdown_t(s_ClientSocket, SHUT_RDWR, s_MainThread);
            kclose_t(s_ClientSocket, s_MainThread);

            delete l_Connection;
            l_Connection = nullptr;
            break;
        }
        s_Server->m_Connections[s_FreeIndex] = l_Connection;

        // Send off for new client thread creation
        s_Server->OnHandleConnection(l_Connection);

        // Zero out the address information for the next call
        memset(&s_ClientAddress, 0, s_ClientAddressLen);
        s_ClientAddress.sin_len = s_ClientAddressLen;

        if (!s_Server->m_Running)
            break;
        
        // TODO: Handle timeout
    }

    // Disconnects all clients, set running to false
    WriteLog(LL_Debug, "rpcserver tearing down");
    s_Server->Teardown();

    WriteLog(LL_Debug, "rpcserver exiting cleanly");
    kthread_exit();
}

void Server::OnHandleConnection(Rpc::Connection* p_Connection)
{
    // This does not need kthread_exit as it is called "flatly"
    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);

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

    // Get connection, 8MiB stack
    auto s_Ret = kthread_add(Connection::ConnectionThread, p_Connection, s_Process, reinterpret_cast<thread **>(p_Connection->Internal_GetThread()), 0, 200, "RpcConn");
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not start new connection thread (%d).", s_Ret);
        return;
    }
}

void Server::OnConnectionDisconnected(Rpc::Connection* p_Connection)
{
    //auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (p_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid connection");
        return;
    }
    WriteLog(LL_Debug, "client disconnect (%p).", p_Connection);

    // Iterate through all connections and disconnect them
    for (auto i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        // Check that we have a valid connection
        auto l_Connection = m_Connections[i];
        if (l_Connection == nullptr)
            continue;
        
        if (l_Connection->GetId() != p_Connection->GetId())
            continue;

        // Disconnect the client, the client will fire the Server::OnConnectionDisconnected
        m_Connections[i] = nullptr;

        WriteLog(LL_Debug, "freeing connection at (%d) (%p).", i, l_Connection);
        delete l_Connection;
        break;
    }
}

int32_t Server::GetSocketById(uint32_t p_Id)
{
    //auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    int32_t s_SocketId = -1;
    for (uint32_t i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        auto l_Connection = m_Connections[i];
        if (!l_Connection)
            continue;
        
        if (l_Connection->IsRunning())
            continue;
        
        if (p_Id != l_Connection->GetId())
            continue;
        
        s_SocketId = l_Connection->GetSocket();
        break;
    }

    return s_SocketId;
}

int32_t Server::GetFreeConnectionCount()
{
    int32_t s_Count = 0;
    for (auto i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        if (m_Connections[i] == nullptr)
            s_Count++;
    }
    return s_Count;
}

int32_t Server::GetUsedConnectionCount()
{
    int32_t s_Count = 0;
    for (auto i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        if (m_Connections[i] != nullptr)
            s_Count++;
    }
    return s_Count;
}

int32_t Server::GetFreeConnectionIndex()
{
    for (auto i = 0; i < ARRAYSIZE(m_Connections); ++i)
    {
        if (m_Connections[i] == nullptr)
            return i;
    }
    return -1;
}