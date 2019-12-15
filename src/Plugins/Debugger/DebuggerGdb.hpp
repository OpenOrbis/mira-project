// Special Thanks to: m0rph3us1987
// for the c port of his stub as reference

#include <Utils/Types.hpp>

extern "C"
{
    #include <netinet/in.h>
    #include <machine/reg.h>
};


// Hold our default std paths
#define GDB_DEFAULT_STDOUT  "/dev/console"
#define GDB_DEFAULT_STDIN   "/dev/zero"
#define GDB_DEFAULT_STDERR  "/dev/console"

#define BREAKPOINT() __asm__("int $3");

namespace Mira
{
    namespace Plugins
    {
        class DebuggerGdb
        {
        private:
            typedef enum _amd64_regnum
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
            } amd64_regnum;

            typedef enum _regnames 
            {
                RAX,
                RCX,
                RDX,
                RBX,
                RSP,
                RBP,
                RSI,
                RDI,
                RIP,
                EFLAGS,
                CS,
                SS,
                DS,
                ES,
                R8,
                R9,
                R10,
                R11,
                R12,
                R13,
                R14,
                R15,
                FS,
                GS
            } regnames;

            enum
            {
                GDB_DEFAULT_PORT = 2345,
                GDB_INVALID_HANDLE = -1,
                GDB_NUM_REGS = 14,
                GDB_MAX_BUFFER_SIZE = 0x4000,
                GDB_REQUIRED_BUFFER_SIZE = 2 * GDB_MAX_BUFFER_SIZE,
            };

            

        private:
            // Remote GDB
            int32_t m_GdbSocket;
            struct sockaddr_in m_GdbAddress;

            struct reg m_Registers;
            struct fpreg m_FloatingRegisters;
            struct dbreg m_DebugRegisters;

            uint8_t* m_Buffer;
            uint32_t m_BufferSize;

            static const char c_Hex[];

            bool m_ExtendedMode;
            int32_t m_LastSignal;

            uint32_t m_ArgCount;
            const char** m_Args;

            bool m_EnableErrorStrings;

            // Path for i/o
            const char* m_StdInPath;
            const char* m_StdOutPath;
            const char* m_StdErrPath;

            // Override working directory
            const char* m_WorkingDirectory;

            bool m_DisableASLR;
            bool m_ListThreadsInStopReply;

            uint32_t m_PacketSizeSupported;

            int32_t m_HaltReason;

            int32_t m_ProcessId;

        public:
            DebuggerGdb();
            ~DebuggerGdb();
            
            bool StartServer(int32_t p_ProcessId, uint16_t p_Port = (uint16_t)GDB_INVALID_HANDLE);

            // Continue at specified address
            // if p_Address == 0, resume at current address
            void Continue(uint64_t p_Address);
            void ContinueWithSignal(uint64_t p_Address, int32_t p_Signal);

            void Detach();

            void ParsePacket(const char* p_Data, uint32_t p_DataLength);

            bool PutChar(uint8_t p_Char);
            uint8_t GetChar();

            bool WritePacket(uint8_t* p_Buffer, uint32_t p_BufferLength);
            uint8_t* ReadPacket();

            static void OnException(int p_ExceptionVector, uint64_t p_ErrorCode);

        private:
            static bool IsProcessAlive(int32_t p_ProcessId);
            static int Hex(char p_Char);

            bool UpdateRegisters();

            bool WriteResponse(const char* p_Response, const uint8_t* p_Data, uint32_t p_DataLength);
            bool WriteError(int32_t p_ErrorCode);

            void HandleExtendedMode(const char* p_Data, uint32_t p_DataLength);
            void HandleHaltReason(const char* p_Data, uint32_t p_DataLength);
            void HandleB(const char* p_Data, uint32_t p_DataLength);

            void HandleContinue(const char* p_Data, uint32_t p_DataLength);
            void HandleContinueWithSignal(const char* p_Data, uint32_t p_DataLength);

            void HandleDebug(const char* p_Data, uint32_t p_DataLength);
            void HandleDetach(const char* p_Data, uint32_t p_DataLength);

            void HandleFile(const char* p_Data, uint32_t p_DataLength);

            void HandleReadGeneralRegisters(const char* p_Data, uint32_t p_DataLength);
            void HandleWriteGeneralRegisters(const char* p_Data, uint32_t p_DataLength);

            void HandleSetThread(const char* p_Data, uint32_t p_DataLength);

            void HandleStepClock(const char* p_Data, uint32_t p_DataLength);
            void HandleSignalCycleStep(const char* p_Data, uint32_t p_DataLength);
            
            void HandleKill(const char* p_Data, uint32_t p_DataLength);

            void HandleRead(const char* p_Data, uint32_t p_DataLength);

            void HandleWrite(const char* p_Data, uint32_t p_DataLength);

            void HandleReadRegister(const char* p_Data, uint32_t p_DataLength);
            void HandleWriteRegister(const char* p_Data, uint32_t p_DataLength);

            void HandleGeneralQuery(const char* p_Data, uint32_t p_DataLength);
            void HandleSetQuery(const char* p_Data, uint32_t p_DataLength);

            void HandleReset(const char* p_Data, uint32_t p_DataLength);
            void HandleRestart(const char* p_Data, uint32_t p_DataLength);

            void HandleSingleStep(const char* p_Data, uint32_t p_DataLength);
            void HandleStepWithSignal(const char* p_Data, uint32_t p_DataLength);

            void HandleSearchBackwards(const char* p_Data, uint32_t p_DataLength);
            void HandleThreadIdAlive(const char* p_Data, uint32_t p_DataLength);

            void HandleV(const char* p_Data, uint32_t p_DataLength);
        };
    }
}