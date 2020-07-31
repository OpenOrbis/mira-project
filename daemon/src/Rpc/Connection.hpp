#pragma once
#include <cstdint>
#include <memory>

extern "C"
{
    #include <netinet/in.h>
};

#include <orbis/libkernel.h>

namespace Mira
{
    namespace Rpc
    {
        class Server;

        class Connection
        {
        private:
            enum
            {
                RpcConnection_MaxMessageSize = 0x400000,
            };

            // Client socket
            int32_t m_Socket;

            // Client id
            uint32_t m_ClientId;

            // Is the client connection still running
            volatile bool m_Running;

            // Thread
            OrbisPthread m_Thread;

            // Client address
            struct sockaddr_in m_Address;

            // Reference to the server class
            Rpc::Server* m_Server;

            // TODO: mutex

        public:
            Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address);
            ~Connection();

            void Disconnect();

            int32_t GetSocket() const { return m_Socket; }
            uint32_t GetId() const { return m_ClientId; }
            bool IsRunning() const { return m_Running; }

            struct sockaddr_in& GetAddress() { return m_Address; }
            
            OrbisPthread* GetThreadPointer() { return &m_Thread; }
        
            static void* ConnectionThread(void* p_ConnectionInstance);
        };
    }
}