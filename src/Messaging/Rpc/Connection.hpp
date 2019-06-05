#pragma once
#include <Utils/Vector.hpp>
#include <netinet/in.h>

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Server;
            
            class Connection
            {
            private:
                // Connection socket
                int32_t m_Socket;

                // Client id
                uint32_t m_Id;

                // Is the client connection still running
                volatile bool m_Running;

                // Client thread loop
                void* m_Thread;

                // Client address
                struct sockaddr_in m_Address;

                // Server reference
                Rpc::Server* m_Server;

            public:
                Connection();
                virtual ~Connection();

                int32_t GetSocket() { return m_Socket; }
                uint32_t GetId() { return m_Id; }
                bool IsRunning() { return m_Running; }
                struct sockaddr_in& GetAdress() { return m_Address; }
            };
        }
    }
}