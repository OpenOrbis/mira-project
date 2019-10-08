#include "Connection.hpp"
#include "Server.hpp"

#include <Messaging/Message.hpp>
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
}


using namespace Mira::Messaging::Rpc;

Connection::Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address) :
    m_Socket(p_Socket),
    m_Id(p_ClientId),
    m_Running(false),
    m_Thread(nullptr),
    m_Address{0},
    m_Server(p_Server),
    m_MessageBuffer{0}
{
    memcpy(&m_Address, &p_Address, sizeof(m_Address));
}

Connection::~Connection()
{
    
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
}

void Connection::ConnectionThread(void* p_Connection)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return;
    }

    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

    // Validate connection
    auto s_Connection = reinterpret_cast<Rpc::Connection*>(p_Connection);
    if (s_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid connection instance");
        kthread_exit();
        return;
    }

    WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%x), thread: (%p).", 
                    s_Connection->m_Socket, 
                    s_Connection->m_Address.sin_addr.s_addr, 
                    s_Connection->m_Thread);

    s_Connection->m_Running = true;

    // Get the header size
    const auto s_MessageHeaderSize = sizeof(Messaging::MessageHeader);
    const auto s_MessageBufferSize = sizeof(s_Connection->m_MessageBuffer);
    
    while (s_Connection->IsRunning())
    {
        // Zero out our entire buffer for a new message
        memset(s_Connection->m_MessageBuffer, 0, s_MessageBufferSize);

        Messaging::MessageHeader l_MessageHeader = { 0 };

        // Recv the data length of this message
        ssize_t l_Ret = krecv_t(s_Connection->GetSocket(), &l_MessageHeader, sizeof(l_MessageHeader), 0, s_MainThread);
        if (l_Ret <= 0)
        {
            WriteLog(LL_Error, "recv returned (%d).", l_Ret);
            break;
        }

        // Make sure that we got all of the data
        if (l_Ret != s_MessageHeaderSize)
        {
            WriteLog(LL_Error, "could not get message header.");
            break;
        }

        // Validate the header magic
        if (l_MessageHeader.magic != MessageHeaderMagic)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", l_MessageHeader.magic, MessageHeaderMagic);
            break;
        }

        // Validate message category
        auto s_Category = static_cast<MessageCategory>(l_MessageHeader.category);
        if (s_Category < MessageCategory_None || s_Category > MessageCategory_Max)
        {
            WriteLog(LL_Error, "invalid category (%d).", s_Category);
            break;
        }

        // We do not want to handle any responses
        if (l_MessageHeader.isRequest == false)
        {
            WriteLog(LL_Error, "attempted to handle outgoing message, fix ya code");
            break;
        }

        // Bounds check the span
        if (l_MessageHeader.payloadLength > s_MessageBufferSize)
        {
            WriteLog(LL_Error, "span does not have enough data, wanted (%d), have (%d).", l_MessageHeader.payloadLength, s_MessageBufferSize);
            break;
        }

        uint32_t l_TotalRecv = 0;
        l_Ret = krecv_t(s_Connection->GetSocket(), s_Connection->m_MessageBuffer, l_MessageHeader.payloadLength, 0, s_MainThread);
        if (l_Ret <= 0)
        {
            WriteLog(LL_Error, "could not recv data err: (%d).", l_Ret);
            break;
        }

        l_TotalRecv += l_Ret;
        while (l_TotalRecv < l_MessageHeader.payloadLength)
        {
            auto l_DataLeft = l_MessageHeader.payloadLength - l_TotalRecv;
            if (l_DataLeft == 0)
                break;
            
            if (l_TotalRecv + l_DataLeft > s_MessageBufferSize)
            {
                WriteLog(LL_Error, "attempted to write out of the bounds of what data we had left.");
                break;
            }

            l_Ret = krecv_t(s_Connection->GetSocket(), s_Connection->m_MessageBuffer + l_TotalRecv, l_DataLeft, 0, s_MainThread);
            if (l_Ret <= 0)
            {
                WriteLog(LL_Error, "could not recv all data err: (%d).", l_Ret);
                break;
            }

            l_TotalRecv += l_Ret;
        }

        // Validate that we have gotten all data
        if (l_TotalRecv != l_MessageHeader.payloadLength)
        {
            WriteLog(LL_Error, "did not recv all of payload wanted (%d) got (%d).", l_MessageHeader.payloadLength, l_TotalRecv);
            break;
        }

        // Send out message
        Messaging::Message l_Message = 
        {
            .Header = l_MessageHeader,
            .Buffer = s_Connection->m_MessageBuffer
        };
        
        auto s_Framework = Mira::Framework::GetFramework();
        if (s_Framework == nullptr)
        {
            WriteLog(LL_Error, "could not get mira framework.");
            break;
        }

        auto s_MessageManager = s_Framework->GetMessageManager();
        if (s_MessageManager == nullptr)
        {
            WriteLog(LL_Error, "could not get message manager");
            break;
        }

        s_MessageManager->OnRequest(s_Connection, l_Message);
    }

    s_Connection->m_Running = false;

    WriteLog(LL_Debug, "connection exiting...");

    auto s_Server = s_Connection->m_Server;
    if (s_Server != nullptr)
        s_Server->OnConnectionDisconnected(s_Connection);

    kthread_exit();
}