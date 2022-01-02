#pragma once
#include <cstdint>
#include <unordered_map>

namespace Mira
{
    namespace Processes
    {
        class Thread;

        class Process
        {
        private:
            std::unordered_map<int32_t, Thread> m_Threads;

        protected:
            int32_t m_ProcessId;
            int32_t m_Signal;

            std::string m_Name;
            std::string m_ElfPath;
            std::string m_RandomizedPath;

        private:
            bool Internal_UpdateProcess();

        public:
            Process(int32_t p_ProcessId);

            bool ReadMemory(void* p_Address, uint8_t* p_OutData, uint32_t p_Size);
            bool WriteMemory(void* p_Address, uint8_t* p_InData, uint32_t p_Size);

            static Process* GetCurrentProcess();
            static Process* GetProcessById(int32_t p_ProcessId);
            static Process* GetProcessByName(const char* p_ProcessName);
        };
    }
}