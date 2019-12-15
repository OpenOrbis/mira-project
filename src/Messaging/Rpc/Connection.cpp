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

    WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%x), thread: (%p).", 
                    s_Connection->m_Socket, 
                    s_Connection->m_Address.sin_addr.s_addr, 
                    s_Connection->m_Thread);

    s_Connection->m_Running = true;

    // Get the header size
    const auto s_MessageHeaderSize = sizeof(Messaging::MessageHeader);
    // Zero out our entire buffer for a new message
    memset(s_Connection->m_MessageBuffer, 0, MaxBufferSize);
    Messaging::MessageHeader s_MessageHeader = { 0 };

    ssize_t s_Ret = -1;
    while ((s_Ret = krecv_t(s_Connection->GetSocket(), &s_MessageHeader, sizeof(s_MessageHeader), 0, s_MainThread)) > 0)
    {
        // Make sure that we got all of the data
        if (s_Ret != s_MessageHeaderSize)
        {
            WriteLog(LL_Error, "could not get message header.");
            break;
        }

        // Validate the header magic
        if (s_MessageHeader.magic != MessageHeaderMagic)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", s_MessageHeader.magic, MessageHeaderMagic);
            break;
        }

        // Validate message category
        auto s_Category = static_cast<MessageCategory>(s_MessageHeader.category);
        if (s_Category < MessageCategory_None || s_Category > MessageCategory_Max)
        {
            WriteLog(LL_Error, "invalid category (%d).", s_Category);
            break;
        }

        // We do not want to handle any responses
        if (s_MessageHeader.isRequest == false)
        {
            WriteLog(LL_Error, "attempted to handle outgoing message, fix ya code");
            break;
        }

        // Bounds check the span
        if (s_MessageHeader.payloadLength > MaxBufferSize)
        {
            WriteLog(LL_Error, "span does not have enough data, wanted (%d), have (%d).", s_MessageHeader.payloadLength, MaxBufferSize);
            break;
        }

        uint32_t l_TotalRecv = 0;
        memset(s_Connection->m_MessageBuffer, 0, MaxBufferSize); // Zero out the buffer
        s_Ret = krecv_t(s_Connection->GetSocket(), s_Connection->m_MessageBuffer, s_MessageHeader.payloadLength, 0, s_MainThread);
        if (s_Ret <= 0)
        {
            WriteLog(LL_Error, "could not recv data err: (%d).", s_Ret);
            break;
        }

        l_TotalRecv += s_Ret;
        while (l_TotalRecv < s_MessageHeader.payloadLength)
        {
            auto l_DataLeft = s_MessageHeader.payloadLength - l_TotalRecv;
            if (l_DataLeft == 0)
                break;
            
            if (l_TotalRecv + l_DataLeft > MaxBufferSize)
            {
                WriteLog(LL_Error, "attempted to write out of the bounds of what data we had left.");
                break;
            }

            s_Ret = krecv_t(s_Connection->GetSocket(), s_Connection->m_MessageBuffer + l_TotalRecv, l_DataLeft, 0, s_MainThread);
            if (s_Ret <= 0)
            {
                WriteLog(LL_Error, "could not recv all data err: (%d).", s_Ret);
                break;
            }

            l_TotalRecv += s_Ret;
        }

        // Validate that we have gotten all data
        if (l_TotalRecv != s_MessageHeader.payloadLength)
        {
            WriteLog(LL_Error, "did not recv all of payload wanted (%d) got (%d).", s_MessageHeader.payloadLength, l_TotalRecv);
            break;
        }

        // Send out message
        Messaging::Message l_Message = 
        {
            .Header = s_MessageHeader,
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

        // Zero out the header for use next iteration
        memset(&s_MessageHeader, 0, sizeof(s_MessageHeader));
    }

    s_Connection->Disconnect();

    kthread_exit();
}