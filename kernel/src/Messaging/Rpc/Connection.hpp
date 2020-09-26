#pragma once
#include <Utils/Vector.hpp>

extern "C"
{
    #include <netinet/in.h>
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
};

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
                enum { MaxBufferSize = 0x8000 };
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
                // TODO: Re-enable this once protobuf has been replaced
                //Rpc::Server* m_Server;

                struct mtx m_Mutex;

            public:
                Connection(Rpc::Server* p_Server, uint32_t p_ClientId, int32_t p_Socket, struct sockaddr_in& p_Address);
                virtual ~Connection();

                // This Takes a lock
                void Disconnect();

                // This takes a lock
                void SetRunning(bool p_Running);

                struct mtx* GetMutex() { return &m_Mutex; }
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