// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FileManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/SelfHeader.hpp>
#include <Utils/Kdlsym.hpp>

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

extern "C"
{
    #include "filemanager.pb-c.h"
    #include <Messaging/Rpc/rpc.pb-c.h>
};

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
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Echo, OnEcho);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Open, OnOpen);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Close, OnClose);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Read, OnRead);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Write, OnWrite);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_GetDents, OnGetDents);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Stat, OnStat);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_MkDir, OnMkDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_RmDir, OnRmDir);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Unlink, OnUnlink);
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_DecryptSelf, OnDecryptSelf);
    
    return true;
}

bool FileManager::OnUnload()
{
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Echo, OnEcho);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Open, OnOpen);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Close, OnClose);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Read, OnRead);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Write, OnWrite);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_GetDents, OnGetDents);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Stat, OnStat);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_MkDir, OnMkDir);
    //Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_RmDir, OnRmDir);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_Unlink, OnUnlink);
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(RPC_CATEGORY__FILE, FileManager_DecryptSelf, OnDecryptSelf);
    return true;
}

void FileManager::OnEcho(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
    FmEchoRequest* s_Request = fm_echo_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }

    WriteLog(LL_Error, "echo: (%s).", s_Request->message);

    fm_echo_request__free_unpacked(s_Request, nullptr);
}

void FileManager::OnOpen(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    if (p_Message->data.data == nullptr || p_Message->data.len <= 0)
    {
        WriteLog(LL_Error, "invalid open message (%p) (%x)", p_Message->data, p_Message->data.len);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    FmOpenRequest* s_Request = fm_open_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }

    int32_t s_Ret = kopen_t(s_Request->path, s_Request->flags, s_Request->mode, s_IoThread);

    fm_open_request__free_unpacked(s_Request, nullptr);

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_Open, s_Ret, nullptr, 0);
}


void FileManager::OnClose(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
    if (p_Message->data.data == nullptr || p_Message->data.len <= 0)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    FmCloseRequest* s_Request = fm_close_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    kclose_t(s_Request->handle, s_IoThread);

    fm_close_request__free_unpacked(s_Request, nullptr);

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_Close, 0, nullptr, 0);
}

void FileManager::OnRead(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    if (p_Message->data.data == nullptr || p_Message->data.len <= 0)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    FmReadRequest* s_Request = fm_read_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_DataSize = s_Request->size;
    uint8_t* s_Data = new uint8_t[s_DataSize];
    if (s_Data == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%x) bytes", s_DataSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        fm_read_request__free_unpacked(s_Request, nullptr);
        return;
    }
    memset(s_Data, 0, s_DataSize);

    auto s_Ret = kread_t(s_Request->handle, s_Data, s_DataSize, s_IoThread);

    fm_read_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    if (s_Ret <= 0)
    {
        WriteLog(LL_Error, "read returned (%d)", s_Ret);
        delete [] s_Data;
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, s_Ret);
        return;
    }

    FmReadResponse s_Response = FM_READ_RESPONSE__INIT;
    s_Response.data.data = s_Data;
    s_Response.data.len = s_DataSize;

    auto s_PackedSize = fm_read_response__get_packed_size(&s_Response);
    if (s_PackedSize <= 0)
    {
        WriteLog(LL_Error, "could not get packed size");
        delete [] s_Data;
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -EIO);
        return;
    }

    auto s_PackedData = new uint8_t[s_PackedSize];
    if (s_PackedData == nullptr)
    {
        WriteLog(LL_Error, "could not allocated packed data (%llx)", s_PackedSize);
        delete [] s_Data;
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }
    memset(s_PackedData, 0, s_PackedSize);

    auto s_PackedRet = fm_read_response__pack(&s_Response, s_PackedData);
    if (s_PackedRet != s_PackedSize)
    {
        WriteLog(LL_Error, "packed ret (%llx) != packed size (%llx)", s_PackedRet, s_PackedSize);
        delete [] s_PackedData;
        delete [] s_Data;
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_Read, 0, s_PackedData, s_PackedSize);

    // Free the allocated data for packing
    delete [] s_PackedData;
    delete [] s_Data;
}

void FileManager::OnGetDents(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    if (p_Message->data.data == nullptr || p_Message->data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    FmGetDentsRequest* s_Request = fm_get_dents_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_DirectoryHandle = kopen_t(s_Request->path, 0x0000 | 0x00020000, 0777, s_IoThread);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", s_Request->path, s_DirectoryHandle);
        fm_get_dents_request__free_unpacked(s_Request, nullptr);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, s_DirectoryHandle);
        return;
    }

    uint64_t s_DentCount = GetDentCount(s_Request->path);
    //WriteLog(LL_Info, "dentCount: (%lld)", s_DentCount);

    fm_get_dents_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    // Protect against zero-size deref
    if (s_DentCount == 0)
    {
        WriteLog(LL_Error, "could not get dents");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOENT);
        return;
    }

    FmDent* s_Dents[s_DentCount];
    memset(s_Dents, 0, sizeof(s_Dents));

    uint64_t s_CurrentDentIndex = 0;

    //WriteLog(LL_Info, "dents: (%p)", s_Dents);

    // Switch this to use stack
    char s_Buffer[0x1000] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));
    //WriteLog(LL_Debug, "here");

    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), s_IoThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            // Bounds check
            if (s_CurrentDentIndex >= s_DentCount)
            {
                WriteLog(LL_Error, "current index (%lld) >= (%lld)", s_CurrentDentIndex, s_DentCount);
                break;
            }

            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);

            auto l_FmDent = new FmDent();
            if (l_FmDent == nullptr)
            {
                WriteLog(LL_Error, "could not allocate fmdent");
                break;
            }
            memset(l_FmDent, 0, sizeof(*l_FmDent));

            // Initialize dent
            *l_FmDent = FM_DENT__INIT;
            l_FmDent->fileno = l_Dent->d_fileno;
            l_FmDent->type = l_Dent->d_type;
            l_FmDent->name = new char[l_Dent->d_namlen + 1];
            if (l_FmDent->name == nullptr)
            {
                WriteLog(LL_Error, "could not allocate memory for name");
                break;
            }
            memset(l_FmDent->name, 0, l_Dent->d_namlen + 1);
            memcpy(l_FmDent->name, l_Dent->d_name, l_Dent->d_namlen);

            //WriteLog(LL_Info, "dent: (%d) (%s)", s_CurrentDentIndex, l_FmDent->name);

            s_Dents[s_CurrentDentIndex] = l_FmDent;

            //WriteLog(LL_Debug, "s_CurrentDentIndex: %lld", s_CurrentDentIndex);
            s_CurrentDentIndex++;

            l_Pos += l_Dent->d_reclen;
        }
    }
    kclose_t(s_DirectoryHandle, s_IoThread);

    //WriteLog(LL_Debug, "here");
    FmGetDentsResponse s_Response = FM_GET_DENTS_RESPONSE__INIT;
    s_Response.n_dents = s_DentCount;
    s_Response.dents = s_Dents;

    //WriteLog(LL_Debug, "here");
    auto s_PackedSize = fm_get_dents_response__get_packed_size(&s_Response);
    //WriteLog(LL_Debug, "packedSize: %lld", s_PackedSize);
    if (s_PackedSize <= 0)
    {
        WriteLog(LL_Error, "could not get packed size");

         // Free all of the allocated names
        for (auto i = 0; i < s_DentCount; ++i)
        {
            if (s_Dents[i] == nullptr)
                continue;
                
            if (s_Dents[i]->name != nullptr)
                delete [] s_Dents[i]->name;
            
            s_Dents[i]->name = nullptr;

            delete s_Dents[i];
            s_Dents[i] = nullptr;
        }
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_Data = new uint8_t[s_PackedSize];
    //WriteLog(LL_Debug, "data: (%p) size: %lld", s_Data, s_PackedSize);
    if (s_Data == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%llx)", s_Data);

         // Free all of the allocated names
        for (auto i = 0; i < s_DentCount; ++i)
        {
            if (s_Dents[i] == nullptr)
                continue;

            if (s_Dents[i]->name != nullptr)
                delete [] s_Dents[i]->name;
            
            s_Dents[i]->name = nullptr;

            delete s_Dents[i];
            s_Dents[i] = nullptr;
        }
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_Ret = fm_get_dents_response__pack(&s_Response, s_Data);
    //WriteLog(LL_Debug, "pack result (%lld)", s_Ret);
    if (s_Ret != s_PackedSize)
    {
        WriteLog(LL_Error, "could not pack");

         // Free all of the allocated names
        for (auto i = 0; i < s_DentCount; ++i)
        {
            if (s_Dents[i] == nullptr)
                continue;
                
            if (s_Dents[i]->name != nullptr)
                delete [] s_Dents[i]->name;
            
            s_Dents[i]->name = nullptr;

            delete s_Dents[i];
            s_Dents[i] = nullptr;
        }
        delete [] s_Data;
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_GetDents, 0, s_Data, s_PackedSize);

    // Free all of the allocated names
    for (auto i = 0; i < s_DentCount; ++i)
    {
        if (s_Dents[i] == nullptr)
            continue;
            
        if (s_Dents[i]->name != nullptr)
            delete [] s_Dents[i]->name;
        
        s_Dents[i]->name = nullptr;

        delete s_Dents[i];
        s_Dents[i] = nullptr;
    }

    delete [] s_Data;
}

uint64_t FileManager::GetDentCount(const char* p_Path)
{	
    auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return 0;
	}

    auto s_DirectoryHandle = kopen_t(p_Path, 0x0000 | 0x00020000, 0777, s_IoThread);
    if (s_DirectoryHandle < 0)
    {
		WriteLog(LL_Error, "could not open directory (%s) (%d).", p_Path, s_DirectoryHandle);
        return 0;
    }

    uint64_t s_DentCount = 0;

    // Switch this to use stack
    char s_Buffer[0x1000] = { 0 };
    memset(s_Buffer, 0, sizeof(s_Buffer));

    int32_t s_ReadCount = 0;
    for (;;)
    {
        memset(s_Buffer, 0, sizeof(s_Buffer));
        s_ReadCount = kgetdents_t(s_DirectoryHandle, s_Buffer, sizeof(s_Buffer), s_IoThread);
        if (s_ReadCount <= 0)
            break;
        
        for (auto l_Pos = 0; l_Pos < s_ReadCount;)
        {
            auto l_Dent = (struct dirent*)(s_Buffer + l_Pos);
            s_DentCount++;
            l_Pos += l_Dent->d_reclen;
        }
    }
    kclose_t(s_DirectoryHandle, s_IoThread);

    return s_DentCount;
}

void FileManager::OnStat(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    if (p_Message->data.data == nullptr || p_Message->data.len <= 0)
    {
        WriteLog(LL_Error, "could not get main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    FmStatRequest* s_Request = fm_stat_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    // Check to see if we have a valid file handle
    struct stat s_Stat = { 0 };

    if (s_Request->handle < 0)
    {
        if (s_Request->path == nullptr)
        {
            WriteLog(LL_Error, "invalid path length");
            fm_stat_request__free_unpacked(s_Request, nullptr);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOENT);
            return;
        }

        int32_t s_Ret = kstat_t(const_cast<char*>(s_Request->path), &s_Stat, s_IoThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->path, s_Ret);
            fm_stat_request__free_unpacked(s_Request, nullptr);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, s_Ret);
            return;
        }
    }
    else
    {
        auto s_Ret = kfstat_t(s_Request->handle, &s_Stat, s_IoThread);
        if (s_Ret < 0)
        {
            WriteLog(LL_Error, "could not stat (%s), returned (%d).", s_Request->path, s_Ret);
            fm_stat_request__free_unpacked(s_Request, nullptr);
            Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, s_Ret);
            return;
        }
    }

    // Free the unpacked
    fm_stat_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;
    
    // Send a success response back
    FmStatResponse s_Response = FM_STAT_RESPONSE__INIT;
    s_Response.st_dev = s_Stat.st_dev;
    s_Response.st_ino = s_Stat.st_ino;
    s_Response.st_mode = s_Stat.st_mode;
    s_Response.st_nlink = s_Stat.st_nlink;
    s_Response.st_uid = s_Stat.st_uid;
    s_Response.st_gid = s_Stat.st_gid;
    s_Response.st_rdev = s_Stat.st_rdev;

    FmTimespec s_Atim = FM_TIMESPEC__INIT;
    s_Atim.tv_sec = s_Stat.st_atim.tv_sec;
    s_Atim.tv_nsec = s_Stat.st_atim.tv_nsec;
    s_Response.st_atim = &s_Atim;

    FmTimespec s_Mtim = FM_TIMESPEC__INIT;
    s_Mtim.tv_sec = s_Stat.st_mtim.tv_sec;
    s_Mtim.tv_nsec = s_Stat.st_mtim.tv_nsec;

    s_Response.st_mtim = &s_Mtim;

    FmTimespec s_Ctim = FM_TIMESPEC__INIT;
    s_Ctim.tv_sec = s_Stat.st_ctim.tv_sec;
    s_Ctim.tv_nsec = s_Stat.st_ctim.tv_nsec;
    s_Response.st_ctim = &s_Ctim;

    s_Response.st_size = s_Stat.st_size;
    s_Response.st_blocks = s_Stat.st_blocks;
    s_Response.st_blksize = s_Stat.st_blksize;
    s_Response.st_flags = s_Stat.st_flags;
    s_Response.st_gen = s_Stat.st_gen;
    s_Response.st_lspare = s_Stat.st_lspare;

    FmTimespec s_Birthtim = FM_TIMESPEC__INIT;
    s_Birthtim.tv_sec = s_Stat.st_birthtim.tv_sec;
    s_Birthtim.tv_nsec = s_Stat.st_birthtim.tv_nsec;
    s_Response.st_birthtim = &s_Birthtim;

    auto s_ResponseSize = fm_stat_response__get_packed_size(&s_Response);
    if (s_ResponseSize <= 0)
    {
        WriteLog(LL_Error, "could not get packed size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_ResponseData = new uint8_t[s_ResponseSize];
    if (s_ResponseData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate (%llx)", s_ResponseSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }
    memset(s_ResponseData, 0, s_ResponseSize);

    auto s_Ret = fm_stat_response__pack(&s_Response, s_ResponseData);
    if (s_Ret != s_ResponseSize)
    {
        WriteLog(LL_Error, "could not pack (%lld) != (%lld)", s_Ret, s_ResponseSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        delete [] s_ResponseData;
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_Stat, 0, s_ResponseData, s_ResponseSize);

    delete [] s_ResponseData;
}

void FileManager::OnUnlink(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}

    if (p_Message->data.data == nullptr)
    {
        WriteLog(LL_Error, "invalid message");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    FmUnlinkRequest* s_Request = fm_unlink_request__unpack(nullptr, p_Message->data.len, p_Message->data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack unlink request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    auto s_Ret = kunlink_t(s_Request->path, s_IoThread);
    if (s_Ret < 0)
    {
        fm_unlink_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "could not unlink (%d)", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    fm_unlink_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__FILE, FileManager_Unlink, 0, nullptr, 0);
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

void FileManager::OnDecryptSelf(Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
	auto s_SyscoreThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_SyscoreThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return;
	}
    WriteLog(LL_Debug, "syscoreThread: %p", s_SyscoreThread);/*

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
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOMEM);
        return;
    }

    // Do some sanity checking
    auto s_Request = reinterpret_cast<const DecryptSelfRequest*>(p_Message.Buffer);
    if (s_Request->PathLength >= ARRAYSIZE(s_Request->Path))
    {
        WriteLog(LL_Error, "invalid path length");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -E2BIG);
        return;
    }
    WriteLog(LL_Debug, "request: %p, pathLength: %d", s_Request, s_Request->PathLength);

    // Open the SELF file for reading
    auto s_SelfHandle = kopen_t(s_Request->Path, O_RDONLY, 0777, s_MainThread);
    WriteLog(LL_Debug, "selfHandle: %d", s_SelfHandle);
    if (s_SelfHandle < 0)
    {
        WriteLog(LL_Error, "could not open self (%s) err: (%d).", s_Request->Path, s_SelfHandle);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, s_SelfHandle);
        return;
    }

    size_t s_ElfDataSize = 0;

    auto s_ElfData = DecryptSelfFd(s_SelfHandle, &s_ElfDataSize);
    if (s_ElfData == nullptr || s_ElfDataSize == 0)
    {
        WriteLog(LL_Error, "could not decrypt self");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__FILE, -ENOEXEC);
        return;
    }

    WriteLog(LL_Debug, "here");
    RpcTransport s_Message;
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
            .magic = RpcTransportHeaderMagic,
            .category = RPC_CATEGORY__FILE,
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
            .magic = RpcTransportHeaderMagic,
            .category = RPC_CATEGORY__FILE,
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
        .magic = RpcTransportHeaderMagic,
        .category = RPC_CATEGORY__FILE,
        .isRequest = false,
        .errorType = static_cast<uint64_t>(0),
        .payloadLength = sizeof(s_Payload),
        .padding = 0
    };
    s_Message.Buffer = (const uint8_t*)&s_Payload;

    delete[] s_ElfData;
    WriteLog(LL_Debug, "here");

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, s_Message);
    WriteLog(LL_Error, "self decryption complete");*/
}

uint8_t* FileManager::DecryptSelfFd(int p_SelfFd, size_t* p_OutElfSize)
{
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
		return nullptr;
	}

    size_t s_Offset = klseek_t(p_SelfFd, 0, SEEK_END, s_IoThread);
    if (s_Offset <= 0)
    {
        WriteLog(LL_Error, "invalid offset (%lld).", s_Offset);
        return nullptr;
    }

    size_t s_SelfSize = s_Offset;

    // This is unsigned, will never be < 0
    s_Offset = klseek_t(p_SelfFd, 0, SEEK_SET, s_IoThread);

    auto s_SelfData = new uint8_t[s_SelfSize];
    if (s_SelfData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate self data (%llx).", s_SelfSize);
        return nullptr;
    }
    memset(s_SelfData, 0, s_SelfSize);

    s_Offset = kread_t(p_SelfFd, s_SelfData, s_SelfSize, s_IoThread);
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
	auto s_IoThread = Mira::Framework::GetFramework()->GetSyscoreThread();
	if (s_IoThread == nullptr)
	{
		WriteLog(LL_Error, "could not get io thread.");
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
            klseek_t(p_SelfFd, 0, SEEK_SET, s_IoThread);
            auto l_ElfSegment = kmmap_t(nullptr, s_Phdr[l_PhIndex].p_filesz, PROT_READ, MAP_SHARED | MAP_SELF | MAP_PREFAULT_READ, p_SelfFd, (((uint64_t)l_PhIndex) << 32), s_IoThread);
            
            WriteLog(LL_Warn, "%p = mmap(%p, 0x%llx, %d, %d, %d, %llx, %p)", l_ElfSegment, nullptr, s_Phdr[l_PhIndex].p_filesz, PROT_READ, MAP_PRIVATE | MAP_SELF, p_SelfFd, (((uint64_t)l_PhIndex) << 32), s_IoThread);
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
            kmunmap_t(l_ElfSegment, s_Phdr[l_PhIndex].p_filesz, s_IoThread);
        }
    }

    return s_ElfData;
}