#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Span.hpp>
#include <Utils/SharedPtr.hpp>

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Connection;
        }
        class Message;
    }
    
    namespace Plugins
    {
        class FileManager : public Mira::Utils::IModule
        {
        public:
            FileManager();
            virtual ~FileManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

            virtual const char* GetName() override { return "FileManager"; }
            virtual const char* GetDescription() override { return "replaces ftp"; }

        private:
            static void OnEcho(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnOpen(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnClose(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnRead(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnWrite(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnGetDents(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnStat(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnMkDir(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnRmDir(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
            static void OnUnlink(Messaging::Rpc::Connection* p_Connection, shared_ptr<Messaging::Message> p_Message);
        };
    }
}