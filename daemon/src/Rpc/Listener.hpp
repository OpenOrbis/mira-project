#pragma once
#include "Status.hpp"

#include "Protos/Rpc.pb.h"
#include <Utils/Logger.hpp>

namespace Mira
{
    namespace Rpc
    {
        enum class Status;

        class Listener
        {
        protected:
            google::protobuf::Arena* m_Arena;

        public:
            Listener(google::protobuf::Arena* p_Arena) :
                m_Arena(p_Arena)
            {
                
            }

            virtual Status OnMessage(RpcMessage* p_Request, RpcMessage* p_Response) = 0;

            template<typename T>
            void SetInnerMessage(RpcMessage* p_Message, T& p_InnerMessage)
            {
                auto s_Any = google::protobuf::Arena::CreateMessage<google::protobuf::Any>(m_Arena);
                if (!s_Any)
                {
                    WriteLog(LL_Error, "could not create new any.");
                    return;
                }

                s_Any->PackFrom(p_InnerMessage);

                p_Message->set_allocated_inner_message(s_Any);
            }
        };
    }
}