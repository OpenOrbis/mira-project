#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Utils
    {
        class Hook
        {
        private:
            // Address of the target function
            void* m_TargetAddress;

            // Address of the hook
            void* m_HookAddress;

            // The backup data length for the jmp overwrite
            uint8_t* m_BackupData;
            uint32_t m_BackupLength;

            bool m_Enabled;

        public:
            Hook(void* p_TargetAddress, void* p_HookAddress);
            Hook();

            ~Hook();

            bool Enable();
            bool Disable();

            void* GetOriginalFunctionAddress();
            void* GetHookedFunctionAddress();

            static int32_t GetMinimumHookSize(void* p_Target);

            bool IsEnabled() { return m_Enabled; }
        };
    }
}