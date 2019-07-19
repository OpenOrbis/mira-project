#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Messaging
    {
        enum { MessageHeaderMagic = 2 };
        typedef struct _MessageHeader
        {
            uint64_t magic : 2;
            uint64_t category : 4;
            uint64_t isRequest : 1;
            uint64_t errorType : 32;
            uint64_t payloadLength : 16;
            uint64_t padding : 9;
        } MessageHeader;
    }
}