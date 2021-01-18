#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Hook.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>

#include <Messaging/Rpc/Connection.hpp>
#include <Messaging/Rpc/Connection.hpp>
#include <Messaging/MessageManager.hpp>
#include <Plugins/PluginManager.hpp>

#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/uio.h>
	#include <sys/proc.h>
	#include <sys/ioccom.h>
    #include <sys/ptrace.h>
    #include <sys/mman.h>
    #include <sys/sx.h>
    #include <sys/errno.h>
    #include <sys/wait.h>
    #include <sys/socket.h>

    #include <vm/vm.h>
	#include <vm/pmap.h>

    #include <machine/reg.h>
    #include <machine/param.h>
    #include <machine/frame.h>
	#include <machine/psl.h>
	#include <machine/pmap.h>
	#include <machine/segments.h>

	#include <sys/eventhandler.h>
};

#define BREAKPOINT() asm("   int $3");

namespace Mira
{
    namespace Plugins
    {
        class Debugger : public Utils::IModule
        {
        private:
            Utils::Hook* m_TrapFatalHook;

        public:
            Debugger();
            virtual ~Debugger();

            virtual const char* GetName() override { return "Debugger"; }
            virtual const char* GetDescription() override { return "Debugger"; }

            // Module callbacks
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

        protected:
            static void OnTrapFatal(struct trapframe* p_Frame, vm_offset_t p_Eva);
            static bool IsStackSpace(void* p_Address);
        public:
        };
    }
}