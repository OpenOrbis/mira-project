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
    #include <Messaging/Rpc/rpc.pb-c.h>

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

namespace Mira
{
    namespace Plugins
    {
        class PS4GDB : public Utils::IModule
        {
        public:
            PS4GDB();
            virtual ~PS4GDB();

            virtual const char* GetName() override { return "PS4GDB"; }
            virtual const char* GetDescription() override { return "PS4 userland gdbstub"; }

            // Module callbacks
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
        private:
            // Necessary methods
            static void PS4GDBMain(void *);
            bool SpawnKproc();
            static void GetRoot();
        };
    }
}