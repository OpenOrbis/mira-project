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

		if (s_MainThread->td_ucred->cr_prison)
			s_MainThread->td_ucred->cr_prison = *(struct prison**)kdlsym(prison0);

		if (s_MainThread->td_proc->p_fd)
			s_MainThread->td_proc->p_fd->fd_rdir = s_MainThread->td_proc->p_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
		
		// Set our auth id as debugger
		s_MainThread->td_ucred->cr_sceAuthID = SceAuthenticationId::SceSysCore;

		// make system credentials
		s_MainThread->td_ucred->cr_sceCaps[0] = SceCapabilites::Max;
		s_MainThread->td_ucred->cr_sceCaps[1] = SceCapabilites::Max;

		WriteLog(LL_Debug, "credentials rooted for new proc");
	}

    if (curthread->td_ucred)
	{
		WriteLog(LL_Info, "escaping thread");

		curthread->td_ucred->cr_rgid = 0;
		curthread->td_ucred->cr_svgid = 0;
		
		curthread->td_ucred->cr_uid = 0;
		curthread->td_ucred->cr_ruid = 0;

		if (curthread->td_ucred->cr_prison)
			curthread->td_ucred->cr_prison = *(struct prison**)kdlsym(prison0);

		if (curthread->td_proc->p_fd)
			curthread->td_proc->p_fd->fd_rdir = curthread->td_proc->p_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
		
		// Set our auth id as debugger
		curthread->td_ucred->cr_sceAuthID = SceAuthenticationId::SceSysCore;

		// make system credentials
		curthread->td_ucred->cr_sceCaps[0] = SceCapabilites::Max;
		curthread->td_ucred->cr_sceCaps[1] = SceCapabilites::Max;

		WriteLog(LL_Debug, "credentials rooted for new proc");
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

    // Get the file size
    auto s_FileSize = klseek_t(s_SelfHandle, 0, SEEK_END, s_MainThread);
    WriteLog(LL_Debug, "fileSize: 0x%llx", s_FileSize);
    if (s_FileSize == (off_t)-1)
    {
        // Close the previously opened handle
        kclose_t(s_SelfHandle, s_MainThread);
        WriteLog(LL_Error, "could not seek to the end of file");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -EIO);
        return;
    }

    // Return back to the start
    auto s_SeekRet = klseek_t(s_SelfHandle, 0, SEEK_SET, s_MainThread);
    WriteLog(LL_Debug, "seekRet: %llx", s_SeekRet);
    if (s_SeekRet == (off_t)-1)
    {
        // Close the previously opened handle
        kclose_t(s_SelfHandle, s_MainThread);
        WriteLog(LL_Error, "could not seek to the start of file");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -EIO);
        return;
    }

    // Allocate to hold our entire self in memory (this may be dangerous/cause kpanics)
    auto s_SelfData = new uint8_t[s_FileSize];
    WriteLog(LL_Debug, "selfData: %p", s_SelfData);
    if (s_SelfData == nullptr)
    {
        // Close the previously opened handle
        kclose_t(s_SelfHandle, s_MainThread);
        WriteLog(LL_Error, "could not allocate (%llx) bytes for self", s_FileSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }
    memset(s_SelfData, 0, s_FileSize);

    // Read out the entire self
    auto s_Read = kread_t(s_SelfHandle, s_SelfData, s_FileSize, s_MainThread);
    WriteLog(LL_Debug, "read: %lld", s_Read);
    if (s_Read < 0)
    {
        // Close the previously opened handle
        kclose_t(s_SelfHandle, s_MainThread);

        // Free the allocated buffer
        delete [] s_SelfData;

        WriteLog(LL_Error, "could not read (%llx) bytes for self", s_FileSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOMEM);
        return;
    }

    // Check to make sure that we haven't already gotten an raw ELF
    if (s_SelfData[0] == ELFMAG0 && s_SelfData[1] == ELFMAG1 && s_SelfData[2] == ELFMAG2 && s_SelfData[3] == ELFMAG3)
    {
        kclose_t(s_SelfHandle, s_MainThread);

        // Free the allocated buffer
        delete [] s_SelfData;

        WriteLog(LL_Error, "tried to decrypt ELF header");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOEXEC);
        return;
    }
    WriteLog(LL_Debug, "did not get elf");

    // Read out the entity count from the self header
    auto s_SelfHeader = reinterpret_cast<Utils::SelfHeader*>(s_SelfData);
    WriteLog(LL_Info, "selfHeader: %p", s_SelfHeader);
    auto s_ElfHeaderOffset = (s_SelfHeader->EntityCount * 0x20) + 0x20;
    WriteLog(LL_Info, "elfHeaderOffset: 0x%x", s_ElfHeaderOffset);
    auto s_ElfInMemory = s_SelfData + s_ElfHeaderOffset;
    WriteLog(LL_Info, "elfInMemory: %p", s_ElfInMemory);

    auto s_ElfHeader = reinterpret_cast<Elf64_Ehdr*>(s_ElfInMemory);

    // Validate the ELF header
    if (s_ElfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
        s_ElfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
        s_ElfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
        s_ElfHeader->e_ident[EI_MAG3] != ELFMAG3)
    {
        kclose_t(s_SelfHandle, s_MainThread);
        
        // Free the allocated buffer
        delete [] s_SelfData;

        WriteLog(LL_Error, "invalid elf magic");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, Messaging::MessageCategory_File, -ENOEXEC);
        return;
    }
    WriteLog(LL_Debug, "ELF Header validated!");

    // We don't want any of the section headers as they aren't decrypted/pointing to invalid bullshit
    s_ElfHeader->e_shnum = 0;
    s_ElfHeader->e_shoff = 0;

    WriteLog(LL_Debug, "disabled section headers");
    
    uint64_t s_TotalSentChunks = 0;

    // Iterate all of the program headers
    auto s_ProgramHeaderStart = reinterpret_cast<Elf64_Phdr*>(s_ElfInMemory + s_ElfHeader->e_phoff);
    for (auto l_Index = 0; l_Index < s_ElfHeader->e_phnum; ++l_Index)
    {
        auto l_ProgramHeader = &s_ProgramHeaderStart[l_Index];
        WriteLog(LL_Debug, "programHeader: %p", l_ProgramHeader);

        // Validate file size
        if (l_ProgramHeader->p_filesz <= 0)
            continue;

        // Skip SCE_SYM?? section
        if (l_ProgramHeader->p_type == 0x6fffff01)
            continue;

        // Check if this program header is overlapping with others
        if (IsPhOverlapping(l_ProgramHeader, l_Index, s_ProgramHeaderStart, s_ElfHeader->e_phnum))
            continue;

        // Check if this section is over 32MiB, we have to do this in chunks otherwise
        const uint64_t c_MaxChunkSize = 0x2000000;
        auto l_Chunks = l_ProgramHeader->p_filesz / c_MaxChunkSize;
        auto l_Leftover = l_ProgramHeader->p_filesz % c_MaxChunkSize;

        WriteLog(LL_Debug, "chunks: %llx leftover: %llx", l_Chunks, l_Leftover);

        // Iterate for each program header in 32MiB chunks
        uint64_t l_ChunkOffset = 0;
        for (auto l_ChunkIndex = 0; l_ChunkIndex < l_Chunks; ++l_ChunkIndex)
        {
            // Calculate the real offset by ph index << 32 | offset in file from chunk
            uint64_t l_RealOffset = (static_cast<uint64_t>(l_Index) << 32) | l_ChunkOffset;
            l_ChunkOffset += c_MaxChunkSize;

            // Decrypt the next 32MiB chunk
            auto l_DecryptedData = kmmap_t(nullptr, c_MaxChunkSize, PROT_READ, MAP_SELF | MAP_SHARED, s_SelfHandle, l_RealOffset, s_MainThread);
            WriteLog(LL_Debug, "realOffset: %llx, decryptedData: %p", l_RealOffset, l_DecryptedData);
            if (l_DecryptedData == MAP_FAILED)
            {
                WriteLog(LL_Error, "could not map_self, returned MAP_FAILED ph (%d).", l_Index);
                continue;
            }

            // Split up for the transport
            auto l_TransportChunks = c_MaxChunkSize / MaxBufferLength;
            auto l_TransportLeftover = c_MaxChunkSize % MaxBufferLength;
            WriteLog(LL_Info, "here");
            auto l_TransportOffset = 0;
            for (auto l_TransportIndex = 0; l_TransportIndex < l_TransportChunks; l_TransportIndex++)
            {
                DecryptSelfResponse l_Payload = {
                    .Index = s_TotalSentChunks,
                    .Offset = l_ChunkOffset + l_TransportOffset,
                    .Length = MaxBufferLength,
                    .Data = { 0 }
                };

                // Copy the data to our payload
                memcpy(l_Payload.Data, l_DecryptedData + l_TransportOffset, MaxBufferLength);

                l_TransportOffset += MaxBufferLength;
                s_TotalSentChunks++;

                // Send the response back
                Messaging::Message s_Response = 
                {
                    .Header = 
                    {
                            .magic = Messaging::MessageHeaderMagic,
                            .category = Messaging::MessageCategory_File,
                            .isRequest = false,
                            .errorType = 0,
                            .payloadLength = sizeof(l_Payload),
                            .padding = 0
                    },
                    .Buffer = reinterpret_cast<const uint8_t*>(&l_Payload)
                };

                Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
            }

            WriteLog(LL_Info, "here");

            if (l_TransportLeftover > 0)
            {
                DecryptSelfResponse l_Payload = {
                    .Index = s_TotalSentChunks,
                    .Offset = l_ChunkOffset + l_TransportOffset,
                    .Length = l_TransportLeftover,
                    .Data = { 0 }
                };

                // Copy the data to our payload
                memcpy(l_Payload.Data, l_DecryptedData + l_TransportOffset, l_TransportLeftover);

                l_TransportOffset += l_TransportLeftover;
                s_TotalSentChunks++;

                WriteLog(LL_Debug, "TransportOffset: %llx, TotalSentChunks: %llx", l_TransportOffset, s_TotalSentChunks);
                
                // Send the response back
                Messaging::Message s_Response = 
                {
                    .Header = 
                    {
                            .magic = Messaging::MessageHeaderMagic,
                            .category = Messaging::MessageCategory_File,
                            .isRequest = false,
                            .errorType = 0,
                            .payloadLength = sizeof(l_Payload),
                            .padding = 0
                    },
                    .Buffer = reinterpret_cast<const uint8_t*>(&l_Payload)
                };

                Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
            }

            // Unmap the previously mapped data
            kmunmap_t(l_DecryptedData, c_MaxChunkSize, s_MainThread);
        }

        if (l_Leftover > 0)
        {
            uint64_t l_RealOffset = (static_cast<uint64_t>(l_Index) << 32) | l_ChunkOffset;
            l_ChunkOffset += l_Leftover;

            auto l_DecryptedData = kmmap_t(nullptr, l_Leftover, PROT_READ, MAP_SELF | MAP_SHARED, s_SelfHandle, l_RealOffset, s_MainThread);
            WriteLog(LL_Debug, "decryptedData: %p", l_DecryptedData);
            if (l_DecryptedData == MAP_FAILED)
            {
                WriteLog(LL_Error, "could not map_self, returned MAP_FAILED ph (%d).", l_Index);
                continue;
            }

            // Split up for the transport
            auto l_TransportChunks = l_Leftover / MaxBufferLength;
            auto l_TransportLeftover = l_Leftover % MaxBufferLength;
            
            WriteLog(LL_Info, "here");
            auto l_TransportOffset = 0;
            for (auto l_TransportIndex = 0; l_TransportIndex < l_TransportChunks; l_TransportIndex++)
            {
                DecryptSelfResponse l_Payload = {
                    .Index = s_TotalSentChunks,
                    .Offset = l_ChunkOffset + l_TransportOffset,
                    .Length = MaxBufferLength,
                    .Data = { 0 }
                };

                // Copy the data to our payload
                memcpy(l_Payload.Data, l_DecryptedData + l_TransportOffset, MaxBufferLength);

                l_TransportOffset += MaxBufferLength;
                s_TotalSentChunks++;

                // Send the response back
                Messaging::Message s_Response = 
                {
                    .Header = 
                    {
                            .magic = Messaging::MessageHeaderMagic,
                            .category = Messaging::MessageCategory_File,
                            .isRequest = false,
                            .errorType = 0,
                            .payloadLength = sizeof(l_Payload),
                            .padding = 0
                    },
                    .Buffer = reinterpret_cast<const uint8_t*>(&l_Payload)
                };

                Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
            }
            WriteLog(LL_Info, "here");
            if (l_TransportLeftover > 0)
            {
                DecryptSelfResponse l_Payload = {
                    .Index = s_TotalSentChunks,
                    .Offset = l_ChunkOffset + l_TransportOffset,
                    .Length = l_TransportLeftover,
                    .Data = { 0 }
                };

                // Copy the data to our payload
                memcpy(l_Payload.Data, l_DecryptedData + l_TransportOffset, l_TransportLeftover);

                l_TransportOffset += l_TransportLeftover;
                s_TotalSentChunks++;

                // Send the response back
                Messaging::Message s_Response = 
                {
                    .Header = 
                    {
                            .magic = Messaging::MessageHeaderMagic,
                            .category = Messaging::MessageCategory_File,
                            .isRequest = false,
                            .errorType = 0,
                            .payloadLength = sizeof(l_Payload),
                            .padding = 0
                    },
                    .Buffer = reinterpret_cast<const uint8_t*>(&l_Payload)
                };

                Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
            }

            // Unmap the previously mapped data
            kmunmap_t(l_DecryptedData, l_Leftover, s_MainThread);
        }
    }

    WriteLog(LL_Debug, "closing self handle");
    kclose_t(s_SelfHandle, s_MainThread);
    s_SelfHandle = -1;

    delete [] s_SelfData;
    s_SelfData = nullptr;

    WriteLog(LL_Info, "here");

    DecryptSelfResponse s_Payload = {
        .Index = __UINT64_MAX__,
        .Offset = 0,
        .Length = 0,
        .Data = { 0 }
    };

    Messaging::Message s_Response = 
    {
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

    WriteLog(LL_Info, "here");

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Response);
    WriteLog(LL_Info, "decrypted (%s) successfully!", s_Request->Path);
}