#pragma once
#include <vector>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

#include <google/protobuf/arena.h>

extern "C"
{
    #include <sys/socket.h>
    #include <netinet/in.h>
};

namespace Mira
{
    namespace Rpc
    {
        class Listener;
        class Connection;

        class Manager
        {
        private:
            enum class Options
            {
                DefaultPort = 9999,
                MaxDefaultPort = 10020,
                MaxConnections = 8,

                MaxIncomingMessageSize = 0x4000000, // 64 Megabytes
                MaxOutgoingMessageSize = 0x4000000, // 64 Megabytes
            };
            //
            // Server socket code
            //
            int32_t m_Socket;
            int16_t m_Port;
            struct sockaddr_in m_Address;

            google::protobuf::Arena m_Arena;
            std::mutex m_Mutex;

            // Internal tracker for the listeners
            std::vector<std::shared_ptr<Listener>> m_Listeners;

            uint64_t m_NextConnectionId;
            std::vector<std::shared_ptr<Connection>> m_Connections;

            std::thread m_ServerThread;
        public:
            Manager();
            virtual ~Manager();

        protected:
            bool Startup();

        private:
            void OnIncomingMessage();
            void OnConnectionDisconnect(uint64_t p_ConnectionId);

            // Internal functions do not lock, be warned!
            void Internal_CloseSocket();
        };
    }
}