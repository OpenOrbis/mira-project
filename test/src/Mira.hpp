#pragma once

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Connection;
            class Server;
        }

        class MessageManager;
    }
    class Framework
    {
    private:
        static Framework* m_Instance;
        Framework();

    public:
        
        static Framework* GetFramework(); // { if (m_Instance == nullptr)m_Instance = new Framework(); return m_Instance; }
        bool Initialize();
        
        void* GetMainThread() { return reinterpret_cast<void*>(0xDEADBEEFDEADBEEF); }
        Messaging::Rpc::Server* GetRpcServer() { return m_Server; }
        Messaging::MessageManager* GetMessageManager() { return m_MessageManager; }
        
        Messaging::Rpc::Server* m_Server;
        Messaging::MessageManager* m_MessageManager;
    };
}