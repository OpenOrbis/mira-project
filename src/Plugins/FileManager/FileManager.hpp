#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Span.hpp>
#include <Utils/SharedPtr.hpp>

namespace Mira
{
    namespace Messaging
    {
        class Message;
    }
    
    namespace Plugins
    {
        class FileManager : public Mira::Utils::IModule
        {
            Span<uint8_t> m_Buffer;

        public:
            FileManager();
            virtual ~FileManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

            virtual const char* GetName() override { return "FileManager"; }
            virtual const char* GetDescription() override { return "replaces ftp"; }

        private:
            static void OnEcho(shared_ptr<Messaging::Message> p_Message);
            static void OnOpen(shared_ptr<Messaging::Message> p_Message);
            static void OnClose(shared_ptr<Messaging::Message> p_Message);
            static void OnRead(shared_ptr<Messaging::Message> p_Message);
            static void OnWrite(shared_ptr<Messaging::Message> p_Message);
            static void OnGetDents(shared_ptr<Messaging::Message> p_Message);
            static void OnStat(shared_ptr<Messaging::Message> p_Message);
            static void OnMkDir(shared_ptr<Messaging::Message> p_Message);
            static void OnRmDir(shared_ptr<Messaging::Message> p_Message);
            static void OnUnlink(shared_ptr<Messaging::Message> p_Message);
        };
    }
}