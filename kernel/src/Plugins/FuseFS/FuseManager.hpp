#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

extern "C"
{
    
    #include <sys/eventhandler.h>
    #include <sys/module.h>
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

            static int FuseLoader(struct module* p_Module, int p_What, void* p_Arg);

        protected:
            static void OnDeviceClone(void* p_Reserved);

        };
    }
}