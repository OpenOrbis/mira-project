#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>
};

struct proc;
namespace Mira
{
    namespace Plugins
    {
        class Substitute : public Utils::IModule
        {
        private:
            eventhandler_entry* m_processStartHandler;
            eventhandler_entry* m_processEndHandler;

        public:
            Substitute();
            virtual ~Substitute();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static int MountNullFS(char* where, char* what);
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
            static Substitute* GetPlugin();
        };
    }
}