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
        };
    }
}