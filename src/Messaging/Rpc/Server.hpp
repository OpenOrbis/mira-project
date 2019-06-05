#pragma once 
#include <Utils/SharedPtr.hpp>
#include <Utils/Vector.hpp>

#include <netinet/in.h>

struct thread;

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            enum { Server_MaxConnections = 256 };
            class Connection;

            class Server
            {
            private:
                // Server address
                struct sockaddr_in m_Address;

                // Socket
                int32_t m_Socket;

                // Thread
                void* m_Thread;

                // Running
                volatile bool m_Running;

                // Give a id to each connection for lookup later
                uint32_t m_NextConnectionId;

                Vector<shared_ptr<Rpc::Connection>> m_Connections;

                struct mtx m_Lock;

            public:
                Server(uint16_t p_Port);
                ~Server();

                bool Startup();
                bool Teardown();

                static void OnHandleConnection(Rpc::Server* p_Instance, shared_ptr<Rpc::Connection> p_Connection);
                static void OnConnectionDisconnected(Rpc::Server* p_Instance, shared_ptr<Rpc::Connection> p_Connection);

            private:
                int32_t GetSocketById(uint32_t p_Id);
                int32_t Deprecated_GetSocketByThread(struct thread* p_Thread);
            };
        }
    }
}