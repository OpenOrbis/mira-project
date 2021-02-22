#include "Connection.hpp"

#include <Daemon.hpp>
#include <Rpc/Manager.hpp>

#include <cstdio>
#include <vector>
#include <array>

#include <External/flatbuffers/rpc_generated.h>

extern "C"
{
    #include <sys/socket.h>
    #include <unistd.h>
};

using namespace Mira::Rpc;

Connection::Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address) :
    m_Socket(p_Socket),    
    m_ClientId(p_ClientId),
    m_Running(false),
    m_Thread(-1),
    m_Address { 0 },
    m_Server(p_Server)
{
    memcpy(&m_Address, &p_Address, sizeof(m_Address));
}

Connection::~Connection()
{
    auto s_Ret = 0;

    // Close the socket
    do
    {
        if (m_Socket == -1)
            break;
        
        s_Ret = shutdown(m_Socket, SHUT_RDWR);
        if (s_Ret != 0)
        {
            fprintf(stderr, "err: could not shutdown socket (%d).\n", s_Ret);
            break;
        }

        s_Ret = close(m_Socket);
        if (s_Ret != 0)
        {
            fprintf(stderr, "err: could not close socket (%d).\n", s_Ret);
            break;
        }
    } while (false);
    

    // Cancel the thread if needed
    do
    {
        void* s_RetVal = nullptr;

        // Check the thread
        if (m_Thread == -1)
            break;

        // Cancel the thread
    #if defined(PS4)
        s_Ret = scePthreadCancel(m_Thread);
    #else
        s_Ret = pthread_cancel(m_Thread);
    #endif
        if (s_Ret != 0)
        {
            fprintf(stderr, "err: cancel connection thread returned (%d).\n", s_Ret);
            break;
        }

        // Wait for the thread to exit
    #if defined(PS4)
        s_Ret = scePthreadJoin(m_Thread, &s_RetVal);
    #else
        s_Ret = pthread_join(m_Thread, &s_RetVal);
    #endif
        if (s_Ret != 0)
        {
            fprintf(stderr, "err: could not join thread (%lu) ret (%d).\n", m_Thread, s_Ret);
            break;
        }
    } while (false);
}

void Connection::Disconnect()
{
    m_Running = false;
}

void* Connection::ConnectionThread(void* p_ConnectionInstance)
{
    auto s_Connection = static_cast<Connection*>(p_ConnectionInstance);
    if (s_Connection == nullptr)
        return nullptr;
    
    auto s_Socket = s_Connection->GetSocket();
    if (s_Socket < 0)
        return nullptr;
    
    uint64_t s_IncomingMessageSize = 0;
    ssize_t s_Ret = -1;

    while ((s_Ret = recv(s_Socket, &s_IncomingMessageSize, sizeof(s_IncomingMessageSize), 0)) > 0 && s_Connection->IsRunning())
    {
        if (s_Ret != sizeof(s_IncomingMessageSize))
        {
            fprintf(stderr, "err: could not get message size.\n");
            break;
        }

        if (s_IncomingMessageSize > RpcConnection_MaxMessageSize)
        {
            fprintf(stderr, "err: incoming message size (0x%lx) > max (0x%x).\n", s_IncomingMessageSize, RpcConnection_MaxMessageSize);
            break;
        }

        if (s_IncomingMessageSize == 0)
        {
            fprintf(stderr, "err: invalid incoming message size of 0.\n");
            break;
        }

        // Allocate the new data
        std::vector<uint8_t> s_Data;
        s_Data.reserve(s_IncomingMessageSize);

        // Read in all of the data from the socket
        s_Ret = recv(s_Socket, s_Data.data(), s_IncomingMessageSize, 0);
        if (s_Ret != s_IncomingMessageSize)
        {
            fprintf(stderr, "err: wanted data (0x%zx) != (0x%lx).\n", s_Ret, s_IncomingMessageSize);
            break;
        }

        // Create a new verifier
        auto s_Verifier = flatbuffers::Verifier(s_Data.data(), s_IncomingMessageSize);

        // Validate the header
        auto s_Valid = VerifyRpcHeaderBuffer(s_Verifier);
        if (!s_Valid)
        {
            fprintf(stderr, "err: could not verify flatbuffer.\n");
            break;
        }

        // Get the header
        auto s_Header = GetRpcHeader(s_Data.data());
        if (s_Header == nullptr)
        {
            fprintf(stderr, "err: header invalid.\n");
            break;
        }

        // Validate the header magic
        if (s_Header->magic() != RpcMagics_Version2)
        {
            fprintf(stderr, "err: invalid magic (%d) wanted (%d).\n", s_Header->magic(), RpcMagics_Version2);
            break;
        }

        // Validate the category
        if (s_Header->category() < RpcCategory_NONE || s_Header->category() >= RpcCategory_MAX)
        {
            fprintf(stderr, "err: invalid category (%d).\n", s_Header->category());
            break;
        }

        auto s_MessageManager = Daemon::GetInstance()->GetMessageManager();
        if (s_MessageManager == nullptr)
        {
            fprintf(stderr, "err: no message manager.\n");
            break;
        }

        s_MessageManager->OnRequest(s_Connection, s_Header);
    }

    return nullptr;
}