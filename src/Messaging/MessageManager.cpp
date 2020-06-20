// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MessageManager.hpp"
#include "MessageListener.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kernel.hpp>

#include "Rpc/Server.hpp"
#include "Rpc/Connection.hpp"

#include <Mira.hpp>

using namespace Mira::Messaging;

MessageManager::MessageManager()
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    // Create the message manager
	mtx_init(&m_Mutex, "MiraMM", nullptr, MTX_DEF);

    _mtx_lock_flags(&m_Mutex, 0);
    for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
        m_Listeners[i].Zero();
    _mtx_unlock_flags(&m_Mutex, 0);
}

MessageManager::~MessageManager()
{

}

bool MessageManager::RegisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&))
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    bool s_Success = false;
    _mtx_lock_flags(&m_Mutex, 0);
    do
    {
        if (p_Category < RPC_CATEGORY__NONE || p_Category >= RPC_CATEGORY__MAX)
        {
            WriteLog(LL_Error, "could not register callback, invalid category (%d).", p_Category);
            break;
        }

        if (p_Callback == nullptr)
        {
            WriteLog(LL_Error, "invalid callback.");
            break;
        }

        MessageListener* s_FoundEntry = nullptr;
        int32_t s_FreeIndex = -1;

        auto s_ListenerCount = ARRAYSIZE(m_Listeners);
        for (auto i = 0; i < s_ListenerCount; ++i)
        {
            auto& l_CategoryEntry = m_Listeners[i];
            
            if (!l_CategoryEntry.GetCallback() && 
                !l_CategoryEntry.GetCategory() && 
                !l_CategoryEntry.GetType())
            {
                s_FreeIndex = i;
                continue;
            }

            if (l_CategoryEntry.GetCategory() != p_Category)
                continue;
            
            if (l_CategoryEntry.GetType() != p_Type)
                continue;
            
            if (l_CategoryEntry.GetCallback() != p_Callback)
                continue;
            
            s_FoundEntry = &m_Listeners[i];
            break;
        }

        if (s_FoundEntry != nullptr)
        {
            WriteLog(LL_Error, "callback already exists");
            break;
        }

        if (s_FreeIndex < 0 || s_FreeIndex >= ARRAYSIZE(m_Listeners))
        {
            WriteLog(LL_Error, "no free index");
            break;
        }

        m_Listeners[s_FreeIndex] = MessageListener(p_Category, p_Type, p_Callback);
        s_Success = true;
    } while (false);
    _mtx_unlock_flags(&m_Mutex, 0);

    return s_Success;
}

bool MessageManager::UnregisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&))
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    bool s_Success = false;
    _mtx_lock_flags(&m_Mutex, 0);
    do
    {
        if (p_Category < RPC_CATEGORY__NONE || p_Category >= RPC_CATEGORY__MAX)
        {
            WriteLog(LL_Error, "could not register callback, invalid category (%d).", p_Category);
            break;
        }

        if (p_Callback == nullptr)
        {
            WriteLog(LL_Error, "invalid callback.");
            break;
        }

        MessageListener* s_FoundEntry = nullptr;
        int32_t s_FoundIndex = -1;

        for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
        {
            auto& l_CategoryEntry = m_Listeners[i];
            if (l_CategoryEntry.GetCategory() != p_Category)
                continue;
        
            if (l_CategoryEntry.GetType() != p_Type)
                continue;
            
            if (l_CategoryEntry.GetCallback() != p_Callback)
                continue;
            
            s_FoundEntry = &m_Listeners[i];
            s_FoundIndex = i;
            break;
        }

        if (s_FoundEntry == nullptr || s_FoundIndex == -1)
        {
            WriteLog(LL_Error, "category entry not found for cat: (%d).", p_Category);
            break;
        }

        // Reset
        s_FoundEntry->Zero();
        s_Success = true;
    } while (false);
    _mtx_unlock_flags(&m_Mutex, 0);

    return s_Success;
}

void MessageManager::SendErrorResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, int32_t p_Error)
{
    if (p_Connection == nullptr)
        return;

    SendResponse(p_Connection, p_Category, 0,  p_Error < 0 ? p_Error : (-p_Error), nullptr, 0);
}

void MessageManager::SendResponse(Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Connection == nullptr)
        return;

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return;
    }

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework");
        return;
    }

    auto s_RpcServer = s_Framework->GetRpcServer();
    if (s_RpcServer == nullptr)
    {
        WriteLog(LL_Error, "could not get rpc server.");
        return;
    }

    // Get the socket for this thread
    auto s_Socket = p_Connection->GetSocket();
    if (s_Socket < 0)
    {
        WriteLog(LL_Error, "invalid socket (%d).", s_Socket);
        return;
    }

    WriteLog(LL_Error, "here");

    auto s_SerializedSize = rpc_transport__get_packed_size(&p_Message);
    if (s_SerializedSize <= 0)
    {
        WriteLog(LL_Error, "invalid serialized size (%x)", s_SerializedSize);
        return;
    }

    WriteLog(LL_Error, "here");

    if (s_SerializedSize > MessageManager_MaxMessageSize)
    {
        WriteLog(LL_Error, "serialized size too large (%x) > (%llx)", s_SerializedSize, MessageManager_MaxMessageSize);
        return;
    }

    WriteLog(LL_Error, "here");

    // Allocate enough space for size + message data
    auto s_TotalSize = sizeof(uint64_t) + s_SerializedSize;
    auto s_SerializedData = new uint8_t[s_TotalSize];
    if (s_SerializedData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%x).", s_SerializedSize);
        return;
    }
    memset(s_SerializedData, 0, s_TotalSize);

    WriteLog(LL_Error, "here");

    // Set the message size
    *(uint64_t*)s_SerializedData = s_SerializedSize;

    WriteLog(LL_Error, "here");

    // Get the message start
    auto s_MessageStart = s_SerializedData + sizeof(uint64_t);

    WriteLog(LL_Error, "here");

    // Pack the message
    auto s_Ret = rpc_transport__pack(&p_Message, s_MessageStart);
    if (s_Ret != s_SerializedSize)
    {
        WriteLog(LL_Error, "could not serialize data");
        memset(s_SerializedData, 0, s_TotalSize);
        delete [] s_SerializedData;
        return;
    }
    //WriteLog(LL_Debug, "packed message size (%x)", s_Ret);

    WriteLog(LL_Error, "here");

    // Get and send the data
    auto s_BytesWritten = kwrite_t(s_Socket, s_SerializedData, s_TotalSize, s_MainThread);
    if (s_BytesWritten < 0)
    {
        WriteLog(LL_Error, "could not send data (%lld).", s_BytesWritten);
        memset(s_SerializedData, 0, s_TotalSize);
        delete [] s_SerializedData;
        return;
    }

    WriteLog(LL_Error, "here");
    memset(s_SerializedData, 0, s_TotalSize);
    delete [] s_SerializedData;
}

void MessageManager::SendResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, void* p_Data, uint32_t p_DataSize)
{
    if (p_Connection == nullptr)
    {
        WriteLog(LL_Error, "invalid connection");
        return;
    }

    if (p_Category < RPC_CATEGORY__NONE || p_Category >= RPC_CATEGORY__MAX)
    {
        WriteLog(LL_Error, "invalid category (%d)", p_Category);
        return;
    }

    RpcHeader s_Header = RPC_HEADER__INIT;
    s_Header.category = p_Category;
    s_Header.type = p_Type;
    s_Header.error = p_Error;
    s_Header.isrequest = false;
    s_Header.magic = 2;

    RpcTransport s_Response = RPC_TRANSPORT__INIT;
    s_Response.header = &s_Header;

    s_Response.data.data = static_cast<uint8_t*>(p_Data);
    s_Response.data.len = p_DataSize;

    SendResponse(p_Connection, s_Response);
}

void MessageManager::OnRequest(Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    MessageListener* s_FoundEntry = nullptr;
    int32_t s_FoundIndex = -1;

    _mtx_lock_flags(&m_Mutex, 0);
    do
    {        
        for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
        {
            auto l_CategoryEntry = &m_Listeners[i];
            if (l_CategoryEntry == nullptr)
                continue;
                        
            if (l_CategoryEntry->GetCategory() != p_Message.header->category)
                continue;
            
            if (l_CategoryEntry->GetType() != p_Message.header->type)
                continue;
            
            s_FoundEntry = &m_Listeners[i];
            s_FoundIndex = i;
            break;
        }

        if (s_FoundEntry == nullptr || s_FoundIndex == -1)
        {
            WriteLog(LL_Error, "could not find endpoint c: (%x) t:(%x)", p_Message.header->category, p_Message.header->type);
            break;
        }
        
    } while (false);
    _mtx_unlock_flags(&m_Mutex, 0);
    
    if (s_FoundEntry == nullptr)
        return;
    
    if (s_FoundEntry->GetCallback())
        s_FoundEntry->GetCallback()(p_Connection, p_Message);
}