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
    auto s_Buffer = Span<uint8_t>(s_Connection->m_MessageBuffer, sizeof(s_Connection->m_MessageBuffer));
    s_Buffer.zero();

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

        Messaging::MessageHeader l_MessageHeader = { 0 };

        // Recv the data length of this message
        ssize_t l_Ret = krecv(s_Connection->GetSocket(), &l_MessageHeader, sizeof(l_MessageHeader), 0);
        if (l_Ret < 0)
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
        if (l_MessageHeader.magic != MessageHeader_Magic)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", l_MessageHeader.magic, MessageHeader_Magic);
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
        if (l_MessageHeader.payloadLength > s_Buffer.size())
        {
            WriteLog(LL_Error, "span does not have enough data, wanted (%d), have (%d).", l_MessageHeader.payloadLength, s_Buffer.size());
            break;
        }

        uint32_t l_TotalRecv = 0;
        l_Ret = krecv(s_Connection->GetSocket(), s_Buffer.data(), l_MessageHeader.payloadLength, 0);
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
            
            l_Ret = krecv(s_Connection->GetSocket(), s_Buffer.data() + l_TotalRecv, l_DataLeft, 0);
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

        // TODO: Send out message
        auto l_Message = shared_ptr<Message>(new Message(l_MessageHeader.payloadLength, static_cast<MessageCategory>(l_MessageHeader.category), l_MessageHeader.errorType, l_MessageHeader.isRequest));
        if (!l_Message)
        {
            WriteLog(LL_Error, "could not allocate message.");
            break;
        }

        if (l_Message->GetPayloadLength() != l_MessageHeader.payloadLength)
        {
            WriteLog(LL_Error, "idk how you even got here");
            break;
        }

        // Copy over our payload
        memcpy(l_Message->GetPayloadData(), s_Buffer.data(), l_MessageHeader.payloadLength);

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

    auto s_Server = s_Connection->m_Server;
    if (s_Server == nullptr)
        s_Server->OnConnectionDisconnected(s_Connection);

    kthread_exit();
}