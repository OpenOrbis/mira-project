#include "FileManagerListener.hpp"
#include "Protos/FileManager.pb.h"

extern "C"
{
    #include <fcntl.h>
    #include <unistd.h>
    #include <dirent.h>
}
using namespace Mira::Rpc;

FileManagerListener::FileManagerListener(google::protobuf::Arena* p_Arena) :
    Listener(p_Arena)
{

}

FileManagerListener::~FileManagerListener()
{
    
}

Status FileManagerListener::OnMessage(RpcMessage* p_Request, RpcMessage* p_Response)
{
    if (p_Request->inner_message().Is<FileManager::EchoRequest>())
        return OnEcho(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::OpenRequest>())
        return OnOpen(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::CloseRequest>())
        return OnClose(p_Request, p_Response);

    if (p_Request->inner_message().Is<FileManager::ReadRequest>())
        return OnRead(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::ListRequest>())
        return OnList(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::StatRequest>())
        return OnStat(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::MkdirRequest>())
        return OnMkdir(p_Request, p_Response);
    
    if (p_Request->inner_message().Is<FileManager::UnlinkRequest>())
        return OnUnlink(p_Request, p_Response);
    
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

Status FileManagerListener::OnOpen(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::OpenRequest s_Request;
    if (!p_Request->inner_message().UnpackTo(&s_Request))
    {
        WriteLog(LL_Error, "could not parse open request.");
        return Status::CANCELLED;
    }

    WriteLog(LL_Debug, "open request p:(%s) m:(%d) f:(%d).", s_Request.path().c_str(), s_Request.mode(), s_Request.flags());

    auto s_Ret = open(s_Request.path().c_str(), s_Request.flags(), s_Request.mode());
    if (s_Ret == -1)
    {
        p_Response->set_error(errno);
        WriteLog(LL_Error, "could not open file err: (%d).", errno);
        return Status::OK;
    }

    FileManager::OpenResponse s_OpenResponse;
    s_OpenResponse.set_fd(s_Ret);

    SetInnerMessage<FileManager::OpenResponse>(p_Response, s_OpenResponse);

    // Clear the error
    p_Response->set_error(0);

    return Status::OK;
}

Status FileManagerListener::OnClose(RpcMessage* p_Request, RpcMessage* p_Response)
{
    // Unserialize the incoming request
    FileManager::CloseRequest s_Request;
    if (!p_Request->inner_message().UnpackTo(&s_Request))
    {
        WriteLog(LL_Error, "could not parse close request.");
        return Status::CANCELLED;
    }

    // Debug logging
    WriteLog(LL_Debug, "close request: fd: (%d).", s_Request.handle());

    // Close the specified handle
    auto s_Ret = close(s_Request.handle());
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not close handle (%d), err: (%d).", s_Request.handle(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    p_Response->set_error(0);
    return Status::OK;
}

Status FileManagerListener::OnRead(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::ReadRequest s_ReadRequest;
    if (!p_Request->inner_message().UnpackTo(&s_ReadRequest))
    {
        WriteLog(LL_Error, "could not parse read request.");
        return Status::CANCELLED;
    }

    // Debug logging
    WriteLog(LL_Debug, "read request: fd: (%d), size: (%lx).", s_ReadRequest.handle(), s_ReadRequest.size());

    // Validate max size
    if (s_ReadRequest.size() > MaxBufferSize)
    {
        WriteLog(LL_Error, "requested size (%lx) > max size (%lx).", s_ReadRequest.size(), MaxBufferSize);
        p_Response->set_error(EMSGSIZE);
        return Status::OK;
    }

    // Create a buffer to hold our data
    std::string s_Buffer;
    s_Buffer.resize(s_ReadRequest.size());

    // Read the data out
    auto s_Ret = read(s_ReadRequest.handle(), (void*)s_Buffer.data(), s_ReadRequest.size());
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not read (%lx) from fd: (%d), err: (%d).", s_ReadRequest.size(), s_ReadRequest.handle(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    // Read from specified handle
    FileManager::ReadResponse s_ReadResponse;
    s_ReadResponse.set_data(s_Buffer);

    SetInnerMessage(p_Response, s_ReadResponse);
    p_Response->set_error(0);

    return Status::OK;
}

Status FileManagerListener::OnList(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::ListRequest s_ListRequest;
    if (!p_Request->inner_message().UnpackTo(&s_ListRequest))
    {
        WriteLog(LL_Error, "could not parse list request.");
        return Status::CANCELLED;
    }

    // Attempt to open the directory
    auto s_Directory = opendir(s_ListRequest.path().c_str());
    if (s_Directory == nullptr)
    {
        WriteLog(LL_Error, "could not open directory (%s), err: (%d).", s_ListRequest.path().c_str(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    FileManager::ListResponse s_ListResponse;
    auto s_Entries = s_ListResponse.mutable_entries();

    struct dirent* s_DirEnt = nullptr;
    while ((s_DirEnt = readdir(s_Directory)))
    {
        auto l_Entry = s_Entries->Add();
        l_Entry->set_name(s_DirEnt->d_name);
        // l_Entry->set_inode(s_DirEnt->d_ino);
        // l_Entry->set_offset(s_DirEnt->d_off);

        switch (s_DirEnt->d_type)
        {
        case DT_BLK:
            l_Entry->set_type(FileManager::ListTypes::BLOCK_DEVICE);
            break;
        case DT_CHR:
            l_Entry->set_type(FileManager::ListTypes::CHARACTER_DEVICE);
            break;
        case DT_DIR:
            l_Entry->set_type(FileManager::ListTypes::DIRECTORY);
            break;
        case DT_FIFO:
            l_Entry->set_type(FileManager::ListTypes::NAMED_PIPE);
            break;
        default:
            l_Entry->set_type(FileManager::ListTypes::UNKNOWN);
        };
        
    }
    
    auto s_Ret = closedir(s_Directory);
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not close directory err: (%d).", errno);
        p_Response->set_error(0);
        return Status::OK;
    }

    p_Response->set_error(0);
    SetInnerMessage<FileManager::ListResponse>(p_Response, s_ListResponse);

    return Status::OK;
}

Status FileManagerListener::OnStat(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::StatRequest s_StatRequest;
    if (!p_Request->inner_message().UnpackTo(&s_StatRequest))
    {
        WriteLog(LL_Error, "could not parse stat request.");
        return Status::CANCELLED;
    }

    struct stat s_Stat = { 0 };
    auto s_Ret = stat(s_StatRequest.path().c_str(), &s_Stat);
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not stat (%s), err: (%d).", s_StatRequest.path().c_str(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    FileManager::StatResponse s_StatResponse;
    s_StatResponse.set_device(s_Stat.st_dev);
    s_StatResponse.set_inode(s_Stat.st_ino);
    s_StatResponse.set_protection(s_Stat.st_mode);
    s_StatResponse.set_num_hard_links(s_Stat.st_nlink);
    s_StatResponse.set_user_id(s_Stat.st_uid);
    s_StatResponse.set_group_id(s_Stat.st_gid);
    s_StatResponse.set_device_id(s_Stat.st_rdev);
    s_StatResponse.set_size(s_Stat.st_size);
    s_StatResponse.set_block_size(s_Stat.st_blksize);
    s_StatResponse.set_blocks(s_Stat.st_blocks);

    auto s_AccessTime = google::protobuf::Arena::CreateMessage<FileManager::Time>(m_Arena);
    if (s_AccessTime == nullptr)
    {
        WriteLog(LL_Error, "could not allocate access time.");
        p_Response->set_error(ENOMEM);
        return Status::OK;
    }
    s_AccessTime->set_seconds(s_Stat.st_atim.tv_sec);
    s_AccessTime->set_nanoseconds(s_Stat.st_atim.tv_nsec);
    s_StatResponse.set_allocated_access_time(s_AccessTime);

    auto s_ModTime = google::protobuf::Arena::CreateMessage<FileManager::Time>(m_Arena);
    if (s_ModTime == nullptr)
    {
        WriteLog(LL_Error, "could not allocate mod time.");
        p_Response->set_error(ENOMEM);
        return Status::OK;
    }
    s_ModTime->set_seconds(s_Stat.st_mtim.tv_sec);
    s_ModTime->set_nanoseconds(s_Stat.st_mtim.tv_nsec);
    s_StatResponse.set_allocated_mod_time(s_ModTime);

    auto s_StatusTime = google::protobuf::Arena::CreateMessage<FileManager::Time>(m_Arena);
    if (s_StatusTime == nullptr)
    {
        WriteLog(LL_Error, "could not allocate status time.");
        p_Response->set_error(ENOMEM);
        return Status::OK;
    }
    s_StatusTime->set_seconds(s_Stat.st_ctim.tv_sec);
    s_StatusTime->set_nanoseconds(s_Stat.st_ctim.tv_nsec);
    s_StatResponse.set_allocated_status_time(s_StatusTime);
    
    SetInnerMessage<FileManager::StatResponse>(p_Response, s_StatResponse);
    p_Response->set_error(0);
    
    return Status::OK;
}

Status FileManagerListener::OnMkdir(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::MkdirRequest s_Request;
    if (!p_Request->inner_message().UnpackTo(&s_Request))
    {
        WriteLog(LL_Error, "could not parse mkdir request.");
        return Status::CANCELLED;
    }

    auto s_Ret = mkdir(s_Request.path().c_str(), s_Request.mode());
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not mkdir (%s), err: (%d).", s_Request.path().c_str(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    p_Response->set_error(0);

    return Status::OK;
}

Status FileManagerListener::OnUnlink(RpcMessage* p_Request, RpcMessage* p_Response)
{
    FileManager::UnlinkRequest s_UnlinkRequest;
    if (!p_Request->inner_message().UnpackTo(&s_UnlinkRequest))
    {
        WriteLog(LL_Error, "could not parse unlink request.");
        return Status::CANCELLED;
    }

    auto s_Ret = unlink(s_UnlinkRequest.path().c_str());
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not unlink (%s), err: (%d).", s_UnlinkRequest.path().c_str(), errno);
        p_Response->set_error(errno);
        return Status::OK;
    }

    p_Response->set_error(0);
    
    return Status::OK;
}