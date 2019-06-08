#pragma once
#include <Utils/Vector.hpp>

#include "MessageCategory.hpp"
#include "MessageCategoryCallback.hpp"

namespace Mira
{
    namespace Messaging
    {
        class Message;
        enum { MessageCategoryEntry_MaxCallbacks = 256 };

        class MessageCategoryEntry
        {
        private:
            Messaging::MessageCategory m_Category;
            Vector<Messaging::MessageCategoryCallback> m_Callbacks;

        public:
            MessageCategoryEntry() :
                m_Category(MessageCategory_None)
            {

            }
            
            MessageCategoryEntry(MessageCategory p_Category);
            ~MessageCategoryEntry();

            bool AddCallback(uint32_t p_Type, void(*p_Callback)(shared_ptr<Message>));
            bool RemoveCallback(uint32_t p_Type, void(*p_Callback)(shared_ptr<Message>));

            auto GetCallbackByType(uint32_t p_Type) -> void(*)(shared_ptr<Messaging::Message>)
            {
                for (auto i = 0; i < m_Callbacks.size(); ++i)
                {
                    auto l_CategoryCallback = m_Callbacks[i];
                    if (l_CategoryCallback.GetType() != p_Type)
                        continue;
                    
                    if (l_CategoryCallback.GetCallback() == nullptr)
                        continue;
                    
                    return l_CategoryCallback.GetCallback();
                }

                return nullptr;
            }

            Messaging::MessageCategory GetCategory() { return m_Category; }
        };
    }
}