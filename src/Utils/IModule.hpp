#pragma once

namespace Mira
{
    namespace Utils
    {
        class IModule
        {
        public:
            IModule() { }
            virtual ~IModule() { }

            virtual bool OnLoad() { return false; }
            virtual bool OnUnload() { return false; }
            virtual bool OnSuspend() { return false; }
            virtual bool OnResume() { return false; }

            virtual const wchar_t* GetName() { return L""; };
            virtual const wchar_t* GetDescription() { return L""; };
        };
    }
}