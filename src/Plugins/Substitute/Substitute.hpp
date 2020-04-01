#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>

    static __inline __pure2 void*
    __getds0(void)
    {
        void* ret;

        __asm("movq %%gs:0,%0" : "=r" (ret));
        return (ret);
    }
};

struct proc;
namespace Mira
{
    namespace Plugins
    {
        class Substitute : public Utils::IModule
        {
        private:
            // Start / Stop process
            eventhandler_entry* m_processStartHandler;
            eventhandler_entry* m_processEndHandler;

        public:
            Substitute();
            virtual ~Substitute();
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

            uint64_t FindOffsetFromNids(struct proc* p, const char* nids_to_find);
            void DebugImportTable(struct proc* p);

        protected:
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
            static Substitute* GetPlugin();
        };
    }
}