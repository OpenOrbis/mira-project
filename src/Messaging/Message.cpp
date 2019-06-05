#include "Message.hpp"
#include <Utils/Logger.hpp>

using namespace Mira::Messaging;

Message::Message(uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest)
{
    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return;
    }


}

shared_ptr<Message> Message::Create(uint32_t p_PayloadSize, MessageCategory p_Category, int32_t p_ErrorType, bool p_IsRequest)
{
    // Validate the maximum payload size
    if (p_PayloadSize > MessageHeader_MaxPayloadSize)
    {
        WriteLog(LL_Error, "could not allocate (%d) max payload size (%d).", p_PayloadSize, MessageHeader_MaxPayloadSize);
        return shared_ptr<Message>(nullptr);
    }

    auto s_Allocation = new uint8_t[(sizeof(Message) + p_PayloadSize)];
    if (s_Allocation == nullptr)
    {
        WriteLog(LL_Error, "could not allocate message and payload");
        return shared_ptr<Message>(nullptr);
    }

    auto s_Message = reinterpret_cast<Message*>(s_Allocation);
    s_Message->magic = MessageHeader_Magic;
    s_Message->category = p_Category;
    s_Message->isRequest = p_IsRequest;
    s_Message->errorType = p_ErrorType;
    s_Message->payloadLength = p_PayloadSize;
    s_Message->padding = 0;

    return shared_ptr<Message>(s_Message);
}