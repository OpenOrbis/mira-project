#pragma once
#include <Utils/IModule.hpp>

#include <mutex>

namespace Mira
{
    namespace Debugging
    {
        class Debugger;
    }

    namespace Rpc
    {
        class Manager;
        class Server;
    }
    
    class Daemon :
        public Utils::IModule
    {
    private:
        
        std::shared_ptr<Debugging::Debugger> m_Debugger;
        std::shared_ptr<Utils::IModule> m_FtpServer;

        // RPC
        std::shared_ptr<Rpc::Manager> m_MessageManager;
        std::shared_ptr<Rpc::Server> m_RpcServer;

    public:
        Daemon();
        virtual ~Daemon();

        virtual bool OnLoad() override;

    protected:
        
    };
};