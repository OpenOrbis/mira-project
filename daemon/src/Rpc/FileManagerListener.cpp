#include "FileManagerListener.hpp"
#include "Protos/FileManager.pb.h"
#include <Utils/Logger.hpp>

using namespace Mira::Rpc;

FileManagerListener::FileManagerListener(google::protobuf::Arena* p_Arena) :
    Listener(p_Arena)
{

}

Status FileManagerListener::OnMessage(RpcMessage* p_Request, RpcMessage* p_Response)
{
    if (p_Request->inner_message().Is<FileManager::EchoRequest>())
        return OnEcho(p_Request, p_Response);
    
    // By default if we don't have the message we are looking for report that we skipped
    // This will ensure that the manager calls the next listener
    return Status::SKIPPED;
}

Status FileManagerListener::OnEcho(RpcMessage* p_Request, RpcMessage* p_Response)
{
    // Parse the incoming message
    FileManager::EchoRequest s_Request;
    if (!p_Request->inner_message().UnpackTo(&s_Request))
    {
        WriteLog(LL_Error, "could not parse echo request.");
        return Status::CANCELLED;
    }

    WriteLog(LL_Info, "[Echo]: %s", s_Request.message().c_str());

    // Clear the error on the response
    p_Response->set_error(0);
    
    return Status::OK;
}