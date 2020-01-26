#include "FileManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/SelfHeader.hpp>
#include <Utils/Kdlsym.hpp>

#include <Messaging/Message.hpp>
#include <Messaging/MessageManager.hpp>

#include <Messaging/Rpc/Server.hpp>
#include <Messaging/Rpc/Connection.hpp>

#include <Mira.hpp>

#include "FileManagerMessages.hpp"
#include "FileManagerSelf.hpp"

#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/elf64.h>
#include <sys/mman.h>
#include <sys/filedesc.h>
#include <sys/file.h>

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
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_DecryptSelf, OnDecryptSelf);
    
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
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_DecryptSelf, OnDecryptSelf);
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

    ReadResponse* s_Payload = new ReadResponse
    {
        .Count = 0,
        .Buffer = { 0 }
    };

    if (s_Payload == nullptr)
    {
        WriteLog(LL_Error, "could not allocate response");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    //WriteLog(LL_Debug, "read (%d) cnt: (%d)", s_Request->Handle, s_Request->Count);

    auto s_SizeRead = kread_t(s_Request->Handle, s_Payload->Buffer, s_Request->Count, s_MainThread);
    if (s_SizeRead <= 0)
    {
        delete s_Payload;

        WriteLog(LL_Error, "there was an error reading (%d).", s_SizeRead);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_SizeRead);
        return;
    }

    s_Payload->Count = static_cast<uint16_t>(s_SizeRead);

    Messaging::MessageHeader s_Header = {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = 0,
        .payloadLength = sizeof(*s_Payload),
        .padding = 0
    };

    Messaging::Message s_Response = {
        .Header = s_Header,
        .Buffer = reinterpret_cast<const uint8_t*>(s_Payload)
    };

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);

    delete s_Payload;
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

bool IsPhOverlapping(Elf64_Phdr* p_ProgramHeader, int p_ProgramHeaderIndex, Elf64_Phdr* p_ProgramHeaders, int p_ProgramHeaderCount)
{
    for (auto l_Index = 0; l_Index < p_ProgramHeaderCount; ++l_Index)
    {
        if (l_Index == p_ProgramHeaderIndex)
            continue;

        Elf64_Phdr* l_ProgramHeader = &p_ProgramHeaders[l_Index];
        if (l_ProgramHeader->p_filesz > 0)
        {
            if ( (p_ProgramHeader->p_offset >= l_ProgramHeader->p_offset) && ( (p_ProgramHeader->p_offset + p_ProgramHeader->p_filesz) <= (l_ProgramHeader->p_offset + l_ProgramHeader->p_filesz)))
                return true;
        }
    }

    return false;
}

void FileManager::OnDecryptSelf(Messaging::Rpc::Connection* p_Connection, const Messaging::Message& p_Message)
{
    // Get main thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }
    WriteLog(LL_Debug, "mainThread: %p", s_MainThread);

    // Root and escape our thread
	if (s_MainThread->td_ucred)
	{
		WriteLog(LL_Info, "escaping thread");

		s_MainThread->td_ucred->cr_rgid = 0;
		s_MainThread->td_ucred->cr_svgid = 0;
		
		s_MainThread->td_ucred->cr_uid = 0;
		s_MainThread->td_ucred->cr_ruid = 0;

        s_MainThread->td_ucred->cr_groups[0] = 0;

		if (s_MainThread->td_ucred->cr_prison)
			s_MainThread->td_ucred->cr_prison = *(struct prison**)kdlsym(prison0);

		if (s_MainThread->td_proc->p_fd)
			s_MainThread->td_proc->p_fd->fd_rdir = s_MainThread->td_proc->p_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
		
		// Set our auth id as debugger
		s_MainThread->td_ucred->cr_sceAuthID = SceAuthenticationId::SceSysCore;

		// make system credentials
		s_MainThread->td_ucred->cr_sceCaps[0] = SceCapabilites::Max;
		s_MainThread->td_ucred->cr_sceCaps[1] = SceCapabilites::Max;

		WriteLog(LL_Debug, "credentials rooted for main proc");
	}

    if (curthread->td_ucred)
	{
		WriteLog(LL_Info, "escaping thread");

		curthread->td_ucred->cr_rgid = 0;
		curthread->td_ucred->cr_svgid = 0;
		
		curthread->td_ucred->cr_uid = 0;
		curthread->td_ucred->cr_ruid = 0;

        curthread->td_ucred->cr_groups[0] = 0;

		if (curthread->td_ucred->cr_prison)
			curthread->td_ucred->cr_prison = *(struct prison**)kdlsym(prison0);

		if (curthread->td_proc->p_fd)
			curthread->td_proc->p_fd->fd_rdir = s_MainThread->td_proc->p_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
		
		// Set our auth id as debugger
		curthread->td_ucred->cr_sceAuthID = SceAuthenticationId::SceSysCore;

		// make system credentials
		curthread->td_ucred->cr_sceCaps[0] = SceCapabilites::Max;
		curthread->td_ucred->cr_sceCaps[1] = SceCapabilites::Max;

		WriteLog(LL_Debug, "credentials rooted for current thread");
	}

    // Validate our buffers
    if (p_Message.Buffer == nullptr || p_Message.Header.payloadLength < sizeof(DecryptSelfRequest))
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    // Do some sanity checking
    auto s_Request = reinterpret_cast<const DecryptSelfRequest*>(p_Message.Buffer);
    if (s_Request->PathLength >= ARRAYSIZE(s_Request->Path))
    {
        WriteLog(LL_Error, "invalid path length");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -E2BIG);
        return;
    }
    WriteLog(LL_Debug, "request: %p, pathLength: %d", s_Request, s_Request->PathLength);

    // Open the SELF file for reading
    auto s_SelfHandle = kopen_t(s_Request->Path, O_RDONLY, 0777, s_MainThread);
    WriteLog(LL_Debug, "selfHandle: %d", s_SelfHandle);
    if (s_SelfHandle < 0)
    {
        WriteLog(LL_Error, "could not open self (%s) err: (%d).", s_Request->Path, s_SelfHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, s_SelfHandle);
        return;
    }

    size_t s_ElfDataSize = 0;

    auto s_ElfData = DecryptSelfFd(s_SelfHandle, &s_ElfDataSize);
    if (s_ElfData == nullptr || s_ElfDataSize == 0)
    {
        WriteLog(LL_Error, "could not decrypt self");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOEXEC);
        return;
    }

    WriteLog(LL_Debug, "here");
    Messaging::Message s_Message;
    memset(&s_Message, 0, sizeof(s_Message));

    WriteLog(LL_Debug, "here");
    DecryptSelfResponse s_Payload;
    memset(&s_Payload, 0, sizeof(s_Payload));

    WriteLog(LL_Debug, "here");
    auto s_ChunkCount = s_ElfDataSize / MaxBufferLength;
    auto s_LeftoverCount = s_ElfDataSize % MaxBufferLength;

    WriteLog(LL_Debug, "ChunkCount: %lld Leftover: %lld", s_ChunkCount, s_LeftoverCount);
    auto s_TotalIndex = 0;
    size_t s_TotalOffset = 0;
    for (auto l_ChunkIndex = 0; l_ChunkIndex < s_ChunkCount; ++l_ChunkIndex)
    {
        s_Payload.Index = s_TotalIndex;
        s_Payload.Offset = s_TotalOffset;
        s_Payload.Length = MaxBufferLength;
        memcpy(s_Payload.Data, &s_ElfData[s_TotalOffset], s_Payload.Length);

        s_Message.Header = 
        {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = static_cast<uint64_t>(0),
            .payloadLength = sizeof(s_Payload),
            .padding = 0
        };
        s_Message.Buffer = (const uint8_t*)&s_Payload;

        Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Message);

        s_TotalIndex++;
        s_TotalOffset += MaxBufferLength;
    }

    if (s_LeftoverCount > 0)
    {
        WriteLog(LL_Debug, "here");
        s_Payload.Index = s_TotalIndex;
        s_Payload.Offset = s_TotalOffset;
        s_Payload.Length = s_LeftoverCount;
        WriteLog(LL_Debug, "here");
        memcpy(s_Payload.Data, &s_ElfData[s_TotalOffset], s_Payload.Length);

        WriteLog(LL_Debug, "here");
        s_Message.Header = 
        {
            .magic = Messaging::MessageHeaderMagic,
            .category = Messaging::MessageCategory_File,
            .isRequest = false,
            .errorType = static_cast<uint64_t>(0),
            .payloadLength = sizeof(s_Payload),
            .padding = 0
        };
        s_Message.Buffer = (const uint8_t*)&s_Payload;

        WriteLog(LL_Debug, "here");

        Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Message);

        s_TotalIndex++;
        s_TotalOffset += MaxBufferLength;
        WriteLog(LL_Debug, "wrote leftover, total bytes: %lld", s_TotalOffset);
    }

    // Send "complete" message
    s_Payload.Index = __UINT64_MAX__;
    s_Payload.Offset = 0;
    s_Payload.Length = 0;
    WriteLog(LL_Debug, "here");
    s_Message.Header = 
    {
        .magic = Messaging::MessageHeaderMagic,
        .category = Messaging::MessageCategory_File,
        .isRequest = false,
        .errorType = static_cast<uint64_t>(0),
        .payloadLength = sizeof(s_Payload),
        .padding = 0
    };
    s_Message.Buffer = (const uint8_t*)&s_Payload;

    delete[] s_ElfData;
    WriteLog(LL_Debug, "here");

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Message);
    WriteLog(LL_Error, "self decryption complete");
}

uint8_t* FileManager::DecryptSelfFd(int p_SelfFd, size_t* p_OutElfSize)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return nullptr;
    }

    size_t s_Offset = klseek_t(p_SelfFd, 0, SEEK_END, s_MainThread);
    if (s_Offset <= 0)
    {
        WriteLog(LL_Error, "invalid offset (%lld).", s_Offset);
        return nullptr;
    }

    size_t s_SelfSize = s_Offset;

    s_Offset = klseek_t(p_SelfFd, 0, SEEK_SET, s_MainThread);
    if (s_Offset < 0)
    {
        WriteLog(LL_Error, "could not seek (%lld).", s_Offset);
        return nullptr;
    }

    auto s_SelfData = new uint8_t[s_SelfSize];
    if (s_SelfData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate self data (%llx).", s_SelfSize);
        return nullptr;
    }
    memset(s_SelfData, 0, s_SelfSize);

    s_Offset = kread_t(p_SelfFd, s_SelfData, s_SelfSize, s_MainThread);
    if (s_Offset != s_SelfSize)
    {
        // Free our allocated buffer
        delete[] s_SelfData;

        WriteLog(LL_Error, "could not read all of the self data (%lld).", s_Offset);
        return nullptr;
    }

    return DecryptSelf(s_SelfData, s_SelfSize, p_SelfFd, p_OutElfSize);
}

uint8_t* FileManager::DecryptSelf(uint8_t* p_SelfData, size_t p_SelfSize, int p_SelfFd, size_t* p_OutElfSize)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return nullptr;
    }

    if (p_SelfData == nullptr ||
        p_OutElfSize == nullptr)
    {
        WriteLog(LL_Error, "invalid self data or out elf size");
        return nullptr;
    }

    size_t s_EhdrOffset = 0;
    auto s_Ehdr = reinterpret_cast<Elf64_Ehdr*>(p_SelfData);
    
    if (IsValidElf(s_Ehdr))
    {
        auto s_ElfData = new uint8_t[p_SelfSize];
        if (s_ElfData == nullptr)
        {
            WriteLog(LL_Error, "could not allocate elf data");
            return nullptr;
        }

        memcpy(s_ElfData, p_SelfData, p_SelfSize);

        *p_OutElfSize = p_SelfSize;

        return s_ElfData;
    }
    else
    {
        // Handle SELF decryption
        auto s_SelfHeader = reinterpret_cast<self_header_t*>(p_SelfData);
        s_EhdrOffset += sizeof(*s_SelfHeader);
        s_EhdrOffset += sizeof(self_entry_t) * s_SelfHeader->num_entries;
        s_Ehdr = reinterpret_cast<Elf64_Ehdr*>(&p_SelfData[s_EhdrOffset]);
    }

    uint8_t* s_ElfBase = reinterpret_cast<uint8_t*>(s_Ehdr);
    if (!IsValidElf(s_Ehdr))
    {
        WriteLog(LL_Error, "could not find ELF header in self");
        return nullptr;
    }

    s_Ehdr->e_shoff = 0;
    s_Ehdr->e_shnum = 0;
    size_t s_ElfSize = s_Ehdr->e_phoff + (s_Ehdr->e_phnum * s_Ehdr->e_phentsize) + (s_Ehdr->e_shnum * s_Ehdr->e_shentsize);

    // Parse PHDRs
    auto s_Phdr = reinterpret_cast<Elf64_Phdr*>(&s_ElfBase[s_Ehdr->e_phoff]);
    for (auto i = 0; i < s_Ehdr->e_phnum; ++i)
    {
        switch (s_Phdr[i].p_type)
        {
        case PT_LOAD:
        case PT_SCE_DYNLIBDATA:
        case PT_DYNAMIC:
            s_ElfSize = MAX(s_ElfSize, s_Phdr[i].p_offset + s_Phdr[i].p_filesz);
            break;
        }
    }

    *p_OutElfSize = s_ElfSize;
    auto s_ElfData = new uint8_t[s_ElfSize];
    if (s_ElfData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate raw elf data.");
        return false;
    }
    memset(s_ElfData, 0, s_ElfSize);

    // Copy over the elf header
    for (auto i = 0; i < s_Ehdr->e_ehsize; ++i)
        s_ElfData[i] = s_ElfBase[i];
    
    // Copy over all program headers
    for (auto l_PhIndex = 0; l_PhIndex < s_Ehdr->e_phnum; ++l_PhIndex)
    {
        for (auto l_PhEntIndex = 0; l_PhEntIndex < s_Ehdr->e_phentsize; ++l_PhEntIndex)
        {
            auto l_Offset = s_Ehdr->e_phoff + (l_PhIndex * s_Ehdr->e_phentsize) + l_PhEntIndex;
            s_ElfData[l_Offset] = s_ElfBase[l_Offset];
        }

        if (s_Phdr[l_PhIndex].p_type == PT_LOAD ||
            s_Phdr[l_PhIndex].p_type == PT_SCE_DYNLIBDATA)
        {
            klseek_t(p_SelfFd, 0, SEEK_SET, s_MainThread);
            auto l_ElfSegment = kmmap_t(nullptr, s_Phdr[l_PhIndex].p_filesz, PROT_READ, MAP_SHARED | MAP_SELF | MAP_PREFAULT_READ, p_SelfFd, (((uint64_t)l_PhIndex) << 32), s_MainThread);
            
            WriteLog(LL_Warn, "%p = mmap(%p, 0x%llx, %d, %d, %d, %llx, %p)", l_ElfSegment, nullptr, s_Phdr[l_PhIndex].p_filesz, PROT_READ, MAP_PRIVATE | MAP_SELF, p_SelfFd, (((uint64_t)l_PhIndex) << 32), s_MainThread);
            if (l_ElfSegment != MAP_FAILED || l_ElfSegment == nullptr)
            {
                // For some strange reason mmap will return ENOMEM for no fucking reason
                if (l_ElfSegment == (caddr_t)(-ENOMEM))
                {
                    WriteLog(LL_Error, "mmap returned (%lld).", l_ElfSegment);
                    continue;
                }

                for (auto l_FileIndex = 0; l_FileIndex < s_Phdr[l_PhIndex].p_filesz; ++l_FileIndex)
                    s_ElfData[s_Phdr[l_PhIndex].p_offset + l_FileIndex] = l_ElfSegment[l_FileIndex];
            }
            kmunmap_t(l_ElfSegment, s_Phdr[l_PhIndex].p_filesz, s_MainThread);
        }
    }

    return s_ElfData;
}