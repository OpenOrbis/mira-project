#pragma once
#include <Utils/Types.hpp>
#include <Utils/IModule.hpp>

extern "C"
{
    #include <sys/param.h>    
    #include <sys/proc.h>
    #include <sys/sysent.h>
};

namespace Mira
{
    namespace Plugins
    {
        enum class CallStatus : uint8_t
        {
            Status_Disabled,
            Status_Reserved,
            Status_Enabled,
            Status_COUNT
        };

        class SyscallGuard :
            public Mira::Utils::IModule
        {
        private:
            uint32_t m_SyscallCount;
            CallStatus* m_CallStatuses;
            sy_call_t** m_SyCalls;

            enum { SyscallCount_Max = 1024 };

        public:
            SyscallGuard();
            virtual ~SyscallGuard();

            virtual bool OnLoad() { return true; }
            virtual bool OnUnload() { return true; }
            virtual bool OnSuspend() { return true; }
            virtual bool OnResume() { return true; }

            bool SetReservedSyscall(int p_SyscallNumber, sy_call_t p_Call);

        protected:
            static int SyscallHandler(struct thread * p_Thread, void * p_Uap);
        };
    }
}