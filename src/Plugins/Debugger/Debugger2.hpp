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
    #include "debugger.pb-c.h"

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

            struct mtx m_Mutex;

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

            /**
             * @brief Replaces an exception handler function in the IDT (interrupt descriptor table) for the specified exception number
             * 
             * @param p_ExceptionNumber Exception number to catch
             * @param p_Function Function to replace with
             * @param p_PreviousFunction Output previous function in the IDT (optional)
             * @return true On success
             * @return false On error
             */
            bool ReplaceExceptionHandler(uint32_t p_ExceptionNumber, void* p_Function, void** p_PreviousFunction = nullptr);


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
            static bool IsProcessAlive(int32_t p_ProcessId);

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
             * @brief Get thread of arbitrary process by pid/tid
             * 
             * @param p_ProcessId Process id
             * @param p_ThreadId Thread id
             * @return struct thread* or nullptr if error
             */
            static struct thread* GetThreadById(int32_t p_ProcessId, int32_t p_ThreadId);

            /**
             * @brief Get process thread by id
             * 
             * @param p_ThreadId Thread id
             * @return struct thread* or nullptr if error
             */
            struct thread* GetThreadById(int32_t p_ThreadId);

            /**
             * @brief Get the Vm Map Entries object
             * 
             * @param p_Process LOCKED PROCESS
             * @param p_Entries Output entries
             * @param p_EntriesCount Output entry count
             * @return true on success
             * @return false otherwise
             */
            static bool GetVmMapEntries(struct proc* p_Process, DbgVmEntry**& p_Entries, size_t& p_EntriesCount);
            
            /**
             * @brief Get limited process information
             * 
             * @param p_Pid Process Id
             * @param p_Info Output information
             * @return true On success
             * @return false On failure
             */
            static bool GetProcessLimitedInfo(int32_t p_Pid, DbgProcessLimited* p_Info);

            /**
             * @brief Get the Process Full Info object
             * 
             * @param p_Pid Process Id
             * @param p_Info Output information
             * @return true On success
             * @return false On failure
             */
            static bool GetProcessFullInfo(int32_t p_Pid, DbgProcessFull* p_Info);

            /**
             * @brief Get the number of threads in a process
             *
             * @param p_Pid Process Id
             */
            static uint64_t GetProcessThreadCount(int32_t p_ProcessId);

            /**
             * @brief Get limited thread information
             * 
             * @param p_ThreadId Input thread id
             * @param p_Info Output thread information
             * @return true On success
             * @return false On failure
             */
            bool GetThreadLimitedInfo(int32_t p_ThreadId, DbgThreadLimited* p_Info);

            /**
             * @brief Get limited thread information
             * 
             * @param p_ThreadId Input thread id
             * @param p_Info Output thread information
             * @return true On success
             * @return false On failure
             */
            static bool GetThreadLimitedInfo(struct thread* p_Thread, DbgThreadLimited* p_Info);

            /**
             * @brief Get full thread information
             * 
             * @param p_ThreadId Thread id to get information from
             * @param p_Info Output information object
             * @return true Success
             * @return false Failure
             */
            bool GetThreadFullInfo(int32_t p_ThreadId, DbgThreadFull* p_Info);

            /**
             * @brief Get full thread information
             * 
             * @param p_ThreadId Thread id to get information from
             * @param p_Info Output information object
             * @return true Success
             * @return false Failure
             */
            static bool GetThreadFullInfo(struct thread* p_Thread, DbgThreadFull* p_Info);

            /**
             * @brief This frees all of the nested resource, does not free the object itself
             * 
             * @param p_Info Full thread information
             */
            static void FreeDbgThreadFull(DbgThreadFull* p_Info);

            /**
             * @brief This frees all of the nested resources, does not free object itself
             * 
             * @param p_Info DbgProcessFull
             */
            static void FreeDbgProcessFull(DbgProcessFull* p_Info);
            
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

            /**
             * @brief Single steps the process
             * 
             * @return true Success
             * @return false Error
             */
            bool Step();

            /**
             * @brief Suspends (stops) the process
             * 
             * @return true Success
             * @return false Error
             */
            bool Suspend();

            /**
             * @brief Resumes the process
             * 
             * @return true Success
             * @return false Error
             */
            bool Resume();
            
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
            /**
             * @brief RPC callback for attaching to a process
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing attach rpc message
             */
            static void OnAttach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            
            /**
             * @brief RPC callback for detaching from a process
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing detach rpc message
             */
            static void OnDetach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for writing kernel memory
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage with single step message
             */
            static void OnThreadSinglestep(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for getting a process list of all running processes on the console
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing process list request message
             */
            static void OnGetProcList(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for reading attached process memory
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing rpm request
             */
            static void OnReadProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
            
            /**
             * @brief RPC callback for writing attached process memory
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing wpm request
             */
            static void OnWriteProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for changing memory protection
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing protect process memory request
             */
            static void OnProtectProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for getting process information
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the pid request
             */
            static void OnGetProcessInfo(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for getting thread information
             *
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the thread id request
             */
            static void OnGetThreadInfo(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for getting process threads
             *
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the pid request
             */
            static void OnGetProcThreads(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for allocating memory in a process
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the size and permission
             */
            static void OnAllocateProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for freeing memory in a process
             * 
             * @param p_Connection Requesing connection
             * @param p_Message RpcMessage containing the memory to free
             */
            static void OnFreeProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for adding a breakpoint
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the breakpoint to add
             */
            static void OnAddBreakpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for adding a watchpoint
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the watchpoint to add
             */
            static void OnAddWatchpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for signaling the process
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the signal information
             */
            static void OnSignalProcess(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for getting thread registers
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the thread id to get registers for
             */
            static void OnGetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for setting the thread registers
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage containing the set thread registers message
             */
            static void OnSetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for reading kernel memory
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage with the read kernel memory request
             */
            static void OnReadKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

            /**
             * @brief RPC callback for writing kernel memory
             * 
             * @param p_Connection Requesting connection
             * @param p_Message RpcMessage with write kernel memory
             */
            static void OnWriteKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
        };
    }
}