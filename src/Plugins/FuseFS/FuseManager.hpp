#pragma once
#include <Utils/IModule.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
};

namespace Mira
{
    namespace Plugins
    {
        class FuseManager : public Utils::IModule
        {
        private:
            eventhandler_entry* m_DeviceCloneHandler;
            
        public:
            FuseManager();
            virtual ~FuseManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static void OnDeviceClone(void* p_Reserved);
        };
    }
}