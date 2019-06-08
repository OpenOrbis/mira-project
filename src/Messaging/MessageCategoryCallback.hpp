#pragma once
#include <Utils/SharedPtr.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Messaging
    {
        class Message;

        class MessageCategoryCallback
        {
        private:
            uint32_t m_Type;
            void(*m_Callback)(shared_ptr<Message> p_Message);

        public:
            MessageCategoryCallback() :
                m_Type(0),
                m_Callback(nullptr)
            {

            }

            MessageCategoryCallback(uint32_t p_Type, void(*p_Callback)(shared_ptr<Message>)) :
                m_Type(p_Type),
                m_Callback(p_Callback)
            { }

            ~MessageCategoryCallback()
            {
                m_Type = 0;
                m_Callback = nullptr;
            }

            uint32_t GetType() const { return m_Type; }
            auto GetCallback() -> void(*)(shared_ptr<Message>) {return m_Callback; }

            // Copy constructor
            MessageCategoryCallback(const MessageCategoryCallback& p_Other) :
                m_Type(p_Other.m_Type),
                m_Callback(p_Other.m_Callback)
            {

            }

            // Copy assignment
            MessageCategoryCallback& operator=(const MessageCategoryCallback& p_Other)
            {
                if (&p_Other == this)
                    return *this;
                
                m_Type = p_Other.m_Type;
                m_Callback = p_Other.m_Callback;

                return *this;
            }

            // Move constructor
            MessageCategoryCallback(MessageCategoryCallback&& p_Other) :
                m_Type(p_Other.m_Type),
                m_Callback(p_Other.m_Callback)
            {

            }

            // Move assignment
            MessageCategoryCallback& operator=(MessageCategoryCallback&& p_Other)
            {
                if (&p_Other == this)
                    return *this;
                
                m_Type = p_Other.m_Type;
                m_Callback = p_Other.m_Callback;

                return *this;
            }
        };
    }
}