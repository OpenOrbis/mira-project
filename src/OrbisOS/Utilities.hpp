#pragma once
#include <Utils/Types.hpp>

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
        };
    }
}