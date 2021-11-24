#pragma once
#include "Status.hpp"

#ifdef _PROTOBUF
#include "Protos/Rpc.pb.h"
#endif

#include <Utils/Logger.hpp>

namespace Mira
{
    namespace Rpc
    {
        class RpcMessage;
        enum class Status;

        class Listener
        {
        protected:
            #ifdef _PROTOBUF
            google::protobuf::Arena* m_Arena;
            #endif

        public:
        #ifdef _PROTOBUF
            Listener(google::protobuf::Arena* p_Arena) :
                m_Arena(p_Arena)
        #endif
            Listener(void* p_Pointer)
            {
                
            }

            virtual ~Listener() { }
            virtual Status OnMessage(RpcMessage* p_Request, RpcMessage* p_Response) = 0;

            template<typename T>
            void SetInnerMessage(RpcMessage* p_Message, T& p_InnerMessage)
            {
            #if _PROTOBUF
                auto s_Any = google::protobuf::Arena::CreateMessage<google::protobuf::Any>(m_Arena);
                if (!s_Any)
                {
                    WriteLog(LL_Error, "could not create new any.");
                    return;
                }

                s_Any->PackFrom(p_InnerMessage);

                p_Message->set_allocated_inner_message(s_Any);
            #endif
            }
        };
    }
}