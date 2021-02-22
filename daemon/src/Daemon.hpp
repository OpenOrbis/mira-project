#pragma once
#include <Utils/IModule.hpp>
#include <cstdio>
#include <mutex>
#include <memory>

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
        static std::shared_ptr<Daemon> GetInstance();

        Daemon();
        virtual ~Daemon();

        virtual bool OnLoad() override;

        std::shared_ptr<Rpc::Manager> GetMessageManager() const { return m_MessageManager; }
        std::shared_ptr<Rpc::Server> GetRpcServer() const { return m_RpcServer; }
    };
};