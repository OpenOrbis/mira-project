// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Debugger2.hpp"

extern "C"
{
    #include <sys/ptrace.h>
    #include <machine/reg.h>
};

using namespace Mira;
using namespace Mira::Plugins;

void Debugger2::OnAttach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgAttachRequest* s_Request = dbg_attach_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack attach request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    WriteLog(LL_Info, "request to attach to (%d).", s_Request->processid);

    int32_t s_Ret = -1;
    if (!s_Debugger->Attach(s_Request->processid, &s_Ret))
    {
        dbg_attach_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "could not attach ret (%d)", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    WriteLog(LL_Debug, "attached to: (%d).", s_Debugger->m_AttachedPid);

    /*DbgThreadFull s_ThreadFull = DBG_THREAD_FULL__INIT;

    if (!s_Debugger->GetThreadFullInfo(curthread, &s_ThreadFull))
    {
        WriteLog(LL_Error, "get thread full info failed.");
    }
    else
    {
        WriteLog(LL_Error, "worked fine");
    }*/

    dbg_attach_request__free_unpacked(s_Request, nullptr);
    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_Attach, s_Debugger->m_AttachedPid, nullptr, 0);
}

void Debugger2::OnDetach(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgDetachRequest* s_Request = dbg_detach_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack detach request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    int32_t s_Ret = -1;
    if (!s_Debugger->Detach(s_Request->force, &s_Ret))
    {
        dbg_detach_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "could not detach ret (%d)", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    WriteLog(LL_Debug, "successfully detached");
    dbg_detach_request__free_unpacked(s_Request, nullptr);
    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_Attach, 0, nullptr, 0);
}

void Debugger2::OnThreadSinglestep(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (!s_Debugger->Step())
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_ThreadSinglestep, 0, nullptr, 0);
}

void Debugger2::OnGetProcList(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    WriteLog(LL_Error, "here");
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    WriteLog(LL_Error, "here");
    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    WriteLog(LL_Error, "here");

    // struct sx* allproclock = (struct sx*)kdlsym(allproc_lock);
    // struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);
    // auto __sx_slock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_slock);
	// auto __sx_sunlock = (void(*)(struct sx *sx, const char *file, int line))kdlsym(_sx_sunlock);
	// auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	// auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    // auto critical_enter = (void(*)(void))kdlsym(critical_enter);
	// auto critical_exit = (void(*)(void))kdlsym(critical_exit);

    Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);

    // Vector<DbgProcessLimited*> s_Vector;

    // __sx_slock(allproclock, 0, LOCK_FILE, LOCK_LINE);
    // struct proc* s_Proc = nullptr;
    // LIST_FOREACH(s_Proc, allproc, p_list)
    // {
    //     PROC_LOCK(s_Proc);

    //     // Allocate a new limited proc
    //     auto s_ProcessLimited = new DbgProcessLimited();
    //     if (s_ProcessLimited == nullptr)
    //     {
    //         WriteLog(LL_Error, "could not allocate dbg process limited.");
    //         PROC_UNLOCK(s_Proc);
    //         continue;
    //     }

    //     // Initialize said proc
    //     *s_ProcessLimited = DBG_PROCESS_LIMITED__INIT;

    //     auto s_ProcessNameLength = sizeof(s_Proc->p_comm);
    //     auto s_ProcessName = new char[s_ProcessNameLength];
    //     if (s_ProcessName == nullptr)
    //     {
    //         WriteLog(LL_Error, "could not allocate process name.");
    //         delete s_ProcessLimited;
    //         PROC_UNLOCK(s_Proc);
    //         continue;
    //     }
    //     memset(s_ProcessName, 0, s_ProcessNameLength);
    //     memcpy(s_ProcessName, s_Proc->p_comm, s_ProcessNameLength);
    //     s_ProcessLimited->name = s_ProcessName;

    //     s_ProcessLimited->processid = s_Proc->p_pid;

    //     DbgVmEntry** s_Entries = nullptr;
    //     size_t s_EntriesCount = 0;

    //     if (!GetVmMapEntries(s_Proc, true, s_Entries, s_EntriesCount))
    //         WriteLog(LL_Warn, "could not get vm entries.");
    //     else
    //     {
    //         s_ProcessLimited->n_entries = s_EntriesCount;
    //         s_ProcessLimited->entries = s_Entries;
    //     }

    //     s_Vector.push_back(s_ProcessLimited);
        
    //     PROC_UNLOCK(s_Proc);
    // }
    // __sx_sunlock(allproclock, LOCK_FILE, LOCK_LINE);
    
    // //WriteLog(LL_Debug, "here");

    // do
    // {
    //     // Vector should be filled with everything we need
    //     auto s_Response = new DbgGetProcessListResponse();
    //     if (s_Response == nullptr)
    //     {
    //         WriteLog(LL_Error, "could not allocate db get process list response.");
    //         Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
    //         break;
    //     }
    //     *s_Response = DBG_GET_PROCESS_LIST_RESPONSE__INIT;

    //     //WriteLog(LL_Debug, "here");

    //     auto s_VectorSize = s_Vector.size();
    //     auto s_EntryList = new DbgProcessLimited*[s_VectorSize];
    //     if (s_EntryList == nullptr)
    //     {
    //         WriteLog(LL_Error, "could not allocate outgoing entry list.");
    //         delete s_Response;
    //         Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
    //         break;
    //     }

    //     //WriteLog(LL_Debug, "here");

    //     // Copy over the entry list out
    //     for (auto i = 0; i < s_VectorSize; ++i)
    //         s_EntryList[i] = s_Vector.at(i);

    //     //WriteLog(LL_Debug, "here");
        
    //     s_Response->n_processes = s_VectorSize;
    //     s_Response->processes = s_EntryList;

    //     //WriteLog(LL_Debug, "here");

    //     auto s_MessageSize = dbg_get_process_list_response__get_packed_size(s_Response);
    //     auto s_MessageData = new uint8_t[s_MessageSize];
    //     if (s_MessageData == nullptr)
    //     {
    //         WriteLog(LL_Error, "could not allocate message data.");
    //         delete s_Response;
    //         delete [] s_EntryList;
    //         Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
    //         break;
    //     }
    //     memset(s_MessageData, 0, s_MessageSize);

    //     //WriteLog(LL_Debug, "here");

    //     auto s_PackedSize = dbg_get_process_list_response__pack(s_Response, s_MessageData);
    //     if (s_PackedSize != s_MessageSize)
    //     {
    //         WriteLog(LL_Error, "could not pack message.");
    //         delete s_Response;
    //         delete [] s_EntryList;
    //         Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
    //         break;
    //     }

    //     //WriteLog(LL_Debug, "here");

    //     Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_GetProcList, 0, s_MessageData, s_MessageSize);

    //     delete s_Response;
    //     delete [] s_EntryList;

    //     //WriteLog(LL_Debug, "here");
    // } while (false);
    
    // //WriteLog(LL_Debug, "here");

    // // Cleanup everything
    // for (auto l_EntryIndex = 0; l_EntryIndex < s_Vector.size(); ++l_EntryIndex)
    // {
    //     auto l_Entry = s_Vector.at(l_EntryIndex);
    //     if (l_Entry == nullptr)
    //         continue;
        
    //     //WriteLog(LL_Debug, "here");
    //     delete [] l_Entry->name;
    //     l_Entry->name = nullptr;

    //     //WriteLog(LL_Debug, "here");
    //     // Free all of the vm entries that were allocated
    //     for (auto l_VmEntryIndex = 0; l_VmEntryIndex < l_Entry->n_entries; ++l_VmEntryIndex)
    //     {
    //         auto l_VmEntry = l_Entry->entries[l_VmEntryIndex];
    //         if (l_VmEntry == nullptr)
    //             continue;
            
    //         //WriteLog(LL_Debug, "here");

    //         delete [] l_VmEntry->name;
    //         l_VmEntry->name = nullptr;

    //         //WriteLog(LL_Debug, "here");
    //         delete l_VmEntry;
    //         l_Entry->entries[l_VmEntryIndex] = nullptr;
    //     }

    //     //WriteLog(LL_Debug, "here");

    //     delete l_Entry;
    // }

    //WriteLog(LL_Debug, "here");
}

void Debugger2::OnReadProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgReadProcessMemoryRequest* s_Request = dbg_read_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack rpm request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    // Get the address
    auto s_Address = s_Request->address;
    if (s_Address == 0)
    {
        WriteLog(LL_Error, "invalid address");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EINVAL);
        return;
    }

    // Allocate buffer for the response
    size_t s_BufferSize = s_Request->size;
    auto s_Buffer = new uint8_t[s_BufferSize];
    if (s_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate buffer of size (%x).", s_BufferSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_Buffer, 0, s_BufferSize);

    auto s_Ret = proc_rw_mem_pid(s_Debugger->m_AttachedPid, reinterpret_cast<void*>(s_Request->address), s_Request->size, s_Buffer, &s_BufferSize, 0);
    if (s_Ret < 0)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not read (%p) size (%x).", s_BufferSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgReadProcessMemoryResponse s_Response = DBG_READ_PROCESS_MEMORY_RESPONSE__INIT;
    s_Response.data.data = s_Buffer;
    s_Response.data.len = s_BufferSize;

    auto s_SerializedSize = dbg_read_process_memory_response__get_packed_size(&s_Response);
    if (s_SerializedSize == 0)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "invalid packed size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_SerializedData = new uint8_t[s_SerializedSize];
    if (s_SerializedData == nullptr)
    {
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not allocate serialized data of size (%x).", s_SerializedSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_SerializedData, 0, s_SerializedSize);

    auto s_PackedRet = dbg_read_process_memory_response__pack(&s_Response, s_SerializedData);
    if (s_PackedRet != s_SerializedSize)
    {
        delete [] s_SerializedData;
        delete [] s_Buffer;
        WriteLog(LL_Error, "could not allocate serialized data of size (%x).", s_SerializedSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_ReadMem, 0, s_SerializedData, s_SerializedSize);

    delete [] s_SerializedData;
    delete [] s_Buffer;
}

void Debugger2::OnWriteProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgWriteProcessMemoryRequest* s_Request = dbg_write_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack wpm request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    // validate our incoming data
    if (s_Request->data.data == nullptr || s_Request->data.len == 0)
    {
        dbg_write_process_memory_request__free_unpacked(s_Request, nullptr);

        WriteLog(LL_Error, "could not get data to write");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    // Write to the attached process
    size_t s_DataLength = s_Request->data.len;
    auto s_Ret = proc_rw_mem_pid(s_Debugger->m_AttachedPid, reinterpret_cast<void*>(s_Request->address), s_Request->data.len, s_Request->data.data, &s_DataLength, 1);
    if (s_Ret < 0)
    {
        dbg_write_process_memory_request__free_unpacked(s_Request, nullptr);

        WriteLog(LL_Error, "could not write to process ret (%d).", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    dbg_write_process_memory_request__free_unpacked(s_Request, nullptr);
    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_WriteMem, 0, nullptr, 0);
}

void Debugger2::OnProtectProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgProtectProcessMemoryRequest* s_Request = dbg_protect_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack protect proc memory request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (s_Request->address == 0)
    {
        dbg_protect_process_memory_request__free_unpacked(s_Request, nullptr);
        WriteLog(LL_Error, "attempted to write to null page");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EFAULT);
        return;
    }

    auto s_Ret = kmprotect_t(reinterpret_cast<void*>(s_Request->address), s_Request->length, s_Request->protection, s_DebuggerThread);

    dbg_protect_process_memory_request__free_unpacked(s_Request, nullptr);

    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "mprotect failed (%d).", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_ProtectMem, s_Ret, nullptr, 0);
}

void Debugger2::OnGetProcessInfo(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgGetProcessInfoRequest* s_Request = dbg_get_process_info_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack get process info request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ProcessId = s_Request->processid;

    dbg_get_process_info_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    DbgProcessFull s_ProcessInfo = DBG_PROCESS_FULL__INIT;
    if (!s_Debugger->GetProcessFullInfo(s_ProcessId, &s_ProcessInfo))
    {
        
        WriteLog(LL_Error, "could not get process info");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EEXIST);
        return;
    }

    auto s_MessageSize = dbg_process_full__get_packed_size(&s_ProcessInfo);
    if (s_MessageSize == 0)
    {
        WriteLog(LL_Error, "invalid message size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    uint8_t* s_MessageData = new uint8_t[s_MessageSize];
    if (s_MessageData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate message data (%llx)", s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_MessageData, 0, s_MessageSize);
    
    auto s_PackedSize = dbg_process_full__pack(&s_ProcessInfo, s_MessageData);
    if (s_PackedSize != s_MessageSize)
    {
        WriteLog(LL_Error, "packed (%llx) != msg (%llx)", s_PackedSize, s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_GetProcInfo, 0, s_MessageData, s_MessageSize);

    delete [] s_MessageData;
}

void Debugger2::OnGetThreadInfo(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgGetThreadInfoRequest* s_Request = dbg_get_thread_info_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack get thread info request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    auto s_ThreadId = s_Request->threadid;
    dbg_get_thread_info_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    DbgThreadFull s_Response = DBG_THREAD_FULL__INIT;
    if (!s_Debugger->GetThreadFullInfo(s_ThreadId, &s_Response))
    {
        WriteLog(LL_Error, "could not get thread full info");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_MessageSize = dbg_thread_full__get_packed_size(&s_Response);
    if (s_MessageSize == 0)
    {
        WriteLog(LL_Error, "invalid message size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    uint8_t* s_MessageData = new uint8_t[s_MessageSize];
    if (s_MessageData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate message data (%llx)", s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_MessageData, 0, s_MessageSize);
    
    auto s_PackedSize = dbg_thread_full__pack(&s_Response, s_MessageData);
    if (s_PackedSize != s_MessageSize)
    {
        WriteLog(LL_Error, "packed (%llx) != msg (%llx)", s_PackedSize, s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        delete [] s_MessageData;
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_GetThreadInfo, 0, s_MessageData, s_MessageSize);

    delete [] s_MessageData;
}

void Debugger2::OnGetProcThreads(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgGetProcessThreadsRequest* s_Request = dbg_get_process_threads_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack get process threads request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    uint64_t s_ThreadCount = s_Debugger->GetProcessThreadCount(s_Request->processid);

    DbgGetProcessThreadsResponse s_Response = DBG_GET_PROCESS_THREADS_RESPONSE__INIT;
    DbgThreadLimited **s_DbgThreads = new DbgThreadLimited*[s_ThreadCount];

    if(!s_Debugger->GetProcessThreads(s_DbgThreads, s_ThreadCount))
    {
        if (s_DbgThreads != nullptr)
            delete [] s_DbgThreads;
        
        WriteLog(LL_Error, "could not get process threads");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    s_Response.n_threads = (size_t)s_ThreadCount;
    s_Response.threads = s_DbgThreads;

    dbg_get_process_threads_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    auto s_MessageSize = dbg_get_process_threads_response__get_packed_size(&s_Response);
    if (s_MessageSize == 0)
    {
        WriteLog(LL_Error, "invalid message size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    uint8_t* s_MessageData = new uint8_t[s_MessageSize];
    if (s_MessageData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate message data (%llx)", s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_MessageData, 0, s_MessageSize);
    
    auto s_PackedSize = dbg_get_process_threads_response__pack(&s_Response, s_MessageData);
    if (s_PackedSize != s_MessageSize)
    {
        WriteLog(LL_Error, "packed (%llx) != msg (%llx)", s_PackedSize, s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_GetProcThreads, 0, s_MessageData, s_MessageSize);

    delete [] s_MessageData;
}

void Debugger2::OnAllocateProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ProcessMainThread = s_Debugger->GetProcessMainThread();
    if (s_ProcessMainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get process main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgAllocateProcessMemoryRequest* s_Request = dbg_allocate_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack allocate request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    size_t s_ResponseSize = 0;
    size_t s_PackedSize = 0;
    uint8_t* s_ResponseData = nullptr;
    DbgAllocateProcessMemoryResponse s_Response = DBG_ALLOCATE_PROCESS_MEMORY_RESPONSE__INIT;
    auto s_AllocatedData = kmmap_t(0, s_Request->size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_PREFAULT_READ, -1, 0, s_ProcessMainThread);
    if (s_AllocatedData == MAP_FAILED || s_AllocatedData == 0)
    {
        WriteLog(LL_Error, "could not allocate memory");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        goto cleanup;
    }
    s_Response.address = reinterpret_cast<uint64_t>(s_AllocatedData);
    
    s_ResponseSize = dbg_allocate_process_memory_response__get_packed_size(&s_Response);
    if (s_ResponseSize == 0)
    {
        WriteLog(LL_Error, "could not get packed size");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        goto cleanup;
    }

    s_ResponseData = new uint8_t[s_ResponseSize];
    if (s_ResponseData)
    {
        WriteLog(LL_Error, "could not allocate response data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        goto cleanup;
    }
    memset(s_ResponseData, 0, s_ResponseSize);

    s_PackedSize = dbg_allocate_process_memory_response__pack(nullptr, s_ResponseData);
    if (s_PackedSize != s_ResponseSize)
    {
        WriteLog(LL_Error, "packed (%llx) != msg (%llx)", s_PackedSize, s_ResponseSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_AllocateProcMem, 0, s_ResponseData, s_ResponseSize);

cleanup:
    if (s_Request != nullptr)
    {
        dbg_allocate_process_memory_request__free_unpacked(s_Request, nullptr);
        s_Request = nullptr;
    }

    if (s_ResponseData != nullptr)
    {
        delete [] s_ResponseData;
    }
}

void Debugger2::OnFreeProcessMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ProcessMainThread = s_Debugger->GetProcessMainThread();
    if (s_ProcessMainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get process main thread");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgFreeProcessMemoryRequest* s_Request = dbg_free_process_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack free request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    auto s_Ret = kmunmap_t(reinterpret_cast<void*>(s_Request->address), s_Request->size, s_ProcessMainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "munmap returned (%d).", s_Ret);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        goto cleanup;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_FreeProcMem, 0, nullptr, 0);


cleanup:
    if (s_Request != nullptr)
        dbg_free_process_memory_request__free_unpacked(s_Request, nullptr);

}

void Debugger2::OnAddBreakpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    WriteLog(LL_Error, "breakpoints not implemented");

    Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EEXIST);
}

void Debugger2::OnAddWatchpoint(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    WriteLog(LL_Error, "watchpoints not implemented");

    Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EEXIST);
}

void Debugger2::OnSignalProcess(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgSignalProcessRequest* s_Request = dbg_signal_process_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack signal request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    auto s_Ret = kkill_t(s_Debugger->m_AttachedPid, s_Request->signal, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not signal pid");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        goto cleanup;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_SignalProc, 0, nullptr, 0);

cleanup:
    if (s_Request != nullptr)
        dbg_signal_process_request__free_unpacked(s_Request, nullptr);
}


void Debugger2::OnReadKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgReadKernelMemoryRequest* s_Request = dbg_read_kernel_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack read kernel memory request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    const void* s_Address = reinterpret_cast<void*>(s_Request->address);
    size_t s_DataLen = s_Request->size;

    dbg_read_kernel_memory_request__free_unpacked(s_Request, nullptr);
    s_Request = nullptr;

    uint8_t* s_Data = new uint8_t[s_DataLen];
    if (s_Data == nullptr)
    {
        WriteLog(LL_Error, "could not allocate memory");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_Data, 0, s_DataLen);

    auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);

    auto s_Ret = vm_fault_disable_pagefaults();
    memcpy(s_Data, s_Address, s_DataLen);
    vm_fault_enable_pagefaults(s_Ret);

    DbgReadKernelMemoryResponse s_Response = DBG_READ_KERNEL_MEMORY_RESPONSE__INIT;
    s_Response.data.data = s_Data;
    s_Response.data.len = s_DataLen;

    auto s_MessageSize = dbg_read_kernel_memory_response__get_packed_size(&s_Response);
    auto s_MessageData = new uint8_t[s_MessageSize];
    if (s_MessageData == nullptr)
    {
        delete [] s_Data;

        WriteLog(LL_Error, "could not allocate message data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    memset(s_MessageData, 0, s_MessageSize);

    auto s_PackedSize = dbg_read_kernel_memory_response__pack(&s_Response, s_MessageData);
    if (s_PackedSize != s_MessageSize)
    {
        delete [] s_Data;
        delete [] s_MessageData;

        WriteLog(LL_Error, "packed (%llx) != msg size (%llx)", s_PackedSize, s_MessageSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    delete [] s_Data;

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_ReadKernelMem, 0, s_MessageData, s_MessageSize);

    delete [] s_MessageData;
}

void Debugger2::OnWriteKernelMemory(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgWriteKernelMemoryRequest* s_Request = dbg_write_kernel_memory_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack write kernel memory request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -EIO);
        return;
    }

    void* s_Address = reinterpret_cast<void*>(s_Request->address);

    auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);

    auto s_Ret = vm_fault_disable_pagefaults();
    memcpy(s_Address, (const void*)s_Request->data.data, s_Request->data.len);
    vm_fault_enable_pagefaults(s_Ret);

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_WriteKernelMem, 0, nullptr, 0);
}

void Debugger2::OnGetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgGetRegistersRequest* s_Request = dbg_get_registers_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not unpack get registers request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }
    
    // Allocate a new DbgThreadFull
    DbgGetRegistersResponse s_Response;
    size_t s_RequestedSize = 0;
    size_t s_ActualSize = 0;
    uint8_t* s_PackedData = nullptr;
    DbgThreadFull* s_Thread = new DbgThreadFull();
    if (s_Thread == nullptr)
    {
        WriteLog(LL_Error, "could not allocate dbg thread full");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    // Get the thread information
    if (!s_Debugger->GetThreadFullInfo(s_Request->threadid, s_Thread))
    {
        WriteLog(LL_Error, "could not get thread full info");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    s_Response = DBG_GET_REGISTERS_RESPONSE__INIT;
    s_Response.dbregisters = s_Thread->dbregisters;
    s_Response.fpregisters = s_Thread->fpregisters;
    s_Response.gpregisters = s_Thread->gpregisters;

    // Get the packed size
    s_RequestedSize = dbg_get_registers_response__get_packed_size(&s_Response);
    if (s_RequestedSize == 0)
    {
        WriteLog(LL_Error, "could not get thread full info");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    s_PackedData = new uint8_t[s_RequestedSize];
    if (s_PackedData == nullptr)
    {
        WriteLog(LL_Error, "could not get thread full info");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    s_ActualSize = dbg_get_registers_response__pack(&s_Response, s_PackedData);
    if (s_ActualSize != s_RequestedSize)
    {
        WriteLog(LL_Error, "actual size (%llx) != requested size (%llx)", s_ActualSize, s_RequestedSize);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_GetRegs, 0, s_PackedData, s_RequestedSize);

cleanup:
    if (s_Request)
        dbg_get_registers_request__free_unpacked(s_Request, nullptr);
    
    if (s_Thread)
    {
        // Free any resources that may have been allocated for DbgThreadFull
        FreeDbgThreadFull(s_Thread);

        // Free the actual object itself
        delete s_Thread;
    }

    if (s_PackedData)
        delete [] s_PackedData;
}

void Debugger2::OnSetThreadRegisters(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message)
{
    if (p_Message.data.data == nullptr || p_Message.data.len <= 0)
    {
        WriteLog(LL_Error, "could not get data");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
		return;
	}

    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    auto s_Debugger = static_cast<Debugger2*>(s_PluginManager->GetDebugger());
    if (s_Debugger == nullptr)
    {
        WriteLog(LL_Error, "could not get debugger");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    DbgSetRegistersRequest* s_Request = dbg_set_registers_request__unpack(nullptr, p_Message.data.len, p_Message.data.data);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "could not read set registers request");
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        return;
    }

    int32_t s_Ret = 0;
    size_t s_DbgRegIndex = 0;
    struct reg s_Reg;
    struct fpreg s_FpReg;
    struct dbreg s_DbReg;

    auto s_Thread = s_Debugger->GetThreadById(s_Request->threadid);
    if (s_Thread == nullptr)
    {
        WriteLog(LL_Error, "could not get thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    if (!s_Debugger->Suspend())
    {
        WriteLog(LL_Error, "could not get thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    s_Reg =
    {
        .r_r15 = (register_t)s_Request->gpregisters->r_r15,
        .r_r14 = (register_t)s_Request->gpregisters->r_r14,
        .r_r13 = (register_t)s_Request->gpregisters->r_r13,
        .r_r12 = (register_t)s_Request->gpregisters->r_r12,
        .r_r11 = (register_t)s_Request->gpregisters->r_r11,
        .r_r10 = (register_t)s_Request->gpregisters->r_r10,
        .r_r9 = (register_t)s_Request->gpregisters->r_r9,
        .r_r8 = (register_t)s_Request->gpregisters->r_r8,
        .r_rdi = (register_t)s_Request->gpregisters->r_rdi,
        .r_rsi = (register_t)s_Request->gpregisters->r_rsi,
        .r_rbp = (register_t)s_Request->gpregisters->r_rbp,
        .r_rbx = (register_t)s_Request->gpregisters->r_rbx,
        .r_rdx = (register_t)s_Request->gpregisters->r_rdx,
        .r_rcx = (register_t)s_Request->gpregisters->r_rcx,
        .r_rax = (register_t)s_Request->gpregisters->r_rax,

        .r_trapno = s_Request->gpregisters->r_trapno,
        .r_fs = (uint16_t)s_Request->gpregisters->r_fs,
        .r_gs = (uint16_t)s_Request->gpregisters->r_gs,
        .r_err = s_Request->gpregisters->r_err,
        .r_es = (uint16_t)s_Request->gpregisters->r_es,
        .r_ds = (uint16_t)s_Request->gpregisters->r_ds,

        .r_rip = (register_t)s_Request->gpregisters->r_rip,
        .r_cs = (register_t)s_Request->gpregisters->r_cs,
        .r_rflags = (register_t)s_Request->gpregisters->r_rflags,
        .r_rsp = (register_t)s_Request->gpregisters->r_rsp,
        .r_ss = (register_t)s_Request->gpregisters->r_ss,
    };

    memcpy(&s_FpReg, s_Request->fpregisters->data.data, s_Request->fpregisters->data.len);

    for (s_DbgRegIndex = 0; s_DbgRegIndex < s_Request->dbregisters->n_debugregs; ++s_DbgRegIndex)
    {
        // Bounds check
        if (s_DbgRegIndex >= ARRAYSIZE(s_DbReg.dr))
            break;
        
        s_DbReg.dr[s_DbgRegIndex] = s_Request->dbregisters->debugregs[s_DbgRegIndex];
    }

    s_Ret = kptrace_t(PT_SETREGS, s_Debugger->m_AttachedPid, (caddr_t)&s_Reg, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not set regs, thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        goto cleanup;
    }

    s_Ret = kptrace_t(PT_SETFPREGS, s_Debugger->m_AttachedPid, (caddr_t)&s_FpReg, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not set fpregs, thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        goto cleanup;
    }

    s_Ret = kptrace_t(PT_SETDBREGS, s_Debugger->m_AttachedPid, (caddr_t)&s_DbReg, 0, s_DebuggerThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not set dbregs, thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, s_Ret);
        goto cleanup;
    }

    if (!s_Debugger->Resume())
    {
        WriteLog(LL_Error, "could not get thread tid: (%d).", s_Request->threadid);
        Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -ENOMEM);
        goto cleanup;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendResponse(p_Connection, RPC_CATEGORY__DEBUG, DbgCmd_SetRegs, 0, nullptr, 0);

cleanup:
    if (s_Request)
        dbg_set_registers_request__free_unpacked(s_Request, nullptr);
}


bool Debugger2::GetProcessThreads(DbgThreadLimited** p_OutThreads, uint32_t p_ThreadCount)
{
    if (p_OutThreads == nullptr)
        return false;
    
    // Zero out the entire allocated array
    for (auto i = 0; i < p_ThreadCount; ++i)
        p_OutThreads[i] = nullptr;

    if (m_AttachedPid < 0)
	{
        WriteLog(LL_Error, "invalid pid (%d)", m_AttachedPid);
        return false;
    }

    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
    auto _thread_lock_flags = (void(*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);
    struct proc* s_Process = pfind(m_AttachedPid);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_AttachedPid);
		return false;
	}
    _mtx_unlock_flags(&s_Process->p_mtx, 0, __FILE__, __LINE__);

    uint64_t s_ThreadCount = 0;
    struct thread* s_Thread = nullptr;
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
        s_ThreadCount++;
    
    s_Thread = nullptr;

    uint64_t s_CurrentThreadIndex = 0;
    FOREACH_THREAD_IN_PROC(s_Process, s_Thread)
    {
        thread_lock(s_Thread);

        if (s_CurrentThreadIndex >= s_ThreadCount)
        {
            thread_unlock(s_Thread);
            WriteLog(LL_Error, "there was a new thread spun up during iteration");
            break;
        }

        DbgThreadLimited* l_Thread = new DbgThreadLimited;
        if (l_Thread == nullptr)
        {
            thread_unlock(s_Thread);
            s_CurrentThreadIndex++;
            WriteLog(LL_Error, "could not allocate thread limited");
            continue;
        }
        
        if (!GetThreadLimitedInfo(s_Thread, l_Thread))
        {
            thread_unlock(s_Thread);

            delete l_Thread;
            s_CurrentThreadIndex++;
            WriteLog(LL_Error, "could not set thread limited info");
            continue;
        }

        thread_unlock(s_Thread);
        p_OutThreads[s_CurrentThreadIndex] = l_Thread;
        s_CurrentThreadIndex++;
    }

    return true;
}