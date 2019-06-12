#include "FileManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>

#include <Messaging/Message.hpp>
#include <Messaging/MessageManager.hpp>

#include <Messaging/Rpc/Server.hpp>

#include <Mira.hpp>

#include "FileManagerMessages.hpp"

#include <sys/dirent.h>
#include <sys/stat.h>

using namespace Mira::Plugins;

FileManager::FileManager() :
    m_Buffer(0x10000)
{
}

FileManager::~FileManager()
{
    // Delete all of the resources

    // Zero the buffer
    m_Buffer.zero();
}

bool FileManager::OnLoad()
{
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Echo, OnEcho);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Open, OnOpen);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Close, OnClose);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Read, OnRead);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Write, OnWrite);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_GetDents, OnGetDents);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Stat, OnStat);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_MkDir, OnMkDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_RmDir, OnRmDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Unlink, OnUnlink);
    
    return true;
}

bool FileManager::OnUnload()
{
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Echo, OnEcho);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Open, OnOpen);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Close, OnClose);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Read, OnRead);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Write, OnWrite);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_GetDents, OnGetDents);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Stat, OnStat);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_MkDir, OnMkDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_RmDir, OnRmDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Unlink, OnUnlink);
    return true;
}

void FileManager::OnEcho(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }

    if (p_Message->GetPayloadLength() < sizeof(struct fileexplorer_echoRequest_t))
    {
        WriteLog(LL_Error, "no message to echo");
        return;
    }
    auto& s_Payload = p_Message->GetPayload();

    s_Payload.setOffset(0);
    auto s_Request = s_Payload.get_struct<struct fileexplorer_echoRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not get request");
        return;
    }

    if (s_Payload.size() < s_Request->length + s_Payload.getOffset())
    {
        WriteLog(LL_Error, "message length too long");
        return;
    }
    WriteLog(LL_Error, "echo: (%s).", s_Request->message);
}

void FileManager::OnOpen(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_Payload = p_Message->GetPayload();

    s_Payload.setOffset(0);
    auto s_Request = s_Payload.get_struct<struct fileexplorer_openRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message->GetPayloadLength() < s_Request->pathLength + s_Payload.getOffset())
    {
        WriteLog(LL_Error, "invalid path length");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

	WriteLog(LL_Debug, "open: (%s) (%d) (%d)", s_Request->path, s_Request->flags, s_Request->mode);

    int32_t s_Ret = kopen(s_Request->path, s_Request->flags, s_Request->mode);

    auto s_Response = Messaging::Message::Create(0, Messaging::MessageCategory_File, s_Ret, false);
    if (!s_Response)
    {
        WriteLog(LL_Error, "could not allocate response message.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(s_Response);
}


void FileManager::OnClose(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_Payload = p_Message->GetPayload();

    s_Payload.setOffset(0);
    auto s_Request = s_Payload.get_struct<struct fileexplorer_closeRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    kclose(s_Request->handle);

    auto s_Response = Messaging::Message::Create(0, Messaging::MessageCategory_File, 0, false);
    if (!s_Response)
    {
        WriteLog(LL_Error, "could not allocate response message.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(s_Response);
}

void FileManager::OnRead(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_RequestPayload = p_Message->GetPayload();

    s_RequestPayload.setOffset(0);
    auto s_Request = s_RequestPayload.get_struct<struct fileexplorer_readRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    Span<uint8_t> s_Buffer(0x4000);
    if (!s_Buffer)
    {
        WriteLog(LL_Error, "could not allocate buffer");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOBUFS);
        return;
    }

    if (s_Request->count > s_Buffer.size())
    {
        WriteLog(LL_Error, "too large of request (%d) max (%d).", s_Request->count, s_Buffer.size());
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOBUFS);
        return;
    }

    auto s_Ret = kread(s_Request->handle, s_Buffer.data(), s_Request->count);
    if (s_Ret < 0 || s_Request->count != s_Ret)
    {
        WriteLog(LL_Error, "could not read (%d).", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_Ret);
        return;
    }

    auto s_PayloadSize = s_Request->count + sizeof(struct fileexplorer_readResponse_t);
    auto s_ResponseMessage = Messaging::Message::Create(s_PayloadSize, Messaging::MessageCategory_File, s_Ret, false);
    if (!s_ResponseMessage)
    {
        WriteLog(LL_Error, "could not allocate response");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_ResponsePayload = s_ResponseMessage->GetPayload();
    auto s_Response = s_ResponsePayload.get_struct<struct fileexplorer_readResponse_t>();
    if (s_Response == nullptr)
    {
        WriteLog(LL_Error, "could not get response");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    s_Response->count = s_Request->count;
    
    // Copy the response payload
    memcpy(s_Response->data, s_Buffer.data(), s_Request->count);

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(s_ResponseMessage);

    WriteLog(LL_Debug, "read: fd: (%d) data: (%p) cnt: (%d).", s_Request->handle, s_Buffer.data(), s_Request->count);
}

void FileManager::OnGetDents(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_Payload = p_Message->GetPayload();

    s_Payload.setOffset(0);
    auto s_Request = s_Payload.get_struct<struct fileexplorer_getdentsRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (s_Payload.size() < s_Request->length + sizeof(struct fileexplorer_getdentsRequest_t) || s_Request->length == 0)
    {
        WriteLog(LL_Error, "path length out of bounds.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_DirectoryHandle = kopen(s_Request->path, 0x0000 | 0x00020000, 0777);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", s_Request->path, s_DirectoryHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_DirectoryHandle);
        return;
    }

    uint64_t s_DentCount = 0;
    Span<uint8_t> s_DentBuffer(0x8000);
    if (!s_DentBuffer)
    {
        WriteLog(LL_Error, "could not allocate dent buffer.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    int32_t s_ReadCount = 0;
    for (;;)
    {
        s_ReadCount = kgetdents(s_DirectoryHandle, reinterpret_cast<char*>(s_DentBuffer.data()), s_DentBuffer.size());
        if (s_ReadCount <= 0)
            break;
        
        for (auto i = 0; i < s_ReadCount;)
        {
            auto l_Dent = reinterpret_cast<struct dirent*>(s_DentBuffer.data() + i);
            s_DentCount++;

            i += l_Dent->d_reclen;
        }
    }
    kclose(s_DirectoryHandle);

    // Send the response with the dent count out
    {
        struct fileexplorer_getdentsResponse_t s_DentResponse =
        {
            .totalDentCount = s_DentCount,
        };

        auto s_Response = Messaging::Message::Create(sizeof(struct fileexplorer_getdentsResponse_t), Messaging::MessageCategory_File, 0, false);
        if (!s_Response)
        {
            WriteLog(LL_Error, "could not allocate response");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
            return;
        }

        auto& s_ResponsePayload = s_Response->GetPayload();
        if (!s_ResponsePayload.set_buffer(0, reinterpret_cast<uint8_t*>(&s_DentResponse), sizeof(s_DentResponse)))
        {
            WriteLog(LL_Error, "could not write payload");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
            return;
        }

        Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(s_Response);
    }
    
    // Zero out the dent buffer
    s_DentBuffer.zero();
    
    s_DirectoryHandle = kopen(s_Request->path, 0x0000 | 0x00020000, 0777);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", s_Request->path, s_DirectoryHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_DirectoryHandle);
        return;
    }

    auto s_ConnectionSocket = Mira::Framework::GetFramework()->GetRpcServer()->Deprecated_GetSocketByThread(curthread);
    if (s_ConnectionSocket < 0)
    {
        WriteLog(LL_Error, "rpcserver could not get socket (%d)", s_ConnectionSocket);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_ConnectionSocket);
        return;
    }

    s_ReadCount = 0;
    for (;;)
    {
        bool l_Error = false;

        s_ReadCount = kgetdents(s_DirectoryHandle, reinterpret_cast<char*>(s_DentBuffer.data()), s_DentBuffer.size());
        if (s_ReadCount <= 0)
            break;
        
        for (auto i = 0; i < s_ReadCount;)
        {
            auto l_Dent = reinterpret_cast<struct dirent*>(s_DentBuffer.data() + i);

            uint32_t l_AllocatedDentSize = sizeof(struct fileexplorer_dent_t) + l_Dent->d_namlen;
            Span<uint8_t> l_AllocatedDent(l_AllocatedDentSize);
            if (!l_AllocatedDent)
            {
                l_Error = true;
                break;
            }

            auto l_AllocatedDentHeader = l_AllocatedDent.get_struct<struct fileexplorer_dent_t>();
            if (l_AllocatedDentHeader == nullptr)
            {
                l_Error = true;
                break;
            }

            l_AllocatedDentHeader->fileno = l_Dent->d_fileno;
            l_AllocatedDentHeader->reclen = l_Dent->d_reclen;
            l_AllocatedDentHeader->type = l_Dent->d_type;
            l_AllocatedDentHeader->namlen = l_Dent->d_namlen;
            (void)memcpy(l_AllocatedDentHeader->name, l_Dent->d_name, l_Dent->d_namlen);

            kwrite(s_ConnectionSocket, l_AllocatedDent.data(), l_AllocatedDent.size());
            
            i += l_Dent->d_reclen;
        }

        if (l_Error)
            break;
    }
}

void FileManager::OnStat(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto& s_RequestPayload = p_Message->GetPayload();

    s_RequestPayload.setOffset(0);
    auto s_Request = s_RequestPayload.get_struct<struct fileexplorer_statRequest_t>();
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    // Check to see if we have a valid file handle
    struct stat s_Stat = { 0 };

    if (s_Request->handle < 0)
    {
        if (s_Request->pathLength == 0)
        {
            WriteLog(LL_Error, "invalid path length");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOENT);
            return;
        }

        if (s_RequestPayload.getRemainingBytes() < s_Request->pathLength)
        {
            WriteLog(LL_Error, "path length too long for remaining data.");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOENT);
            return;
        }

        int32_t s_Ret = kstat(s_Request->path, &s_Stat);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->path, s_Ret);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_Ret);
            return;
        }
    }
    else
    {
        auto s_Ret = kfstat(s_Request->handle, &s_Stat);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->path, s_Ret);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, s_Ret);
            return;
        }
    }
    
    // Send a success response back
	struct fileexplorer_stat_t s_ResponseData = 
	{
		.st_dev = s_Stat.st_dev,
		.st_ino = s_Stat.st_ino,
		.st_mode = s_Stat.st_mode,
		.st_nlink = s_Stat.st_nlink,
		.st_uid = s_Stat.st_uid,
		.st_gid = s_Stat.st_gid,
		.st_rdev = s_Stat.st_rdev,
		.st_atim = 
		{
			.tv_sec = s_Stat.st_atim.tv_sec,
			.tv_nsec = s_Stat.st_atim.tv_nsec
		},
		.st_mtim =
		{
			.tv_sec = s_Stat.st_mtim.tv_sec,
			.tv_nsec = s_Stat.st_mtim.tv_nsec
		},
		.st_ctim = 
		{
			.tv_sec = s_Stat.st_ctim.tv_sec,
			.tv_nsec = s_Stat.st_ctim.tv_nsec
		},
		.st_size = s_Stat.st_size,
		.st_blocks = s_Stat.st_blocks,
		.st_blksize = s_Stat.st_blksize,
		.st_flags = s_Stat.st_flags,
		.st_gen = s_Stat.st_gen,
		.st_lspare = s_Stat.st_lspare,
		.st_birthtim =
		{
			.tv_sec = s_Stat.st_birthtim.tv_sec,
			.tv_nsec = s_Stat.st_birthtim.tv_nsec
		}
	};

    auto s_Response = Messaging::Message::Create(sizeof(s_ResponseData), Messaging::MessageCategory_File, 0, false);
    
    auto& s_ResponsePayload = s_Response->GetPayload();
    if (!s_ResponsePayload)
    {
        WriteLog(LL_Error, "could not get allocate payload");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (!s_ResponsePayload.set_buffer(0, reinterpret_cast<uint8_t*>(&s_ResponseData), sizeof(s_ResponseData)))
    {
        WriteLog(LL_Error, "could not write response data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(s_Response);
}