#include "FileManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>

#include <Messaging/Message.hpp>
#include <Messaging/MessageManager.hpp>

#include <Messaging/Rpc/Server.hpp>
#include <Messaging/Rpc/Connection.hpp>

#include <Mira.hpp>

#include "FileManagerMessages.hpp"

#include <sys/dirent.h>
#include <sys/stat.h>

using namespace Mira::Plugins;
using namespace Mira::Plugins::FileManagerExtent;

FileManager::FileManager()
{
}

FileManager::~FileManager()
{
    // Delete all of the resources
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

void FileManager::OnEcho(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.Buffer == nullptr || p_Message.BufferLength < sizeof(EchoRequest))
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }

    auto s_Request = reinterpret_cast<const EchoRequest*>(p_Message.Buffer);
    if (s_Request->Length > MaxEchoLength)
    {
        WriteLog(LL_Error, "invalid length");
        return;
    }

    WriteLog(LL_Error, "echo: (%s).", s_Request->Message);
}

void FileManager::OnOpen(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.BufferLength < sizeof(OpenRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const OpenRequest*>(p_Message.Buffer);

	//WriteLog(LL_Debug, "open: (%s) (%d) (%d)", s_Request->path, s_Request->flags, s_Request->mode);

    int32_t s_Ret = kopen(s_Request->Path, s_Request->Flags, s_Request->Mode);

    Messaging::MessageHeader s_Header = {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = static_cast<uint64_t>(s_Ret),
        .payloadLength = 0,
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = &s_Header,
        .BufferLength = 0,
        .Buffer = nullptr
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}


void FileManager::OnClose(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.Buffer == nullptr || p_Message.BufferLength < sizeof(CloseRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const CloseRequest*>(p_Message.Buffer);
    kclose(s_Request->Handle);

    Messaging::MessageHeader s_Header = {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = 0,
        .payloadLength = 0,
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = &s_Header,
        .BufferLength = 0,
        .Buffer = nullptr
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnRead(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.Buffer == nullptr || p_Message.BufferLength < sizeof(ReadRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const ReadRequest*>(p_Message.Buffer);
    if (s_Request->Count > sizeof(ReadResponse::Buffer))
    {
        WriteLog(LL_Error, "read request size too large (%x) > (%x).", s_Request->Count, sizeof(ReadResponse::Buffer));
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -EOVERFLOW);
        return;
    }

    ReadResponse s_Payload = {
        .Count = 0,
        .Buffer = { 0 }
    };

    auto s_SizeRead = kread(s_Request->Handle, s_Payload.Buffer, s_Request->Count);
    if (s_SizeRead < 0)
    {
        WriteLog(LL_Error, "there was an error reading (%d).", s_SizeRead);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_SizeRead);
        return;
    }

    s_Payload.Count = static_cast<uint16_t>(s_SizeRead);

    Messaging::MessageHeader s_Header = {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = 0,
        .payloadLength = 0,
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = &s_Header,
        .BufferLength = sizeof(s_Payload),
        .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnGetDents(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.Buffer == nullptr || p_Message.BufferLength < sizeof(GetDentsRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const GetDentsRequest*>(p_Message.Buffer);
       if (s_Request->PathLength > sizeof(s_Request->Path))
    {
        WriteLog(LL_Error, "path length out of bounds.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_DirectoryHandle = kopen(s_Request->Path, 0x0000 | 0x00020000, 0777);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", s_Request->Path, s_DirectoryHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_DirectoryHandle);
        return;
    }

    uint64_t s_DentCount = 0;

    // Switch this to use stack
    struct dirent s_Dent = { 0 };

    int32_t s_ReadCount = 0;
    for (;;)
    {
        s_ReadCount = kgetdents(s_DirectoryHandle, reinterpret_cast<char*>(&s_Dent), sizeof(s_Dent));
        if (s_ReadCount <= 0)
            break;
        
        s_DentCount++;
    }
    kclose(s_DirectoryHandle);

    s_Dent = { 0 };

    if (s_DentCount > 0)
    {
        for (;;)
        {
            s_ReadCount = kgetdents(s_DirectoryHandle, reinterpret_cast<char*>(&s_Dent), sizeof(s_Dent));
            if (s_ReadCount <= 0)
                break;
            
            GetDentsResponse s_Payload = 
            {
                .RemainingDents = s_DentCount,
                .Info = {
                    .fileno = s_Dent.d_fileno,
                    .reclen = s_Dent.d_reclen,
                    .type = s_Dent.d_type,
                    .namlen = s_Dent.d_namlen
                }
            };
            
            if (s_Dent.d_namlen < sizeof(s_Payload.Info.name))
                (void)memcpy(s_Payload.Info.name, s_Dent.d_name, s_Dent.d_namlen);

            s_DentCount--;
        }
    }
}

void FileManager::OnStat(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Header == nullptr || p_Message.Buffer == nullptr || p_Message.BufferLength < sizeof(StatRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const StatRequest*>(p_Message.Buffer);

    // Check to see if we have a valid file handle
    struct stat s_Stat = { 0 };

    if (s_Request->Handle < 0)
    {
        if (s_Request->PathLength == 0)
        {
            WriteLog(LL_Error, "invalid path length");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOENT);
            return;
        }

        if (s_Request->PathLength > sizeof(s_Request->Path))
        {
            WriteLog(LL_Error, "path length too long for remaining data.");
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOENT);
            return;
        }

        int32_t s_Ret = kstat(const_cast<char*>(s_Request->Path), &s_Stat);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->Path, s_Ret);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_Ret);
            return;
        }
    }
    else
    {
        auto s_Ret = kfstat(s_Request->Handle, &s_Stat);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->Path, s_Ret);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_Ret);
            return;
        }
    }
    
    // Send a success response back
	StatResponse s_Payload = 
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

    Messaging::MessageHeader s_Header = {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = 0,
        .payloadLength = 0,
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = &s_Header,
        .BufferLength = sizeof(s_Payload),
        .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);}