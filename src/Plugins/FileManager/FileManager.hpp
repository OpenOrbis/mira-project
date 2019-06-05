#pragma once
#include <Utils/IModule.hpp>

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

            virtual const wchar_t* GetName() override { return L"FileManager"; }
            virtual const wchar_t* GetDescription() override { return L"replaces ftp"; }
        };
    }
}