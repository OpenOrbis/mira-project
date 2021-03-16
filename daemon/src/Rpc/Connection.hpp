#pragma once
#include <functional>

extern "C"
{
    #include <netinet/in.h>
}

namespace Mira
{
    namespace Rpc
    {
        class Connection
        {
        private:
            uint64_t m_Id;
            int32_t m_Socket;
            struct sockaddr_in m_Address;

            std::function<void(uint64_t)> OnDisconnectCallback;

        public:
            Connection(uint64_t p_Id, int32_t p_Socket, struct sockaddr_in& p_Address, std::function<void(uint64_t)> p_DisconnectCallback) :
                m_Id(p_Id),
                m_Socket(p_Socket),
                m_Address { 0 },
                OnDisconnectCallback(p_DisconnectCallback)
            {
                // Copy over our client address
                memcpy(&m_Address, &p_Address, sizeof(m_Address));
            }

            uint64_t GetId() const { return m_Id; }
            int32_t GetSocket() const { return m_Socket; }
            const struct sockaddr_in& GetAddress() const { return m_Address; }
        };
    }
}