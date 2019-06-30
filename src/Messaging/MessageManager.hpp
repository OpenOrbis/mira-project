#pragma once
#include <Utils/SharedPtr.hpp>
#include <Utils/Vector.hpp>

#include "MessageCategory.hpp"

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Connection;
        }
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

            bool RegisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, shared_ptr<Messaging::Message>));
            bool UnregisterCallback(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, shared_ptr<Messaging::Message>));
            void UnregisterAllCallbacks();

            void SendErrorResponse(Rpc::Connection* p_Connection, MessageCategory p_Category, int32_t p_Error);
            void SendResponse(Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);

            void OnRequest(Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
        };
    }
}