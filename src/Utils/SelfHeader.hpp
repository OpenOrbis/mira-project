#pragma once
#include "Types.hpp"

namespace Mira
{
    namespace Utils
    {
        typedef struct _SelfHeader
        {
            uint8_t Unknown00[0x18];
            uint16_t EntityCount;
        } SelfHeader;
    }
}