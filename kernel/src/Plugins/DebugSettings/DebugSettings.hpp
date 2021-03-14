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
            #if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_755
                static constexpr int SYSCTL_DEBUG_1 = 1208; //machdep.rcmgr_debug_menu
                static constexpr int SYSCTL_DEBUG_2 = 1218; //machdep.rcmgr_utoken_store_mode
            #else
                #error "unsupported firmware"
            #endif
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
