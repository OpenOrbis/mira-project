#pragma once
#include "Listener.hpp"

namespace Mira
{
    namespace Rpc
    {
        class FileManagerListener :
            public Listener
        {
        public:
            FileManagerListener(google::protobuf::Arena* p_Arena);
            virtual Status OnMessage(RpcMessage* p_Request, RpcMessage* p_Response) override;

            Status OnEcho(RpcMessage* p_Request, RpcMessage* p_Response);
        };
    }
}