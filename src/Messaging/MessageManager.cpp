#include "MessageManager.hpp"
#include "MessageCategory.hpp"
#include "MessageListener.hpp"
#include "Message.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>

#include "Rpc/Server.hpp"

#include <Mira.hpp>

using namespace Mira::Messaging;

MessageManager::MessageManager()
{
    // Initialize the mutex
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
    mtx_init(&m_ListenersLock, "MsgMgr", nullptr, 0);
}

MessageManager::~MessageManager()
{

}

bool MessageManager::RegisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>))
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (p_Category < MessageCategory_None || p_Category >= MessageCategory_Max)
    {
        WriteLog(LL_Error, "could not register callback, invalid category (%d).", p_Category);
        return false;
    }

    if (p_Callback == nullptr)
    {
        WriteLog(LL_Error, "invalid callback.");
        return false;
    }

    MessageListener* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Listeners.size(); ++i)
    {
        auto& l_CategoryEntry = m_Listeners[i];
        if (l_CategoryEntry.GetCategory() != p_Category)
            continue;
        
        if (l_CategoryEntry.GetType() != p_Type)
            continue;
        
        if (l_CategoryEntry.GetCallback() != p_Callback)
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);

    if (s_FoundEntry != nullptr)
    {
        WriteLog(LL_Error, "callback already exists");
        return false;
    }

    _mtx_lock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);
    m_Listeners.push_back(MessageListener(p_Category, p_Type, p_Callback));
    _mtx_unlock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);

    
    return true;
}

bool MessageManager::UnregisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>))
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    if (p_Category < MessageCategory_None || p_Category >= MessageCategory_Max)
    {
        WriteLog(LL_Error, "could not register callback, invalid category (%d).", p_Category);
        return false;
    }

    if (p_Callback == nullptr)
    {
        WriteLog(LL_Error, "invalid callback.");
        return false;
    }

    MessageListener* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Listeners.size(); ++i)
    {
        auto& l_CategoryEntry = m_Listeners[i];
        if (l_CategoryEntry.GetCategory() != p_Category)
            continue;
    
        if (l_CategoryEntry.GetType() != p_Type)
            continue;
        
        if (l_CategoryEntry.GetCallback() != p_Callback)
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);

    if (s_FoundEntry == nullptr)
    {
        WriteLog(LL_Error, "category entry not found for cat: (%d).", p_Category);
        return false;
    }

    s_FoundEntry->Zero();

    return true;
}

void MessageManager::SendErrorResponse(MessageCategory p_Category, int32_t p_Error)
{
    auto s_Message = Message::Create(0, p_Category, p_Error, false);

    SendResponse(s_Message);
}

void MessageManager::SendResponse(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
        return;

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
    auto s_Socket = s_RpcServer->Deprecated_GetSocketByThread(curthread);
    if (s_Socket < 0)
    {
        WriteLog(LL_Error, "invalid socket (%d).", s_Socket);
        return;
    }

    // Get and send the header data
    auto s_HeaderData = p_Message->GetHeader();
    auto s_Ret = kwrite(s_Socket, s_HeaderData, sizeof(*s_HeaderData));
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not send header data (%p).", s_HeaderData);
        return;
    }

    // Send the payload
    auto s_Data = p_Message->GetPayloadData();
    auto s_DataLength = p_Message->GetPayloadLength();

    // If there is no payload then we don't send anything
    if (s_Data == nullptr || s_DataLength == 0)
        return;
    
    s_Ret = kwrite(s_Socket, s_Data, s_DataLength);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not send payloa data (%p) len (%d).", s_Data, s_DataLength);
        return;
    }
}

void MessageManager::OnRequest(shared_ptr<Messaging::Message> p_Message)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }
    //WriteLog(LL_Debug, "Message: c: (%d) t: (%d).", p_Message->GetCategory(), p_Message->GetErrorType());

    MessageListener* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Listeners.size(); ++i)
    {
        auto& l_CategoryEntry = m_Listeners[i];
        if (l_CategoryEntry.GetCategory() != p_Message->GetCategory())
            continue;
        
        if (l_CategoryEntry.GetType() != p_Message->GetErrorType())
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_ListenersLock, 0, __FILE__, __LINE__);

    if (s_FoundEntry == nullptr)
    {
        WriteLog(LL_Error, "could not find the right shit");
        return;
    }

    //WriteLog(LL_Debug, "calling callback (%p).", s_FoundEntry->GetCallback());
    s_FoundEntry->GetCallback()(p_Message);
}