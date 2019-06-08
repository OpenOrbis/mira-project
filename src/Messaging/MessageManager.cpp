#include "MessageManager.hpp"
#include "MessageCategory.hpp"
#include "MessageCategoryEntry.hpp"
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
    mtx_init(&m_CategoriesLock, "MsgMgr", nullptr, 0);
}

MessageManager::~MessageManager()
{

}

bool MessageManager::RegisterCallback(MessageCategory p_Category, uint32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>))
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

    MessageCategoryEntry* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Categories.size(); ++i)
    {
        auto& l_CategoryEntry = m_Categories[i];
        if (l_CategoryEntry.GetCategory() != p_Category)
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);

    // Check if this category entry exists already
    if (s_FoundEntry == nullptr)
    {
        // This category has not been added to our list yet add it
        auto s_CatEntry = Messaging::MessageCategoryEntry(p_Category);
        if (!s_CatEntry.AddCallback(p_Type, p_Callback))
        {
            WriteLog(LL_Error, "could not add callback c:(%d), t:(%d) cb:(%p).", p_Category, p_Type, p_Callback);
            return false;
        }

        _mtx_lock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);
        m_Categories.push_back(s_CatEntry);
        _mtx_unlock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);
    }
    else // Category exist add the callback
    {
        if (s_FoundEntry->GetCallbackByType(p_Type) != nullptr)
        {
            WriteLog(LL_Error, "callback exists c: (%d) t:(%d) cb: (%p).", p_Category, p_Type, p_Callback);
            return false;
        }

        if (!s_FoundEntry->AddCallback(p_Type, p_Callback))
        {
            WriteLog(LL_Error, "could not add callback c:(%d), t:(%d) cb:(%p).", p_Category, p_Type, p_Callback);
            return false;
        }
    }
    
    return true;
}

bool MessageManager::UnregisterCallback(MessageCategory p_Category, uint32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>))
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

    MessageCategoryEntry* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Categories.size(); ++i)
    {
        auto& l_CategoryEntry = m_Categories[i];
        if (l_CategoryEntry.GetCategory() != p_Category)
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);

    if (s_FoundEntry == nullptr)
    {
        WriteLog(LL_Error, "category entry not found for cat: (%d).", p_Category);
        return false;
    }

    if (!s_FoundEntry->RemoveCallback(p_Type, p_Callback))
    {
        WriteLog(LL_Error, "could not remove the callback for type (%d) (%p).", p_Type, p_Callback);
        return false;
    }

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
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }
    auto s_Category = p_Message->GetCategory();
    if (s_Category < MessageCategory_None || s_Category >= MessageCategory_Max)
    {
        WriteLog(LL_Error, "invalid category (%d).", s_Category);
        return;
    }

    auto s_CategoryEntry = GetCategory(static_cast<MessageCategory>(s_Category));
    if (s_CategoryEntry == nullptr)
    {
        WriteLog(LL_Error, "no category found (%d).", s_Category);
        return;
    }

    auto s_Callback = s_CategoryEntry->GetCallbackByType(p_Message->GetErrorType());
    if (s_Callback == nullptr)
    {
        WriteLog(LL_Error, "no callback found for type (%d).", p_Message->GetErrorType());
        return;
    }

    s_Callback(p_Message);
}

MessageCategoryEntry* MessageManager::GetCategory(MessageCategory p_Category)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    
    if (p_Category < MessageCategory_None || p_Category >= MessageCategory_Max)
        return nullptr;

    MessageCategoryEntry* s_FoundEntry = nullptr;
    _mtx_lock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);
    for (auto i = 0; i < m_Categories.size(); ++i)
    {
        auto& l_CategoryEntry = m_Categories[i];
        if (l_CategoryEntry.GetCategory() != p_Category)
            continue;
        
        s_FoundEntry = &l_CategoryEntry;
        break;
    }
    _mtx_unlock_flags(&m_CategoriesLock, 0, __FILE__, __LINE__);

    return s_FoundEntry;
}