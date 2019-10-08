#include "MessageManager.hpp"
#include "MessageCategory.hpp"
#include "MessageListener.hpp"
#include "Message.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>

#include "Rpc/Server.hpp"
#include "Rpc/Connection.hpp"

#include <Mira.hpp>

using namespace Mira::Messaging;

MessageManager::MessageManager()
{
    for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
        m_Listeners[i].Zero();
}

MessageManager::~MessageManager()
{

}

bool MessageManager::RegisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const Messaging::Message&))
{
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
    int32_t s_FreeIndex = -1;
    for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
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
        return false;
    }

    if (s_FreeIndex < 0 || s_FreeIndex > ARRAYSIZE(m_Listeners))
    {
        WriteLog(LL_Error, "no free index");
        return false;
    }
    
    //WriteLog(LL_Warn, "freeIndex: %d", s_FreeIndex);

    m_Listeners[s_FreeIndex] = MessageListener(p_Category, p_Type, p_Callback);

    s_FoundEntry = &m_Listeners[s_FreeIndex];
    //WriteLog(LL_Warn, "reset entry: %p, cat: %d, type: %x cb: %p", s_FoundEntry, s_FoundEntry->GetCategory(), s_FoundEntry->GetType(), s_FoundEntry->GetCallback());
    return true;
}

bool MessageManager::UnregisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const Messaging::Message&))
{
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
        return false;
    }


    //WriteLog(LL_Warn, "foundIndex: %d", s_FoundIndex);
    //WriteLog(LL_Warn, "reset entry: %p, cat: %d, type: %x cb: %p", s_FoundEntry, s_FoundEntry->GetCategory(), s_FoundEntry->GetType(), s_FoundEntry->GetCallback());

    // Reset
    s_FoundEntry->Zero();

    return true;
}

void MessageManager::SendErrorResponse(Rpc::Connection* p_Connection, MessageCategory p_Category, int32_t p_Error)
{
    if (p_Connection == nullptr)
        return;

    Messaging::Message s_Message = {
        .Header = 
        {
            .magic = MessageHeaderMagic,
            .category = p_Category,
            .isRequest = false,
            .errorType = static_cast<uint64_t>(p_Error > 0 ? -p_Error : p_Error),
            .payloadLength = 0,
            .padding = 0
        },
        .Buffer = nullptr
    };

    SendResponse(p_Connection, s_Message);
}

void MessageManager::SendResponse(Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Connection == nullptr)
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
    auto s_Socket = p_Connection->GetSocket();
    if (s_Socket < 0)
    {
        WriteLog(LL_Error, "invalid socket (%d).", s_Socket);
        return;
    }

    // Get and send the header data
    auto s_Ret = kwrite(s_Socket, &p_Message.Header, sizeof(p_Message.Header));
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not send header data (%p).", &p_Message.Header);
        return;
    }

    // Send the payload
    auto s_Data = p_Message.Buffer;
    auto s_DataLength = p_Message.Header.payloadLength;

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

void MessageManager::OnRequest(Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    MessageListener* s_FoundEntry = nullptr;
    int32_t s_FoundIndex = -1;
    for (auto i = 0; i < ARRAYSIZE(m_Listeners); ++i)
    {
        auto l_CategoryEntry = &m_Listeners[i];
        if (l_CategoryEntry == nullptr)
            continue;
                    
        if (l_CategoryEntry->GetCategory() != p_Message.Header.category)
            continue;
        
        if (l_CategoryEntry->GetType() != p_Message.Header.errorType)
            continue;
        
        s_FoundEntry = &m_Listeners[i];
        s_FoundIndex = i;
        break;
    }

    if (s_FoundEntry == nullptr || s_FoundIndex == -1)
    {
        WriteLog(LL_Error, "could not find the right shit");
        return;
    }

    //WriteLog(LL_Warn, "foundIndex: %d", s_FoundIndex);
    //WriteLog(LL_Warn, "call entry: %p, cat: %d, type: %x cb: %p", s_FoundEntry, s_FoundEntry->GetCategory(), s_FoundEntry->GetType(), s_FoundEntry->GetCallback());

    if (s_FoundEntry->GetCallback())
        s_FoundEntry->GetCallback()(p_Connection, p_Message);
}