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
}


using namespace Mira::Messaging::Rpc;

Connection::Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address) :
    m_Socket(p_Socket),
    m_Id(p_ClientId),
    m_Running(false),
    m_Thread(nullptr),
    m_Address{0}//, // TOOD: Re-enable once protobuf has been replaced
    //m_Server(p_Server)
{
    memcpy(&m_Address, &p_Address, sizeof(m_Address));

    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    mtx_init(&m_Mutex, "MiraRpcConMtx", nullptr, MTX_DEF);
}

Connection::~Connection()
{
    if (m_Running)
        Disconnect();
    
    auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);
	mtx_destroy(&m_Mutex);
}

void Connection::Disconnect()
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework to disconnect.");
        return;
    }

    _mtx_lock_flags(&m_Mutex, 0);
    do
    {
        // Even if something freakish happens try to free resources
        if (m_Running)
            m_Running = false;
        
        auto s_MainThread = s_Framework->GetMainThread();
        if (s_MainThread == nullptr)
        {
            WriteLog(LL_Error, "could not get main thread");
            break;
        }

        // If the socket is in use, kill it, should break out of the threaded loop
        if (m_Socket > 0)
        {
            kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
            kclose_t(m_Socket, s_MainThread);
            m_Socket = -1;
        }

        WriteLog(LL_Debug, "client (%p) disconnecting.", this);
    } while (false);
    _mtx_unlock_flags(&m_Mutex, 0);    
}

void Connection::SetRunning(bool p_Running)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    _mtx_lock_flags(&m_Mutex, 0);
    m_Running = p_Running;
    _mtx_unlock_flags(&m_Mutex, 0);
}

void Connection::ConnectionThread(void* p_Connection)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
    /*auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
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

    auto s_Buffer = new uint8_t[MaxBufferSize];
    if (s_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate maximum buffer length.");
        kthread_exit();
        return;
    }

    WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%x), thread: (%p).", 
                    s_Connection->m_Socket, 
                    s_Connection->m_Address.sin_addr.s_addr, 
                    s_Connection->m_Thread);

    // Update the running state, this takes a lock
    s_Connection->SetRunning(true);

    auto s_Socket = s_Connection->GetSocket();

    uint64_t s_IncomingMessageSize = 0;
    ssize_t s_Ret = -1;
    while (((s_Ret = krecv_t(s_Socket, &s_IncomingMessageSize, sizeof(s_IncomingMessageSize), 0, s_MainThread)) > 0) && s_Connection->IsRunning())
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

        if (s_IncomingMessageSize == 0)
        {
            WriteLog(LL_Error, "invalid message size.");
            break;
        }

        // Zero our buffer
        memset(s_Buffer, 0, MaxBufferSize);

        // Get the message data from the wire
        s_Ret = krecv_t(s_Socket, s_Buffer, s_IncomingMessageSize, 0, s_MainThread);
        if (s_Ret != s_IncomingMessageSize)
        {
            WriteLog(LL_Error, "did not get correct amount of data (%llx) wanted (%llx)", s_Ret, s_IncomingMessageSize);
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        RpcTransport* s_Transport = rpc_transport__unpack(nullptr, s_IncomingMessageSize, s_Buffer);
        if (s_Transport == nullptr)
        {
            WriteLog(LL_Error, "error unpacking incoming message");
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        // Validate the header magic
        auto s_Header = s_Transport->header;

        if (s_Header == nullptr)
        {
            WriteLog(LL_Error, "could not get the transport header.");
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        if (s_Header->magic != 2)
        {
            WriteLog(LL_Error, "incorrect magic got(%d) wanted (%d).", s_Header->magic, 2);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        // Validate message category
        auto s_Category = s_Header->category;
        if (s_Category < RPC_CATEGORY__NONE || s_Category >= RPC_CATEGORY__MAX)
        {
            WriteLog(LL_Error, "invalid category (%d).", s_Category);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        // We do not want to handle any responses
        if (!s_Header->isrequest)
        {
            WriteLog(LL_Error, "attempted to handle outgoing message, fix ya code");
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        // Bounds check the span
        if (s_Transport->data.len > MaxBufferSize)
        {
            WriteLog(LL_Error, "transport data does not have enough data, wanted (%d), have (%d).", s_Transport->data.len, MaxBufferSize);
            rpc_transport__free_unpacked(s_Transport, nullptr);
            memset(s_Buffer, 0, MaxBufferSize);
            break;
        }

        s_MessageManager->OnRequest(s_Connection, const_cast<const RpcTransport*>(s_Transport));

        WriteLog(LL_Error, "here");

        // Free the protobuf
        rpc_transport__free_unpacked(s_Transport, nullptr);

        // Zero out the header for use next iteration
        memset(s_Buffer, 0, MaxBufferSize);
        s_IncomingMessageSize = 0;
    }

    // Free the maximum buffer size
    memset(s_Buffer, 0, MaxBufferSize);
    delete [] s_Buffer;

    WriteLog(LL_Error, "why did we get here (%d)", s_Connection->m_Running);

    // Cleans up resources
    s_Connection->Disconnect();

    // Tell the server to free this memory
    if (s_Connection != nullptr && s_Connection->m_Server != nullptr)
        s_Connection->m_Server->OnConnectionDisconnected(s_Connection);
    */
    kthread_exit();
}