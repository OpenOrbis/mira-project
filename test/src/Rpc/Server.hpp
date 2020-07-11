#pragma once 
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

#ifdef __PS4__
extern "C"
{
    #include <netinet/in.h>
    #include <sys/param.h>
    #include <sys/sx.h>
}
#else
#include <netinet/in.h>
#endif

struct thread;

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            enum {  RpcServer_MaxConnections = 8,
                    RpcServer_DefaultPort = 9999, };
            class Connection;

            class Server : Mira::Utils::IModule
            {
            private:
                // Server address
                struct sockaddr_in m_Address;

                // Socket
                int32_t m_Socket;

                // Server port
                uint16_t m_Port;

                // Thread
                void* m_Thread;

                // Running
                volatile bool m_Running;

                // Give a id to each connection for lookup later
                uint32_t m_NextConnectionId;

                Rpc::Connection* m_Connections[RpcServer_MaxConnections];

                // struct sx m_Mutex;

            public:
                Server(uint16_t p_Port = RpcServer_DefaultPort);
                virtual ~Server();

                bool Startup();
                bool Teardown();

                void OnHandleConnection(Rpc::Connection* p_Connection);
                void OnConnectionDisconnected(Rpc::Connection* p_Connection);

                virtual const char* GetName() override { return "RpcServer"; }
                virtual bool OnLoad() override;
                virtual bool OnUnload() override;
                virtual bool OnSuspend() override;
                virtual bool OnResume() override;

            public:
                int32_t GetSocketById(uint32_t p_Id);
                int32_t GetSocketByConnection(Rpc::Connection* p_Connection);
                int32_t GetFreeConnectionCount();
                int32_t GetUsedConnectionCount();
                int32_t GetFreeConnectionIndex();

            private:
                static void* ServerThreadWrapper(void* p_UserData);
                static void ServerThread(void* p_UserData);
            };
        }
    }
}