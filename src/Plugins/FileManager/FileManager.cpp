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
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Unlink, OnUnlink);
    
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
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Unlink, OnUnlink);
    return true;
}

void FileManager::OnEcho(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(EchoRequest))
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
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(OpenRequest))
    {
        WriteLog(LL_Error, "invalid open message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const OpenRequest*>(p_Message.Buffer);

	//WriteLog(LL_Debug, "open: (%s) (%d) (%d)", s_Request->path, s_Request->flags, s_Request->mode);

    int32_t s_Ret = kopen_t(s_Request->Path, s_Request->Flags, s_Request->Mode, s_MainThread);

    Messaging::Message s_Response = {
        .Header = {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = static_cast<uint64_t>(s_Ret),
            .payloadLength = 0,
            .padding = 0
        },
        .Buffer = nullptr
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}


void FileManager::OnClose(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(CloseRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const CloseRequest*>(p_Message.Buffer);
    kclose_t(s_Request->Handle, s_MainThread);

    Messaging::Message s_Response = {
        .Header = 
        {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = 0,
            .payloadLength = 0,
            .padding = 0
        },
        .Buffer = nullptr
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnRead(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(ReadRequest))
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

    //WriteLog(LL_Debug, "here");

    ReadResponse s_Payload = {
        .Count = 0,
        .Buffer = { 0 }
    };

    //WriteLog(LL_Debug, "read (%d) cnt: (%d)", s_Request->Handle, s_Request->Count);

    auto s_SizeRead = kread_t(s_Request->Handle, s_Payload.Buffer, s_Request->Count, s_MainThread);
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
        .payloadLength = sizeof(s_Payload),
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = s_Header,
        .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnGetDents(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Header.payloadLength != sizeof(GetDentsRequest))
    {
        WriteLog(LL_Error, "payloadLength (%d) != sizeof(GetDentsRequest) (%d)", p_Message.Header.payloadLength, sizeof(GetDentsRequest));
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    // BUG: This keeps getting called for a read request, don't know fucking why.
    if (p_Message.Header.errorType != FileManager_GetDents)
    {
        WriteLog(LL_Error, "this is not a dents command (%d) != (%d)", p_Message.Header.errorType, FileManager_GetDents);
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

    auto s_DirectoryHandle = kopen_t(s_Request->Path, 0x0000 | 0x00020000, 0777, s_MainThread);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", s_Request->Path, s_DirectoryHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_DirectoryHandle);
        return;
    }

    uint64_t s_DentCount = 0;

    // Switch this to use stack
    char s_Buffer[0x1000] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));
    WriteLog(LL_Debug, "here");

    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), s_MainThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);

            GetDentsResponse s_Payload = 
            {
                .DentIndex = s_DentCount,
                .Info = {
                    .fileno = l_Dent->d_fileno,
                    .reclen = l_Dent->d_reclen,
                    .type = l_Dent->d_type,
                    .namlen = l_Dent->d_namlen
                }
            };
            
            if (l_Dent->d_namlen <= sizeof(s_Payload.Info.name))
                (void)memcpy(s_Payload.Info.name, l_Dent->d_name, l_Dent->d_namlen);

            s_DentCount++;

            Messaging::Message s_Response = {
                .Header = {
                    .magic = Messaging::MessageHeaderMagic,
                    .category = Messaging::MessageCategory_File,
                    .isRequest = false,
                    .errorType = 0,
                    .payloadLength = sizeof(s_Payload),
                    .padding = 0
                },
                .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
            };

            Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
            l_Pos += l_Dent->d_reclen;
        }
    }
    kclose_t(s_DirectoryHandle, s_MainThread);

    GetDentsResponse s_Payload = 
    {
        .DentIndex = __UINT64_MAX__,
        .Info = {
            .fileno = 0,
            .reclen = 0,
            .type = 0,
            .namlen = 0,
            .name = { 0 }
        }
    };

    Messaging::Message s_Response = {
        .Header = 
        {
                .magic = Messaging::MessageHeaderMagic,
                .category = Messaging::MessageCategory_File,
                .isRequest = false,
                .errorType = 0,
                .payloadLength = sizeof(s_Payload),
                .padding = 0
        },
        .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnStat(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(StatRequest))
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

        int32_t s_Ret = kstat_t(const_cast<char*>(s_Request->Path), &s_Stat, s_MainThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->Path, s_Ret);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_Ret);
            return;
        }
    }
    else
    {
        auto s_Ret = kfstat_t(s_Request->Handle, &s_Stat, s_MainThread);
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

    Messaging::Message s_Response = {
        .Header = 
        {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = 0,
            .payloadLength = sizeof(s_Payload),
            .padding = 0
        },
        .Buffer = reinterpret_cast<const uint8_t*>(&s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}

void FileManager::OnUnlink(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(UnlinkRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    auto s_Request = reinterpret_cast<const UnlinkRequest*>(p_Message.Buffer);
    
    auto s_Ret = kunlink_t(const_cast<char*>(s_Request->Path), s_MainThread);

    WriteLog(LL_Info, "unlink: (%d) (%s).", s_Ret, s_Request->Path);

    Messaging::Message s_Response = {
        .Header = 
        {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = static_cast<uint64_t>(s_Ret),
            .payloadLength = 0,
            .padding = 0
        },
        .Buffer = nullptr
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
}