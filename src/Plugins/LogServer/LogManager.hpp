#pragma once 
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>
#include <Utils/Hook.hpp>

#include <netinet/in.h>
#include <sys/syslimits.h>
#include <sys/param.h>

#define DEFAULT_PATH "/dev/klog"

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

                // Device path
                char m_Device[PATH_MAX];

                // Running
                volatile bool m_Running;

            public:
                LogManager(uint16_t p_Port = 9998, char* p_Device = nullptr);
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