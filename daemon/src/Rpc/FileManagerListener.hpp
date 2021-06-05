#pragma once
#include "Listener.hpp"

namespace Mira
{
    namespace Rpc
    {
        class FileManagerListener :
            public Listener
        {
        private:
            enum
            {
                MaxBufferSize = 0xFFFF
            };

        public:
            FileManagerListener(google::protobuf::Arena* p_Arena);
            virtual ~FileManagerListener();
            virtual Status OnMessage(RpcMessage* p_Request, RpcMessage* p_Response) override;

            Status OnEcho(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnOpen(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnClose(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnRead(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnList(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnStat(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnMkdir(RpcMessage* p_Request, RpcMessage* p_Response);
            Status OnUnlink(RpcMessage* p_Request, RpcMessage* p_Response);
        };
    }
}