#include "MessageCategoryEntry.hpp"
#include <Utils/SharedPtr.hpp>

using namespace Mira::Messaging;

MessageCategoryEntry::MessageCategoryEntry(MessageCategory p_Category) :
    m_Category(p_Category)
{

}

MessageCategoryEntry::~MessageCategoryEntry()
{
    m_Category = MessageCategory_None;
    m_Callbacks.clear();
}

bool MessageCategoryEntry::AddCallback(uint32_t p_Type, void(*p_Callback)(shared_ptr<Message> p_Message))
{
    if (p_Callback == nullptr)
    {
        WriteLog(LL_Error, "invalid callback");
        return false;
    }

    // Check to see if a callback has already been added
    for (auto i = 0; i < m_Callbacks.size(); ++i)
    {
        auto& l_Callback = m_Callbacks[i];
        if (p_Type != l_Callback.GetType())
            continue;
        
        if (l_Callback.GetCallback() != p_Callback)
            continue;
        
        return false;
    }

    m_Callbacks.push_back(MessageCategoryCallback(p_Type, p_Callback));

    return true;
}

bool MessageCategoryEntry::RemoveCallback(uint32_t p_Type, void(*p_Callback)(shared_ptr<Message> p_Message))
{
    // Validate the callback
    if (p_Callback == nullptr)
    {
        WriteLog(LL_Error, "invalid callback");
        return false;
    }

    // Check to see if a callback has already been added
    for (auto i = 0; i < m_Callbacks.size(); ++i)
    {
        auto& l_Callback = m_Callbacks[i];
        if (p_Type != l_Callback.GetType())
            continue;
        
        if (l_Callback.GetCallback() != p_Callback)
            continue;
        
        l_Callback = MessageCategoryCallback();
        return true;
    }

    return false;
}