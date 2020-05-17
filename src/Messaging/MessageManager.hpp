#pragma once
#include "MessageListener.hpp"

extern "C"
{
    #include "Rpc/rpc.pb-c.h"
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/sx.h>
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
            // struct sx m_Mutex;

        public:
            MessageManager();
            ~MessageManager();

            bool RegisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&));
            bool UnregisterCallback(RpcCategory p_Category, int32_t p_Type, void(*p_Callback)(Rpc::Connection*, const RpcTransport&));
            void UnregisterAllCallbacks();

            void SendErrorResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, int32_t p_Error);

            void SendResponse(Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            void SendResponse(Rpc::Connection* p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, void* p_Data, uint32_t p_DataSize);
            
            /**
             * @brief Sends raw data to the socket on the connection, this is needed because C has struct limits which break shit
             * 
             * @param p_Connection Rpc connection
             * @param p_Data Pointer to data buffer
             * @param p_Size Size of the data buffer
             */
            void SendRawResponse(Rpc::Connection* p_Connection, void* p_Data, size_t p_Size);
            
            void OnRequest(Rpc::Connection* p_Connection, const RpcTransport& p_Message);
        };
    }
}