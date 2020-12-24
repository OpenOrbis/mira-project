#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Plugins
    {
        class DebugSettingsActivator : public Mira::Utils::IModule
        {
        public:
            DebugSettingsActivator();
            virtual ~DebugSettingsActivator();
            virtual const char* GetName() override { return "DebugSettingsActivator"; }
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;
        };
    }
}
