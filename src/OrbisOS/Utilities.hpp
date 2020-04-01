#pragma once
#include <Utils/Types.hpp>
#include <Utils/Kernel.hpp>

struct proc;
struct thread;

namespace Mira
{
    namespace OrbisOS
    {
        class Utilities
        {
        private:
            static Utilities* m_Instance;

            Utilities();

        public:
            static Utilities* GetInstance();

            static void HookFunctionCall(uint8_t* p_HookTrampoline, void* p_Function, void* p_Address);
            static uint64_t PtraceIO(int32_t p_ProcessId, int32_t p_Operation, void* p_DestAddress, void* p_ToReadWriteAddress, size_t p_ToReadWriteSize);
            static struct ::proc* FindProcessByName(const char* name);

            static int ProcessReadWriteMemory(struct ::proc* p_Process, void* p_DestAddress, size_t p_Size, void* p_ToReadWriteAddress, size_t* p_ReadWriteSize, bool p_Write);
            static int GetProcessVmMap(struct ::proc* p_Process, ProcVmMapEntry** p_Entries, size_t* p_NumEntries);
            static int MountNullFS(char* where, char* what, int flags, struct thread* td);
        };
    }
}