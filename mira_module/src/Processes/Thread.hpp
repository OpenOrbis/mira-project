#pragma once
#include "x86_64regs.hpp"

#include <string>

namespace Mira
{
    namespace Processes
    {
        class Thread
        {
        protected:
            int32_t m_ThreadId;

            int32_t m_Error;

            int64_t m_ReturnValue;

            // Name of this thrad
            std::string m_Name;

            // General purpose registers (GPRs)
            GPRegisters m_GpRegisters;

            // Floating point registers (FPRs)
            FPRegisters m_FpRegisters;

            // Debug registers (DBRs)
            DBRegisters m_DbRegisters;

        private:
            // Update general purpose registers
            bool Internal_UpdateGprs();

            // Update floating point registers
            bool Intenral_UpdateFprs();

            // Update debug registers
            bool Internal_UpdateDbrs();

            // Update all current thread information
            bool Internal_UpdateThread();

        public:
            Thread(int32_t p_ProcessId);
            ~Thread();

            bool Step();
            bool Continue();
            bool Kill();
            bool Suspend();
            bool Resume();
        };
    }
}