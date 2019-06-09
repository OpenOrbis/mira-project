#include "Message.hpp"
#include <Utils/Logger.hpp>

using namespace Mira::Messaging;

Message::Message() :
    m_Header { 0 },
    m_Payload()
{

}

Message::Message(uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest)
{
    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return;
    }

    m_Payload = Span<uint8_t>(p_PayloadSize);
    m_Header = MessageHeader
    {
        .magic = MessageHeader_Magic,
        .category = p_Category,
        .isRequest = p_IsRequest,
        .errorType = static_cast<uint64_t>(p_ErrorType),
        .payloadLength = p_PayloadSize,
        .padding = 0
    };

    WriteLog(LL_Debug, "header magic: (%d) category: (%d).", m_Header.magic, m_Header.category);

}

Message::~Message()
{
    memset(&m_Header, 0, sizeof(m_Header));
    m_Payload.zero();
}

shared_ptr<Message> Message::Create(uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest)
{
    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return shared_ptr<Message>(nullptr);
    }

    auto s_Message = new Message(p_PayloadSize, p_Category, p_ErrorType, p_IsRequest);
    if (s_Message == nullptr)
    {
        WriteLog(LL_Error, "could not allocate new message");
        return shared_ptr<Message>(nullptr);
    }

    return shared_ptr<Message>(s_Message);
}