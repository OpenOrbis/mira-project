#pragma once
#include <Utils/Vector.hpp>
#include <netinet/in.h>

#include <Messaging/Message.hpp>

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

                uint8_t m_MessageBuffer[Messaging::MessageHeader_MaxPayloadSize + sizeof(Messaging::MessageHeader)];

            public:
                Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address);
                virtual ~Connection();

                void Disconnect();

                int32_t GetSocket() { return m_Socket; }
                uint32_t GetId() { return m_Id; }
                bool IsRunning() { return m_Running; }
                struct sockaddr_in* GetAdress() { return &m_Address; }
                void** Internal_GetThread() { return &m_Thread; }
                void* GetConnectionThread() { return m_Thread; }

                static void ConnectionThread(void* p_Connection);
            };
        }
    }
}