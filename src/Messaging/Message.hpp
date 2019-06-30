#pragma once
#include <Utils/Types.hpp>
#include <Utils/SharedPtr.hpp>
#include <Utils/Span.hpp>

#include "MessageHeader.hpp"
#include "MessageCategory.hpp"

namespace Mira
{
    namespace Messaging
    {
        enum 
        { 
            MessageHeader_Magic = 0x2,
            MessageHeader_MaxPayloadSize = 0x4000,
        };

        class Message
        {
        private:
            MessageHeader m_Header;
            uint8_t* m_PayloadData;
            Span<uint8_t> m_Payload;
            
        public:
            Message();
            Message(uint32_t p_PayloadSize = 0, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);
            Message(uint8_t* p_PayloadBuffer, uint32_t p_PayloadSize, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);
            ~Message();

            static shared_ptr<Message> Create(uint32_t p_PayloadSize = 0, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);
            static shared_ptr<Message> Create(uint8_t* p_PayloadBuffer, uint32_t p_PayloadSize, MessageCategory p_Category = MessageCategory_None, int32_t p_ErrorType = 0, bool p_IsRequest = false);

            MessageHeader* GetHeader() { return &m_Header; }
            Span<uint8_t>& GetPayload() { return m_Payload; }
            uint8_t* GetPayloadData() { return m_Payload.data(); }
            uint8_t GetMagic() { return m_Header.magic; }
            uint16_t GetCategory() { return m_Header.category; }
            bool IsRequest() { return m_Header.isRequest; }
            int32_t GetErrorType() { return m_Header.errorType; }
            uint32_t GetPayloadLength() { return m_Payload.size(); }
        };
    }
}