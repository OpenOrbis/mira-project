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

#define BREAKPOINT() asm("   int $3");

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

            void putDebugChar(char p_Char);
            unsigned char getDebugChar();

            enum 
            { 
                BufMax = PAGE_SIZE * 4,
                NumRegs = 14,
                NumRegBytes = (NumRegs * sizeof(void*))
            };
            enum amd64_regnum
            {
                AMD64_RAX_REGNUM,
                AMD64_RBX_REGNUM,
                AMD64_RCX_REGNUM,
                AMD64_RDX_REGNUM,
                AMD64_RSI_REGNUM,
                AMD64_RDI_REGNUM,
                AMD64_RBP_REGNUM,
                AMD64_RSP_REGNUM,
                AMD64_R8_REGNUM,
                AMD64_R9_REGNUM,
                AMD64_R10_REGNUM,
                AMD64_R11_REGNUM,
                AMD64_R12_REGNUM,
                AMD64_R13_REGNUM,
                AMD64_R14_REGNUM,
                AMD64_R15_REGNUM,
                AMD64_RIP_REGNUM,
                AMD64_EFLAGS_REGNUM,
                AMD64_CS_REGNUM,
                AMD64_SS_REGNUM,
                AMD64_DS_REGNUM,
                AMD64_ES_REGNUM,
                AMD64_FS_REGNUM,
                AMD64_GS_REGNUM,
                AMD64_ST0_REGNUM = 24,
                AMD64_ST1_REGNUM,
                AMD64_FCTRL_REGNUM = AMD64_ST0_REGNUM + 8,
                AMD64_FSTAT_REGNUM = AMD64_ST0_REGNUM + 9,
                AMD64_FTAG_REGNUM = AMD64_ST0_REGNUM + 10,
                AMD64_XMM0_REGNUM = 40,
                AMD64_XMM1_REGNUM,
                AMD64_MXCSR_REGNUM = AMD64_XMM0_REGNUM + 16,
                AMD64_YMM0H_REGNUM,
                AMD64_YMM15H_REGNUM = AMD64_YMM0H_REGNUM + 15,
                AMD64_BND0R_REGNUM = AMD64_YMM15H_REGNUM + 1,
                AMD64_BND3R_REGNUM = AMD64_BND0R_REGNUM + 3,
                AMD64_BNDCFGU_REGNUM,
                AMD64_BNDSTATUS_REGNUM,
                AMD64_XMM16_REGNUM,
                AMD64_XMM31_REGNUM = AMD64_XMM16_REGNUM + 15,
                AMD64_YMM16H_REGNUM,
                AMD64_YMM31H_REGNUM = AMD64_YMM16H_REGNUM + 15,
                AMD64_K0_REGNUM,
                AMD64_K7_REGNUM = AMD64_K0_REGNUM + 7,
                AMD64_ZMM0H_REGNUM,
                AMD64_ZMM31H_REGNUM = AMD64_ZMM0H_REGNUM + 31,
                AMD64_PKRU_REGNUM,
                AMD64_FSBASE_REGNUM,
                AMD64_GSBASE_REGNUM
            };


            static const char hexchars[];

            int hex (char ch);
        public:
        };
    }
}