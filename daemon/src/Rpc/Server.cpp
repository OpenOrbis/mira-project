#include "Server.hpp"
#include "Connection.hpp"

#include <cstdio>

extern "C"
{
    #include <unistd.h>
    #include <sys/time.h>
};

using namespace Mira::Rpc;

Server::Server(uint16_t p_Port) :
    m_Socket(-1),
    m_Port(p_Port),
    m_Thread(-1),
    m_Running(false),
    m_Address { 0 }
{

}

Server::~Server()
{

}

bool Server::OnLoad()
{
    return Startup();
}

bool Server::OnUnload()
{
    return Teardown();
}

bool Server::OnSuspend()
{
    return Teardown();
}

bool Server::OnResume()
{
    return Startup();
}

bool Server::Startup()
{
    // Debugging output
    printf("rpc server starting...\n");

    do
    {
        // Configure the address
        memset(&m_Address, 0, sizeof(m_Address));
        m_Address.sin_family = AF_INET;
        m_Address.sin_addr.s_addr = htonl(INADDR_ANY);
        m_Address.sin_port = htons(m_Port);
        
        // Create a new socket
        auto s_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s_Socket <= 0)
        {
            fprintf(stderr, "err: could not create socket (%d).\n", s_Socket);
            break;
        }

        // Bind a new socket
        auto s_Ret = bind(s_Socket, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address));
        if (s_Ret < 0)
        {
            fprintf(stderr, "err: could not bind socket (%d).\n", s_Ret);
            
            // Shutdown socket
            s_Ret = shutdown(s_Socket, SHUT_RDWR);
            if (s_Ret < 0)
                fprintf(stderr, "err: could not shutdown socket (%d).\n", s_Ret);
            
            // Close socket
            s_Ret = close(s_Socket);
            if (s_Ret < 0)
                fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);
            
            break;
        }

        printf("info: socket (%d) bound to port (%d).\n", s_Socket, m_Port);

        // Listen for new connections
        s_Ret = listen(m_Socket, RpcServer_MaxConnections);
        if (s_Ret < 0)
        {
            fprintf(stderr, "err: could not listen on socket (%d).\n", s_Ret);
            
            // Shutdown socket
            s_Ret = shutdown(s_Socket, SHUT_RDWR);
            if (s_Ret < 0)
                fprintf(stderr, "err: could not shutdown socket (%d).\n", s_Ret);
            
            // Close socket
            s_Ret = close(s_Socket);
            if (s_Ret < 0)
                fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);
            
            break;
        }

        // Create a new server processing thread
        
        s_Ret = scePthreadCreate(&m_Thread, nullptr, (void*)ServerThread, this, "RpcServer"); //pthread_create(&m_Thread, nullptr, ServerThread, this);
        if (s_Ret != 0)
        {
            fprintf(stderr, "err: could not create new thread (%d).\n", s_Ret);
            
            // Teardown
            if (!Teardown())
                fprintf(stderr, "err: could not teardown.\n");
            
            break;
        }

        m_Socket = s_Socket;
        return true;
    } while (false);
    

    return true;
}

bool Server::Teardown()
{
    return true;
}

void* Server::ServerThread(void* p_ServerInstance)
{
    auto s_Server = static_cast<Server*>(p_ServerInstance);
    do
    {
        if (s_Server == nullptr)
            break;
        
        struct timeval s_Timeout
        {
            .tv_sec = 0,
            .tv_usec = 0
        };
        
        // Hold the client address
        struct sockaddr_in s_ClientAddress = { 0 };
        socklen_t s_ClientAddressLength = sizeof(s_ClientAddress);

        int32_t s_ClientSocket = -1;

        fd_set s_ReadFds;
        FD_ZERO(&s_ReadFds);
        FD_SET(s_Server->m_Socket, &s_ReadFds);

        auto s_Ret = 0;
        while ((s_ClientSocket = accept(s_Server->m_Socket, reinterpret_cast<struct sockaddr*>(&s_ClientAddress), &s_ClientAddressLength)) > 0)
        {
            // Set the client socket options
            s_Ret = setsockopt(s_ClientSocket, SOL_SOCKET, SO_LINGER, &s_Timeout, sizeof(s_Timeout));
            if (s_Ret < 0)
            {
                fprintf(stderr, "err: could not set linger options (%d).\n", s_Ret);
                
                // Shutdown socket
                s_Ret = shutdown(s_ClientSocket, SHUT_RDWR);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not shutdown client socket (%d).\n", s_Ret);
                
                // Close socket
                s_Ret = close(s_ClientSocket);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);

                break;
            }

            // Print out the address
            auto l_Address = static_cast<uint32_t>(s_ClientAddress.sin_addr.s_addr);
            printf("got new rpc connection (%d) from IP (%03d.%03d.%03d.%03d).\n",
                s_ClientSocket,
                (l_Address & 0xFF),
                (l_Address >> 8) & 0xFF,
                (l_Address >> 16) & 0xFF,
                (l_Address >> 24) & 0xFF);
            
            // Allocate a new connection
            auto l_Connection = std::make_shared<Rpc::Connection>(s_Server, ++s_Server->m_NextConnectionId, s_ClientSocket, s_ClientAddress);
            if (l_Connection == nullptr)
            {
                fprintf(stderr, "err: could not allocate connection.\n");

                // Shutdown socket
                s_Ret = shutdown(s_ClientSocket, SHUT_RDWR);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not shutdown client socket (%d).\n", s_Ret);
                
                // Close socket
                s_Ret = close(s_ClientSocket);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);

                break;
            }

            // Create a new thread
            s_Ret = scePthreadCreate(l_Connection->GetThreadPointer(), nullptr, (void*)Connection::ConnectionThread, l_Connection.get(), "RpcConn");
            if (s_Ret != 0)
            {
                fprintf(stderr, "err: could not create new connection thread (%d).\n", s_Ret);
                
                // Shutdown socket
                s_Ret = shutdown(s_ClientSocket, SHUT_RDWR);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not shutdown client socket (%d).\n", s_Ret);
                
                // Close socket
                s_Ret = close(s_ClientSocket);
                if (s_Ret != 0)
                    fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);

                break;
            }

            // Add the connection to the server list
            s_Server->m_Connections.push_back(std::move(l_Connection));
        }

    } while (false);

    return nullptr;
}