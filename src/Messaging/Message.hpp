#pragma once
#include <Utils/Types.hpp>
#include <Utils/SharedPtr.hpp>

namespace Mira
{
    namespace Messaging
    {
        enum 
        { 
            MessageHeader_Magic = 0x2,
            MessageHeader_MaxPayloadSize = 0x8000,
        };
        enum MessageCategory
        {
            MessageCategory_None = 0,
            MessageCategory_System,
            MessageCategory_Log,
            MessageCategory_Debug,
            MessageCategory_File,
            MessageCategory_Command,
            MessageCategory_Max = 14
        };

        struct MessageHeader
        {
            uint64_t magic : 2;
            uint64_t category : 4;
            uint64_t isRequest : 1;
            uint64_t errorType : 32;
            uint64_t payloadLength : 16;
            uint64_t padding : 9;
        };

        struct Message : public MessageHeader
        {
            uint8_t payload[];

            Message(uint32_t p_PayloadSize = 0, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);
            ~Message();

            static shared_ptr<Message> Create(uint32_t p_PayloadSize = 0, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);
        };
    }
}