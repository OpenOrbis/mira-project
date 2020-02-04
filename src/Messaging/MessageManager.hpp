#pragma once
#include "MessageListener.hpp"

extern "C"
{
    #include "Rpc/rpc.pb-c.h"
}

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Connection;
        }

        enum
        {
            MessageManager_MaxCategories = 14,
            MessageManager_MaxListeners = 10
        };

        class MessageManager
        {
        private:
            Messaging::MessageListener m_Listeners[MessageManager_MaxListeners];

        public:
            MessageManager();
            ~MessageManager();

            bool RegisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&));
            bool UnregisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&));
            void UnregisterAllCallbacks();

            void SendErrorResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, int32_t p_Error);

            void SendResponse(Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            void OnRequest(Rpc::Connection* p_Connection, const RpcTransport& p_Message);
        };
    }
}