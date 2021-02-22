#pragma once
#include <vector>
#include <cstddef>
#include <limits>
#include <mutex>

namespace Mira
{
    namespace Rpc
    {
        struct RpcHeader;
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
            std::mutex m_Lock;

        public:
            Manager();
            virtual ~Manager();

            bool Register(RpcCategory p_Category, uint32_t p_Type, std::function<void(Rpc::Connection*, const Rpc::RpcHeader*)>);
            bool Unregister(RpcCategory p_Category, uint32_t p_Type, std::function<void(Rpc::Connection*, const Rpc::RpcHeader*)>);

            void Clear();

            void SendErrorResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, int32_t p_Error);
            
            void SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, std::vector<uint8_t> p_Data);

            void OnRequest(Rpc::Connection* p_Connection, const Mira::Rpc::RpcHeader* p_Message);
        };
    }
}