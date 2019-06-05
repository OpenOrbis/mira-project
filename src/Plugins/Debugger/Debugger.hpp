#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Hook.hpp>

struct trapframe;

namespace Mira
{
    namespace Plugins
    {
        class Debugger : public Mira::Utils::IModule
        {
        private:
            Utils::Hook* m_TrapFatalHook;

        protected:
            static void OnTrapFatal(struct trapframe* p_Frame, vm_offset_t p_Eva);
            static bool IsStackSpace(void* p_Address);
            
        public:
            Debugger();
            virtual ~Debugger();

            virtual const char* GetName() override { return "Debugger"; }
            virtual const char* GetDescription() override { return "GDB compatible debugger with ReClass abilties."; }
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

            virtual bool OnSuspend() override;
            virtual bool OnResume() override;
        };
    }
}