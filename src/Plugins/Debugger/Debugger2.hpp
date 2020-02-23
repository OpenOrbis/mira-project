#pragma once
#include <Utils/IModule.hpp>
#include <Messaging/Rpc/Connection.hpp>

extern "C"
{
    #include <Messaging/Rpc/rpc.pb-c.h>
    #include "debugger.pb-c.h"
    #include <sys/socket.h>
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
            } Commands;

            // Remote GDB extension
            // https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html
            struct sockaddr m_ServerAddress;
            int32_t m_Socket;
            uint16_t m_Port;

            // Local target information

            // Attached process id
            int32_t m_AttachedPid;

        public:
            Debugger2(uint16_t p_Port = -1);
            virtual ~Debugger2();

            // Module callbacks
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            // The gdb protocol does not allow the '$' or '#' in data, they must be escaped
            // The escape character is 0x7d '}' then the next byte is $ or # or'd with 0x20, } character itself too must be escaped
            void EscapePacket(char* p_PacketData, uint32_t p_Size);

            /**
             * @brief Determines if a process is alive
             * 
             * @param p_ProcessId Process id
             * @return true If process exists
             * @return false If process does not exist
             */
            bool IsProcessAlive(int32_t p_ProcessId);

            bool PacketRequiresAcknowledgement(char p_Request);
        public:
        
            /**
             * @brief Reads memory from within a running process
             * 
             * @param p_Address Address to read from in the process
             * @param p_Size Size of data to read
             * @param p_OutData Output buffer of bytes allocated by the caller
             * @return true On success
             * @return false On failure
             */
            bool ReadProcessMemory(uint64_t p_Address, uint32_t p_Size, uint8_t*& p_OutData);
            /**
             * @brief Writes memory to a running process
             * 
             * @param p_Address Address to write to in the process
             * @param p_Data Input data to write
             * @param p_Size Size of the data to read
             * @return true Success
             * @return false Failure
             */
            bool WriteProcessMemory(uint64_t p_Address, uint8_t* p_Data, uint32_t p_Size);
            
            /**
             * @brief Changes protection on specified address and size
             * 
             * @param p_Address Address in process
             * @param p_Size Size of protection to change
             * @param p_Protection Protection flags
             * @return true Success
             * @return false Error
             */
            bool ProtectProcessMemory(uint64_t p_Address, uint32_t p_Size, int32_t p_Protection);

            /**
             * @brief Get the Process Main Thread object
             * 
             * @return struct thread* Thread or nullptr on error
             */
            struct thread* GetProcessMainThread();

            /**
             * @brief Get the Process Full Info object
             * 
             * @param p_Info Output information
             * @return true On success
             * @return false On failure
             */
            bool GetProcessFullInfo(DbgProcessFull* p_Info);

            /**
             * @brief Get limited process information
             * 
             * @param p_Info Output information
             * @return true On success
             * @return false On failure
             */
            bool GetProcessLimitedInfo(DbgProcessLimited* p_Info);

            /**
             * @brief Get limited thread information
             * 
             * @param p_Thread Input thread
             * @param p_Info Output thread information
             * @return true On success
             * @return false On failure
             */
            bool GetThreadLimitedInfo(struct thread* p_Thread, DbgThreadLimited* p_Info);

            /**
             * @brief Get full thread information
             * 
             * @param p_Thread Thread to get information from
             * @param p_Info Output information object
             * @return true Success
             * @return false Failure
             */
            bool GetThreadFullInfo(struct thread* p_Thread, DbgThreadFull* p_Info);
            
            /**
             * @brief Allocates memory in the attached process
             * 
             * @param p_Size Size of the allocation
             * @param p_Zero Zero the allocation on successful allocation
             * @return uint64_t Address of the allocation in the process
             */
            uint64_t AllocateProcessMemory(uint32_t p_Size, bool p_Zero = true);
            
            /**
             * @brief Free memory allocated with AllocateProcessMemory
             * 
             * @param p_Address Address to call free on
             * @param p_Size Size of the allocation
             * @return true Success
             * @return false Failure
             */
            bool FreeProcessMemory(uint64_t p_Address, uint32_t p_Size);

            /**
             * @brief Attach to a process
             * 
             * @param p_ProcessId ProcessId to attach to
             * @param p_Status optional status output
             * @return true on success
             * @return false if already attached, or failure
             */
            bool Attach(int32_t p_ProcessId, int32_t* p_Status = nullptr);

            /**
             * @brief Detaches from the process, this does not clear bp and wp's
             * 
             * @param p_Force force debugger detach uncleanly (may leave proc/console in unstable state)
             * @param p_Status optional status output
             * @return true Successful detach
             * @return false Not attached, or failure
             */
            bool Detach(bool p_Force = false, int32_t* p_Status = nullptr);

            // This function returns a handle that will be used to query this breakpoint
            //uint64_t AddBreakpoint(uint64_t p_Address, DbgBreakpointType p_Type, bool p_Enabled);
            //uint64_t AddWatchpoint(uint64_t p_Address, uint32_t p_Length, bool p_Enabled);

            /**
             * @brief Gets limited view of all threads
             * 
             * @param p_OutThreads Outgoing thread structures (list needs to be allocated ahead of time)
             * ex: DbgThreadLimited* s_Threads[p_ThreadCount];
             * @param p_ThreadCount Thread count
             * @return true Success
             * @return false Failure
             */
            bool GetProcessThreads(DbgThreadLimited** p_OutThreads, uint32_t p_ThreadCount);

            /**
             * @brief Signals the attached process
             * 
             * @param p_Signal Signal to send
             * @return true On success
             * @return false on failure
             */
            bool SignalProcess(int32_t p_Signal);

            //bool GetThreadRegisters(int32_t p_ThreadId, DbgGpRegisters& p_GpRegisters, DbgFpRegisters& p_FpRegisters, DbgDbRegisters& p_DbRegisters);
            //bool SetThreadRegisters(int32_t p_ThreadId, DbgGpRegisters& p_GpRegisters, DbgFpRegisters& p_FpRegisters, DbgDbRegisters& p_DbRegisters);

            /**
             * @brief Reads kernel memory and returns it in requested buffer
             * 
             * @param p_Address Address of the read
             * @param p_Size Size of data to read
             * @param p_OutData Output buffer location
             * @return true Successful read
             * @return false Faulted or other error
             */
            bool ReadKernelMemory(uint64_t p_Address, uint32_t p_Size, uint8_t*& p_OutData);
            
            /**
             * @brief Writes kernel memory
             * 
             * @param p_Address Address to write to
             * @param p_Data Data to write to
             * @param p_Size Length of data to write
             * @return true Success
             * @return false Failure
             */
            bool WriteKernelMemory(uint64_t p_Address, uint8_t* p_Data, uint32_t p_Size);

        private:
            static void OnAttach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnDetach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnReadProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnWriteProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnProtectProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnGetProcessInfo(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnAllocateProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnFreeProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnAddBreakpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnAddWatchpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnSignalProcess(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnGetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnSetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            static void OnReadKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            static void OnWriteKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
        };
    }
}