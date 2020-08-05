#pragma once
#include <External/flatbuffers/rpc_generated.h>

#include <functional>
#include <memory>

namespace Mira
{
    namespace Rpc
    {
        class Connection;

        class Listener
        {
        private:
            RpcCategory m_Category;
            uint32_t m_Type;
            std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<RpcHeader>)> m_Callback;

        public:
            Listener(RpcCategory p_Category, uint32_t p_Type, std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<RpcHeader>)> p_Callback) :
                m_Category(p_Category),
                m_Type(p_Type),
                m_Callback(p_Callback)
            {

            }

            ~Listener()
            {
                m_Category = RpcCategory_NONE;
                m_Type = 0;
                m_Callback = nullptr;
            }

            RpcCategory GetCategory() const { return m_Category; }
            uint32_t GetType() const { return m_Type; }
            auto GetCallback() const { return m_Callback; }
        };
    }
}