#pragma once
#include <Utils/SharedPtr.hpp>
#include <Utils/Vector.hpp>

#include "MessageCategory.hpp"

namespace Mira
{
    namespace Messaging
    {
        class MessageListener;
        class Message;

        enum
        {
            MessageManager_MaxCategories = 14
        };

        class MessageManager
        {
        private:
            Vector<Messaging::MessageListener> m_Listeners;
            struct mtx m_ListenersLock;

        public:
            MessageManager();
            ~MessageManager();

            bool RegisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>));
            bool UnregisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(shared_ptr<Messaging::Message>));
            void UnregisterAllCallbacks();

            void SendErrorResponse(MessageCategory p_Category, int32_t p_Error);
            void SendResponse(shared_ptr<Messaging::Message> p_Message);

            void OnRequest(shared_ptr<Messaging::Message> p_Message);
        };
    }
}