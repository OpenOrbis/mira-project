#pragma once
#include "MessageHeader.hpp"

namespace Mira
{
    namespace Messaging
    {
        typedef struct _Message
        {
            Messaging::MessageHeader Header;
            const uint8_t* Buffer;
        } Message;
    }
}