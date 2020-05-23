// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Connection.hpp"
#include "Server.hpp"

#include <Messaging/MessageManager.hpp>

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Span.hpp>
#include <Utils/SysWrappers.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/socket.h>
    #include <sys/filedesc.h>
    #include <sys/proc.h>
    #include <sys/pcpu.h>
    
    #include "rpc.pb-c.h"
}


using namespace Mira::Messaging::Rpc;

Connection::Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address) :
    m_Socket(p_Socket),
    m_Id(p_ClientId),
    m_Running(false),
    m_Thread(nullptr),
    m_Address{0},
    m_Server(p_Server),
    m_MessageBuffer(new uint8_t[MaxBufferSize])
{
    memcpy(&m_Address, &p_Address, sizeof(m_Address));

    if (m_MessageBuffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate message buffer.");
        return;
    }

    // Zero out our buffer
    memset(m_MessageBuffer, 0, MaxBufferSize);
}

Connection::~Connection()
{
    if (m_Running)
        Disconnect();
    
    if (m_MessageBuffer)
    {
        delete [] m_MessageBuffer;
        m_MessageBuffer = nullptr;
    }
}

void Connection::Disconnect()
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return;
    }

    if (m_Running)
        m_Running = false;
    
    if (m_Socket > 0)
    {
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
        kclose_t(m_Socket, s_MainThread);
        m_Socket = -1;
    }

    WriteLog(LL_Debug, "client (%p) disconnected.", this);

    if (m_Server != nullptr)
        m_Server->OnConnectionDisconnected(this);
}

void Connection::ConnectionThread(void* p_Connection)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        kthread_exit();
        return;
    }

    // Validate connection
    auto s_Connection = reinterpret_cast<Rpc::Connection*>(p_Connection);
    if (s_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid connection instance");
        kthread_exit();
        return;
    }

    // Validate that we have a buffer
    if (s_Connection->m_MessageBuffer == nullptr)
    {
        WriteLog(LL_Error, "there is no message buffer");
        kthread_exit();
        return;
    }

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get mira framework.");
        kthread_exit();
        return;
    }

    auto s_MessageManager = s_Framework->GetMessageManager();
    if (s_MessageManager == nullptr)
    {
        WriteLog(LL_Error, "could not get message manager");
        kthread_exit();
        return;
    }

    WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%x), thread: (%p).", 
                    s_Connection->m_Socket, 
                    s_Connection->m_Address.sin_addr.s_addr, 
                    s_Connection->m_Thread);

    s_Connection->m_Running = true;

    uint64_t s_IncomingMessageSize = 0;
    ssize_t s_Ret = -1;
    while ((s_Ret = krecv_t(s_Connection->GetSocket(), &s_IncomingMessageSize, sizeof(s_IncomingMessageSize), 0, s_MainThread)) > 0)
    {
        // Make sure that we got all of the data
        if (s_Ret != sizeof(s_IncomingMessageSize))
        {
            WriteLog(LL_Error, "could not get message header.");
            break;
        }

        // Validate the incoming message size
        if (s_IncomingMessageSize > MaxBufferSize)
        {
            WriteLog(LL_Error, "incoming message size (%llx) > max buffer size (%llx)", s_IncomingMessageSize, MaxBufferSize);
            break;
        }

        // Allocate data for our incoming message size
        auto s_IncomingMessageData = new uint8_t[s_IncomingMessageSize];
        if (s_IncomingMessageData == nullptr)
        {
            WriteLog(LL_Error, "could not allocate incoming message size (%llx)", s_IncomingMessageSize);
            break;
        }
        memset(s_IncomingMessageData, 0, s_IncomingMessageSize);

        // Get the message data from the wire
        s_Ret = krecv_t(s_Connection->GetSocket(), s_IncomingMessageData, s_IncomingMessageSize, 0, s_MainThread);
        if (s_Ret != s_IncomingMessageSize)
        {
            WriteLog(LL_Error, "did not get correct amount of data (%llx) wanted (%llx)", s_Ret, s_IncomingMessageSize);
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        RpcTransport* s_Transport = rpc_transport__unpack(nullptr, s_IncomingMessageSize, s_IncomingMessageData);
        if (s_Transport == nullptr)
        {
            WriteLog(LL_Error, "error unpacking incoming message");
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        // Validate the header magic
        auto s_Header = s_Transport->header;

        if (s_Header == nullptr)
        {
            WriteLog(LL_Error, "could not get the transport header.");
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        if (s_Header->magic != 2)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", s_Header->magic, 2);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        // Validate message category
        auto s_Category = s_Header->category;
        if (s_Category < RPC_CATEGORY__NONE || s_Category > RPC_CATEGORY__MAX)
        {
            WriteLog(LL_Error, "invalid category (%d).", s_Category);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        // We do not want to handle any responses
        if (!s_Header->isrequest)
        {
            WriteLog(LL_Error, "attempted to handle outgoing message, fix ya code");
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        // Bounds check the span
        if (s_Transport->data.len > MaxBufferSize)
        {
            WriteLog(LL_Error, "transport data does not have enough data, wanted (%d), have (%d).", s_Transport->data.len, MaxBufferSize);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
            delete [] s_IncomingMessageData;
            break;
        }

        s_MessageManager->OnRequest(s_Connection, *s_Transport);

        // Zero out the header for use next iteration
        s_IncomingMessageSize = 0;

        rpc_transport__free_unpacked(s_Transport, nullptr);
        memset(s_IncomingMessageData, 0, s_IncomingMessageSize);
        delete [] s_IncomingMessageData;
    }

    s_Connection->Disconnect();

    kthread_exit();
}