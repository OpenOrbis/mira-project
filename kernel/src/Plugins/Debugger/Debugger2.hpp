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
        /**
         * @brief Debugger Plugin
         * 
         * This thing memory leaks like a mf
         * go back and fix it at a later date, maybe tomorow?
         */
        class Debugger2 : public Utils::IModule
        {
        private:
            enum
            {
                GdbDefaultPort = 9997
            };

            typedef enum _Commands
            {
                DbgCmd_None = 0x7AB56E31,
                DbgCmd_ReadMem = 0xF25FEE19,
                DbgCmd_WriteMem = 0x78B3A60C,
                DbgCmd_ProtectMem = 0x73FA541B,
                DbgCmd_ScanMem = 0xEDCCE6D4,
                DbgCmd_GetProcInfo = 0xF3B7D3F1,
                DbgCmd_AllocateProcMem = 0x16FE60FC,
                DbgCmd_FreeProcMem = 0x93E0CC76,
                DbgCmd_GetProcMap = 0x758DC819,
                DbgCmd_Attach = 0xFEFCF9C8,
                DbgCmd_Detach = 0xF3B1D649,
                DbgCmd_Breakpoint = 0xD60E69E4,
                DbgCmd_Watchpoint = 0x23DE0FCE,
                DbgCmd_GetProcThreads = 0x1F5290F2,
                DbgCmd_SignalProc = 0xA2E2610F,
                DbgCmd_GetRegs = 0x449EAA46,
                DbgCmd_SetRegs = 0xD70F129B,
                DbgCmd_GetThreadInfo = 0x51B931C2,
                DbgCmd_ThreadSinglestep = 0x080B3752,
                DbgCmd_ReadKernelMem = 0x844AE491,
                DbgCmd_WriteKernelMem = 0xCFD904E7,
                DbgCmd_GetProcList = 0x7CF61ABE
            } Commands;

            Utils::Hook* m_TrapFatalHook;

            // Remote GDB extension
            // https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html
            struct sockaddr m_ServerAddress;
            int32_t m_Socket;
            uint16_t m_Port;

            // struct sx m_Mutex;
            eventhandler_tag m_OnProcessExitTag;
            

            // Local target information

            // Attached process id
            int32_t m_AttachedPid;

        public:
            Debugger2(uint16_t p_Port = -1);
            virtual ~Debugger2();

            virtual const char* GetName() override { return "Debugger"; }
            virtual const char* GetDescription() override { return "GDB compatible debugger with ReClass abilties."; }

            // Module callbacks
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static void OnTrapFatal(struct trapframe* p_Frame, vm_offset_t p_Eva);
            static bool IsStackSpace(void* p_Address);
        public:
        };
    }
}