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


using namespace Mira::Messaging::Rpc;

Connection::Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address) :
    m_Socket(p_Socket),
    m_Id(p_ClientId),
    m_Running(false),
    m_Thread(nullptr),
    m_Address{0},
    m_Server(p_Server)
{
    memcpy(&m_Address, &p_Address, sizeof(m_Address));
}

Connection::~Connection()
{
    
}

void Connection::Disconnect()
{
    if (m_Running)
        m_Running = false;
    
    if (m_Socket > 0)
    {
        kclose(m_Socket);
        m_Socket = -1;
    }
}

void Connection::ConnectionThread(void* p_Connection)
{
    auto s_Connection = reinterpret_cast<Rpc::Connection*>(p_Connection);
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

    // Validate connection
    if (s_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid connection instance");
        kthread_exit();
        return;
    }

    // Allocate our new buffer
    auto s_Buffer = Span<uint8_t>(0x10000);

    WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%x), thread: (%p).", 
                    s_Connection->m_Socket, 
                    s_Connection->m_Address.sin_addr.s_addr, 
                    s_Connection->m_Thread);

    // TODO: Thread escape
    s_Connection->m_Running = true;

    // Get the header size
    const auto s_MessageHeaderSize = sizeof(Messaging::MessageHeader);
    
    while (s_Connection->IsRunning())
    {
        // Zero out our entire buffer for a new message
        s_Buffer.zero();
        s_Buffer.setOffset(0);

        // Recv the data length of this message
        ssize_t l_Ret = krecv(s_Connection->GetSocket(), s_Buffer.data(), s_MessageHeaderSize, 0);
        if (l_Ret < 0)
        {
            WriteLog(LL_Error, "recv returned (%d).", l_Ret);
            break;
        }

        if (l_Ret != s_MessageHeaderSize)
        {
            WriteLog(LL_Error, "could not get message header.");
            break;
        }

        // Parse out a message header
        auto s_Header = s_Buffer.get_struct<Messaging::MessageHeader>();
        if (s_Header == nullptr)
        {
            WriteLog(LL_Error, "could not get the message header");
            break;
        }

        // Validate the header magic
        if (s_Header->magic != MessageHeader_Magic)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", s_Header->magic, MessageHeader_Magic);
            break;
        }

        // Validate message category
        auto s_Category = static_cast<MessageCategory>(s_Header->category);
        if (s_Category < MessageCategory_None || s_Category > MessageCategory_Max)
        {
            WriteLog(LL_Error, "invalid category (%d).", s_Category);
            break;
        }

        // We do not want to handle any responses
        if (s_Header->isRequest == false)
        {
            WriteLog(LL_Error, "attempted to handle outgoing message, fix ya code");
            break;
        }

        // Bounds check the span
        if (s_Header->payloadLength > s_Buffer.getRemainingBytes())
        {
            WriteLog(LL_Error, "span does not have enough data, wanted (%d), have (%d).", s_Header->payloadLength, s_Buffer.getRemainingBytes());
            break;
        }

        uint32_t l_TotalRecv = 0;
        l_Ret = krecv(s_Connection->GetSocket(), s_Buffer.dataAtOffset(), s_Header->payloadLength, 0);
        if (l_Ret <= 0)
        {
            WriteLog(LL_Error, "could not recv data err: (%d).", l_Ret);
            break;
        }

        l_TotalRecv += l_Ret;
        while (l_TotalRecv < s_Header->payloadLength)
        {
            auto l_DataLeft = s_Header->payloadLength - l_TotalRecv;
            if (l_DataLeft == 0)
                break;
            
            l_Ret = krecv(s_Connection->GetSocket(), s_Buffer.dataAtOffset() + l_TotalRecv, l_DataLeft, 0);
            if (l_Ret <= 0)
            {
                WriteLog(LL_Error, "could not recv all data err: (%d).", l_Ret);
                break;
            }

            l_TotalRecv += l_Ret;
        }

        // Validate that we have gotten all data
        if (l_TotalRecv != s_Header->payloadLength)
        {
            WriteLog(LL_Error, "did not recv all of payload wanted (%d) got (%d).", s_Header->payloadLength, l_TotalRecv);
            break;
        }

        // TODO: Send out message
        auto l_Message = Message::Create(s_Header->payloadLength, static_cast<MessageCategory>(s_Header->category), s_Header->errorType, s_Header->isRequest);
        if (!l_Message)
        {
            WriteLog(LL_Error, "could not allocate message.");
            break;
        }

        if (l_Message->GetPayloadLength() != s_Header->payloadLength)
        {
            WriteLog(LL_Error, "idk how you even got here");
            continue;
        }

        // Copy over our payload
        memcpy(l_Message->GetPayloadData(), s_Buffer.dataAtOffset(), s_Header->payloadLength);

        auto s_Framework = Mira::Framework::GetFramework();
        if (s_Framework == nullptr)
        {
            WriteLog(LL_Error, "could not get mira framework.");
            continue;
        }

        auto s_MessageManager = s_Framework->GetMessageManager();
        if (s_MessageManager == nullptr)
        {
            WriteLog(LL_Error, "could not get message manager");
            continue;
        }

        s_MessageManager->OnRequest(l_Message);
    }

    s_Connection->m_Running = false;

    auto s_Server = s_Connection->m_Server;
    if (s_Server == nullptr)
        s_Server->OnConnectionDisconnected(s_Server, s_Connection);

    kthread_exit();
}