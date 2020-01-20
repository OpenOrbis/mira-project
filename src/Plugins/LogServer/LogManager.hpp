#pragma once 
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

#include <netinet/in.h>

struct thread;

namespace Mira
{
    namespace Plugins
    {
        namespace LogManagerExtent
        {
            class LogManager : public Mira::Utils::IModule
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

            public:
                LogManager(uint16_t p_Port = 9998);
                virtual ~LogManager();

                bool Startup();
                bool Teardown();
                virtual const char* GetName() override { return "LogServer"; }
                virtual bool OnLoad() override;
                virtual bool OnUnload() override;
                virtual bool OnSuspend() override;
                virtual bool OnResume() override;

            private:
                static void ServerThread(void* p_UserData);
            };
        }
    }
}