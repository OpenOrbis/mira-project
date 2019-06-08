#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Messaging
    {
        enum MessageCategory : uint32_t
        {
            MessageCategory_None = 0,
            MessageCategory_System,
            MessageCategory_Log,
            MessageCategory_Debug,
            MessageCategory_File,
            MessageCategory_Command,
            MessageCategory_Max = 14
        };
    }
}