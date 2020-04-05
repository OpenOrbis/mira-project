#pragma once
#include <Utils/Types.hpp>
#include <Utils/IModule.hpp>
#include <Utils/Hook.hpp>

namespace Mira
{
    namespace Plugins
    {
        class SyscallGuard :
            public Mira::Utils::IModule
        {
        private:
            uint32_t m_SyscallCount;
            Mira::Utils::Hook* m_SyscallHooks;

        public:
            SyscallGuard();
            virtual ~SyscallGuard();

            virtual bool OnLoad();
            virtual bool OnUnload();
            virtual bool OnSuspend();
            virtual bool OnResume();

        protected:
            static int DeadSyscallHandler(struct thread* p_Thread, void* p_Uap);
        };
    }
}