#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Plugins
    {
        class FileManager : public Mira::Utils::IModule
        {
            uint8_t m_Buffer[0x10000];

        public:
            FileManager();
            virtual ~FileManager();

            virtual const char* GetName() override { return "FileManager"; }
            virtual const char* GetDescription() override { return "replaces ftp"; }
        };
    }
}