#pragma once 
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>
#include <Utils/Hook.hpp>

#include <netinet/in.h>
#include <sys/syslimits.h>
#include <sys/param.h>

namespace Mira
{
    namespace Plugins
    {
        class ModuleLoader : public Utils::IModule
        {
        public:
            ModuleLoader();
            virtual ~ModuleLoader();

            virtual const char* GetName() override { return "ModuleLoader"; }
            virtual const char* GetDescription() override { return "Loads raw binaries into kernel space and executes them";}

            // Module callbacks
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
        private:
            // Necessary methods
            static void KprocMain(void *);
            bool SpawnKproc();
            static void GetRoot();            
        };
    }
}