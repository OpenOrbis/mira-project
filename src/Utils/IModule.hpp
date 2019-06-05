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

            virtual bool OnLoad() { return true; }
            virtual bool OnUnload() { return true; }
            virtual bool OnSuspend() { return true; }
            virtual bool OnResume() { return true; }

            virtual const wchar_t* GetName() { return L""; };
            virtual const wchar_t* GetDescription() { return L""; };
        };
    }
}