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

            // Address of our trampoline, this calls the preable of the original function, then jumps mid-function to the destination
            // This way, we can "call original" without needing to keep flip-flopping the hook on/off
            void* m_TrampolineAddress;
            uint32_t m_TrampolineSize;

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
            void* GetTrampolineFunctionAddress(uint32_t* p_OutSize);

            static int32_t GetMinimumHookSize(void* p_Target);

            bool IsEnabled() { return m_Enabled; }

        private:
            uint8_t* CreateTrampoline(uint32_t* p_OutTrampolineSize);

            void* k_malloc(size_t p_Size);
            void k_free(void* p_Address);
        };
    }
}