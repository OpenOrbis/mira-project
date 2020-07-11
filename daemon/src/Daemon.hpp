#pragma once
#include <Utils/IModule.hpp>

#include <mutex>

namespace Mira
{
    class Daemon :
        public Utils::IModule
    {
    private:
        std::unique_ptr<Utils::IModule> m_RpcServer;
        std::unique_ptr<Utils::IModule> m_Debugger;
        std::unique_ptr<Utils::IModule> m_FtpServer;

    public:
        Daemon();
        virtual ~Daemon();

        virtual bool OnLoad() override;
    };
};