#include "Message.hpp"
#include <Utils/Logger.hpp>

using namespace Mira::Messaging;

Message::Message() :
    m_Header { 0 },
    m_PayloadData(nullptr),
    m_Payload()
{

}

Message::Message(uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest) :
    m_Header { 0 },
    m_PayloadData(nullptr),
    m_Payload()
{
    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return;
    }

    m_PayloadData = new uint8_t[p_PayloadSize];
    if (m_PayloadData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate payload data.");
        return;
    }

    m_Payload = Span<uint8_t>(m_PayloadData, p_PayloadSize);
    m_Payload.zero();

    m_Header = MessageHeader
    {
        .magic = MessageHeader_Magic,
        .category = p_Category,
        .isRequest = p_IsRequest,
        .errorType = static_cast<uint64_t>(p_ErrorType),
        .payloadLength = p_PayloadSize,
        .padding = 0
    };
}

Message::Message(uint8_t* p_PayloadBuffer, uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest)
{
    if (p_PayloadBuffer == nullptr)
    {
        WriteLog(LL_Error, "could not set payload buffer");
        return;
    }

    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return;
    }

    m_Payload = Span<uint8_t>(p_PayloadBuffer, p_PayloadSize);
    if (!m_Payload.isNull())
        m_Payload.set_buffer(0, p_PayloadBuffer, p_PayloadSize);
    
    m_Header = MessageHeader
    {
        .magic = MessageHeader_Magic,
        .category = p_Category,
        .isRequest = p_IsRequest,
        .errorType = static_cast<uint64_t>(p_ErrorType),
        .payloadLength = p_PayloadSize,
        .padding = 0
    };
}

Message::~Message()
{
    memset(&m_Header, 0, sizeof(m_Header));

    if (m_PayloadData != nullptr)
        delete [] m_PayloadData;
    
    m_PayloadData = nullptr;
}