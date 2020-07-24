#pragma once
#include <External/flatbuffers/rpc_generated.h>
#include <vector>
#include <cstddef>
#include <limits>

namespace Mira
{
    namespace Rpc
    {
        class Connection;
        class Listener;

        class Manager
        {
        private:
            enum
            {
                Manager_MaxCategories = std::numeric_limits<uint32_t>::max() - 1,
                Manager_MaxListeners = 64,

                // Maximum message size (4MB)
                Manager_MaxMessageSize = 0x4000000,
            };

            std::vector<std::shared_ptr<Rpc::Listener>> m_Listeners;
            // TODO: mutex

        public:
            Manager();
            ~Manager();

            bool Register(RpcCategory p_Category, uint32_t p_Type, std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<Rpc::RpcHeader>)>);
            bool Unregister(RpcCategory p_Category, uint32_t p_Type, std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<Rpc::RpcHeader>)>);

            void Clear();

            void SendErrorResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, int32_t p_Error);
            
            void SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, std::shared_ptr<Rpc::RpcHeader> p_Message);
            void SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, std::vector<uint8_t> p_Data);

            void OnRequest(std::shared_ptr<Rpc::Connection> p_Connection, std::shared_ptr<Rpc::RpcHeader> p_Message);
        };
    }
}