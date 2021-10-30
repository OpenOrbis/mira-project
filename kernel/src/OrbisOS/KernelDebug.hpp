#pragma once
#include <Utils/Types.hpp>
#include <Utils/Kernel.hpp>

namespace Mira
{
    namespace Debug
    {
        class KernelDebug
        {
        public:
            static void EnableTrap(int index, void* address, char dr_len, char dr_breaktype);
            static void DisableTrap(int index);
        };
    }
}
