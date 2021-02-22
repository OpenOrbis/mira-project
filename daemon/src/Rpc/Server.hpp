#pragma once
#include <vector>
#include <memory>

#include <Utils/IModule.hpp>

#if defined(PS4)
#include <orbis/libkernel.h>
#include <orbis/Net.h>
#else
#include <netinet/in.h>
#endif

extern "C"
{
    #include <sys/socket.h>
    #include <netinet/in.h>

    #include <pthread.h>
};

namespace Mira
{
    namespace Rpc
    {
        class Connection;

        class Server : 
            public Utils::IModule
        {
        private:
            enum
            {
                RpcServer_MaxConnections = 8,
                RpcServer_DefaultPort = 9999
            };

            // Socket
            int32_t m_Socket;

            // Port
            uint16_t m_Port;

            // Thread
            pthread_t m_Thread;

            // Running
            volatile bool m_Running;

            // Give semi-random id for lookup later
            uint32_t m_NextConnectionId;

            // Address
            struct sockaddr_in m_Address;

            // Connections
            std::vector<std::shared_ptr<Rpc::Connection>> m_Connections;

            // TODO: mutex

        public:
            Server(uint16_t p_Port = RpcServer_DefaultPort);
            virtual ~Server();

            virtual const char* GetName() override { return "RpcServer"; }
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            bool Startup();
            bool Teardown();

            int32_t GetSocketByClientId(uint32_t p_ClientId);
            
        private:
            static void* ServerThread(void* p_ServerInstance);
        };
    }
}