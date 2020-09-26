#pragma once
#include "MessageListener.hpp"

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
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
            MessageManager_MaxListeners = 64,
            MessageManager_MaxMessageSize = 0x4000000,
        };

        class MessageManager
        {
        private:
            Messaging::MessageListener m_Listeners[MessageManager_MaxListeners];
            struct mtx m_Mutex;

        public:
            MessageManager();
            ~MessageManager();

            bool RegisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport*));
            bool UnregisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport*));
            void UnregisterAllCallbacks();

            void SendErrorResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, int32_t p_Error);

            void SendResponse(Rpc::Connection* p_Connection, const RpcTransport* p_Message);
            void SendResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, void* p_Data, uint32_t p_DataSize);
            
            void OnRequest(Rpc::Connection* p_Connection, const RpcTransport* p_Message);
        };
    }
}