#pragma once
#include "MessageCategory.hpp"
#include <Utils/SharedPtr.hpp>

namespace Mira
{
    namespace Messaging
    {
        class Message;

        class MessageListener
        {
        private:
            MessageCategory m_Category;
            int32_t m_Type;
            void(*m_Callback)(shared_ptr<Message> p_Message);

        public:
            MessageListener() :
                m_Category(MessageCategory_None),
                m_Type(0),
                m_Callback(nullptr)
            {

            }

            MessageListener(MessageCategory p_Category, int32_t p_Type, void(*p_Callback)(shared_ptr<Message>)) :
                m_Category(p_Category),
                m_Type(p_Type),
                m_Callback(p_Callback)
            {

            }

            ~MessageListener()
            {
                m_Category = MessageCategory_None;
                m_Type = 0;
                m_Callback = nullptr;
            }

            // Copy constructor
            MessageListener(const MessageListener& p_Other) :
                m_Category(p_Other.m_Category),
                m_Type(p_Other.m_Type),
                m_Callback(p_Other.m_Callback)
            {

            }

            // Copy assignment
            MessageListener& operator=(const MessageListener& p_Other)
            {
                if (&p_Other == this)
                    return *this;
                
                m_Category = p_Other.m_Category;
                m_Type = p_Other.m_Type;
                m_Callback = p_Other.m_Callback;

                return *this;
            }

            // Move constructor
            MessageListener(MessageListener&& p_Other) :
                m_Category(p_Other.m_Category),
                m_Type(p_Other.m_Type),
                m_Callback(p_Other.m_Callback)
            {

            }

            // Move assignment
            MessageListener& operator=(MessageListener&& p_Other)
            {
                if (&p_Other == this)
                    return *this;
                
                m_Category = p_Other.m_Category;
                m_Type = p_Other.m_Type;
                m_Callback = p_Other.m_Callback;

                return *this;
            }

            MessageCategory GetCategory() const { return m_Category; }
            int32_t GetType() const { return m_Type; }
            auto GetCallback() -> void(*)(shared_ptr<Message> p_Message)
            {
                return m_Callback;
            };

            void Zero()
            {
                m_Category = MessageCategory_None;
                m_Type = 0;
                m_Callback = nullptr;
            }
        };
    }
}