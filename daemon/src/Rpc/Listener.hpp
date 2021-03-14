#pragma once
#include "Status.hpp"

#include "Protos/Rpc.pb.h"

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
        };
    }
}