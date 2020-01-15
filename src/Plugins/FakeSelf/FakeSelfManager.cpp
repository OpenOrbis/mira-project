#include "FakeSelfManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <Mira.hpp>
#include <Boot/Config.hpp>
#include <Plugins/PluginManager.hpp>

using namespace Mira::Plugins;

SelfContext* FakeSelfManager::m_LastContext = nullptr;

const uint8_t FakeSelfManager::c_ExecAuthInfo[] =
{
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x20,
	0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
	0x00, 0x40, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t FakeSelfManager::c_DynlibAuthInfo[] =
{
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x30, 0x00, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
	0x00, 0x40, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

FakeSelfManager::FakeSelfManager()
{
    m_SceSblServiceMailboxHook = new Utils::Hook(kdlsym(sceSblServiceMailbox), reinterpret_cast<void*>(OnSceSblServiceMailbox));
    m_SceSblAuthMgrVerifyHeaderHook = new Utils::Hook(kdlsym(sceSblAuthMgrVerifyHeader), reinterpret_cast<void*>(OnSceSblAuthMgrVerifyHeader));
    m_SceSblAuthMgrIsLoadable2Hook = new Utils::Hook(kdlsym(sceSblAuthMgrIsLoadable2), reinterpret_cast<void*>(OnSceSblAuthMgrIsLoadable2));
    //m_SceSblACMgrGetPathIdHook = new Utils::Hook(kdlsym(sceSblACMgrGetPathId), OnSceSblACMgrGetPathId);

    m__SceSblAuthMgrSmLoadSelfBlockHook = new Utils::Hook(kdlsym(_sceSblAuthMgrSmLoadSelfBlock), reinterpret_cast<void*>(On_SceSblAuthMgrSmLoadSelfBlock));
    m__SceSblAuthMgrSmLoadSelfSegmentHook = new Utils::Hook(kdlsym(_sceSblAuthMgrSmLoadSelfSegment), reinterpret_cast<void*>(On_SceSblAuthMgrSmLoadSelfSegment));
}

FakeSelfManager::~FakeSelfManager()
{
    if (m_SceSblServiceMailboxHook != nullptr)
    {
        (void)m_SceSblServiceMailboxHook->Disable();
        delete m_SceSblServiceMailboxHook;
        m_SceSblServiceMailboxHook = nullptr;
    }

    if (m_SceSblAuthMgrVerifyHeaderHook != nullptr)
    {
        (void)m_SceSblAuthMgrVerifyHeaderHook->Disable();
        delete m_SceSblAuthMgrVerifyHeaderHook;
        m_SceSblAuthMgrVerifyHeaderHook = nullptr;
    }

    if (m_SceSblAuthMgrIsLoadable2Hook != nullptr)
    {
        (void)m_SceSblAuthMgrIsLoadable2Hook->Disable();
        delete m_SceSblAuthMgrIsLoadable2Hook;
        m_SceSblAuthMgrIsLoadable2Hook = nullptr;
    }

    if (m__SceSblAuthMgrSmLoadSelfBlockHook != nullptr)
    {
        (void)m__SceSblAuthMgrSmLoadSelfBlockHook->Disable();
        delete m__SceSblAuthMgrSmLoadSelfBlockHook;
        m__SceSblAuthMgrSmLoadSelfBlockHook = nullptr;
    }

    if (m__SceSblAuthMgrSmLoadSelfSegmentHook!= nullptr)
    {
        (void)m__SceSblAuthMgrSmLoadSelfSegmentHook->Disable();
        delete m__SceSblAuthMgrSmLoadSelfSegmentHook;
        m__SceSblAuthMgrSmLoadSelfSegmentHook = nullptr;
    }
}

int FakeSelfManager::OnSceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo)
{
    if (p_Context == nullptr)
    {
        WriteLog(LL_Error, "invalid context");
        return SceSblAuthMgrIsLoadable2(p_Context, p_OldAuthInfo, p_PathId, p_NewAuthInfo);
    } 
    
    if (p_Context->format == SelfFormat::Elf || IsFakeSelf(p_Context))
    {
        WriteLog(LL_Debug, "building fake self information");
        return BuildFakeSelfAuthInfo(p_Context, p_OldAuthInfo, p_NewAuthInfo);
    }        
    else
        return SceSblAuthMgrIsLoadable2(p_Context, p_OldAuthInfo, p_PathId, p_NewAuthInfo);
}

int FakeSelfManager::SceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        return -1;
    }

    auto s_Manager = static_cast<FakeSelfManager*>(s_PluginManager->GetFakeSelfManager());
    if (s_Manager == nullptr)
    {
        WriteLog(LL_Error, "could not get fake self manager instance");
        return -1;
    }

    auto s_Hook = s_Manager->m_SceSblServiceMailboxHook;
    if (s_Hook == nullptr)
    {
        WriteLog(LL_Error, "could not find the sceSblServiceMailbox hook");
        return -1;
    }
    if (s_Hook->GetOriginalFunctionAddress() == nullptr)
        return -1;

    int32_t s_Ret = -1;

    if (!s_Hook->Disable())
        return -1;
    
    auto s_Call = (int(*)(uint32_t, void*, void*))s_Hook->GetOriginalFunctionAddress();
    
    // Call the original
    s_Ret = s_Call(p_ServiceId, p_Request, p_Response);

    (void)s_Hook->Enable();

    return s_Ret;
}

int FakeSelfManager::SceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        return -1;
    }

    auto s_Manager = static_cast<FakeSelfManager*>(s_PluginManager->GetFakeSelfManager());
    if (s_Manager == nullptr)
    {
        WriteLog(LL_Error, "could not get fake self manager instance");
        return -1;
    }

    auto s_Hook = s_Manager->m_SceSblAuthMgrIsLoadable2Hook;
    if (s_Hook == nullptr)
    {
        WriteLog(LL_Error, "could not find the sceSblAuthMgrIsLoadable2 hook");
        return -1;
    }
    if (s_Hook->GetOriginalFunctionAddress() == nullptr)
        return -1;

    int32_t s_Ret = -1;

    if (!s_Hook->Disable())
        return -1;
    
    auto s_Call = (int(*)(SelfContext*, SelfAuthInfo*, int32_t, SelfAuthInfo*))s_Hook->GetOriginalFunctionAddress();
    
    // Call the original
    s_Ret = s_Call(p_Context, p_OldAuthInfo, p_PathId, p_NewAuthInfo);

    (void)s_Hook->Enable();

    return s_Ret;
}


int FakeSelfManager::OnSceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto s_Request = static_cast<MailboxMessage*>(p_Request);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request");
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    // Only hook on the needed service id
    if (p_ServiceId != 0)
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    
    switch (s_Request->funcId)
    {
    case LoadSelfSegment:
        return SceSblAuthMgrSmLoadSelfSegment_Mailbox(p_ServiceId, p_Request, p_Response);
    case LoadSelfBlock:
        return SceSblAuthMgrSmLoadSelfBlock_Mailbox(p_ServiceId, p_Request, p_Response);
    default:
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }
}

int FakeSelfManager::SceSblAuthMgrVerifyHeader(SelfContext* p_Context)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        return -1;
    }

    auto s_Manager = static_cast<FakeSelfManager*>(s_PluginManager->GetFakeSelfManager());
    if (s_Manager == nullptr)
    {
        WriteLog(LL_Error, "could not get fake self manager instance");
        return -1;
    }

    auto s_Hook = s_Manager->m_SceSblAuthMgrVerifyHeaderHook;
    if (s_Hook == nullptr)
    {
        WriteLog(LL_Error, "could not find the sceSblAuthMgrVerifyHeader hook");
        return -1;
    }
    if (s_Hook->GetOriginalFunctionAddress() == nullptr)
        return -1;

    int32_t s_Ret = -1;

    if (!s_Hook->Disable())
        return -1;
    
    auto s_Call = (int(*)(SelfContext*))s_Hook->GetOriginalFunctionAddress();
    
    // Call the original
    s_Ret = s_Call(p_Context);

    (void)s_Hook->Enable();

    return s_Ret;
}

int FakeSelfManager::AuthSelfHeader(SelfContext* p_Context)
{    
    bool s_IsUnsigned = p_Context->format == SelfFormat::Elf || IsFakeSelf(p_Context);
    if (s_IsUnsigned)
    {
        WriteLog(LL_Debug, "fixing unsigned");
        auto s_OldFormat = p_Context->format;
        auto s_OldTotalHeaderSize = p_Context->totalHeaderSize;

        // Get the mini-syscore.elf binary
        SelfHeader* s_Header = (SelfHeader*)kdlsym(mini_syscore_self_binary);

        auto s_NewTotalHeaderSize = s_Header->headerSize + s_Header->metaSize;

        // Allocate some memory to hold our header size
        auto s_Temp = new uint8_t[s_NewTotalHeaderSize];
        if (s_Temp == nullptr)
        {
            WriteLog(LL_Error, "could not allocate new total header size (%x).", s_NewTotalHeaderSize);
            return ENOMEM;
        }

        // Backup our current header
        memcpy(s_Temp, p_Context->header, s_NewTotalHeaderSize);

        // Copy over mini-syscore.elf's header
        memcpy(p_Context->header, s_Header, s_NewTotalHeaderSize);

        // Change the format
        p_Context->format = SelfFormat::Self;
        p_Context->totalHeaderSize = s_NewTotalHeaderSize;

        // xxx: call the original method using a real SELF file
        auto s_Result = SceSblAuthMgrVerifyHeader(p_Context);

        // Restore everything
        memcpy(p_Context->header, s_Temp, s_NewTotalHeaderSize);
        p_Context->format = s_OldFormat;
        p_Context->totalHeaderSize = s_OldTotalHeaderSize;

        delete [] s_Temp;

        return s_Result;
    }
    else
        return SceSblAuthMgrVerifyHeader(p_Context);
}

int FakeSelfManager::OnSceSblAuthMgrVerifyHeader(SelfContext* p_Context)
{
    auto _sceSblAuthMgrSmStart = (void(*)(void**))kdlsym(_sceSblAuthMgrSmStart);

    void* s_Temp = nullptr;
    _sceSblAuthMgrSmStart(&s_Temp);

    return AuthSelfHeader(p_Context);
}

SblMapListEntry* FakeSelfManager::SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa)
{
    if (p_GpuVa == 0)
    {
        WriteLog(LL_Error, "invalid gpu va");
        return nullptr;
    }
    
    SblMapListEntry* s_Entry = *(SblMapListEntry**)kdlsym(gpu_va_page_list);
    while (s_Entry)
    {
        if (s_Entry->gpuVa == p_GpuVa)
            return s_Entry;
        s_Entry = s_Entry->next;
    }

    return nullptr;
}

vm_offset_t FakeSelfManager::SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups)
{
    SblMapListEntry* s_Entry = SceSblDriverFindMappedPageListByGpuVa(p_GpuVa);
    if (s_Entry == nullptr)
    {
        WriteLog(LL_Error, "invalid gpu va entry");
        return 0;
    }
    
    if (p_NumPageGroups != nullptr)
        *p_NumPageGroups = s_Entry->numPageGroups;
    
    return s_Entry->cpuVa;
}

bool FakeSelfManager::IsFakeSelf(SelfContext* p_Context)
{
    auto _sceSblAuthMgrGetSelfInfo = (int (*)(SelfContext* ctx, void *exInfo))kdlsym(_sceSblAuthMgrGetSelfInfo);
    if (p_Context == nullptr)
    {
        WriteLog(LL_Error, "invalid context");
        return false;
    }
    
    SelfExInfo* s_Info = nullptr;
    if (p_Context != nullptr && p_Context->format == SelfFormat::Self)
    {
        auto s_Ret = _sceSblAuthMgrGetSelfInfo(p_Context, &s_Info);  
        if (s_Ret)
            s_Ret = 0;
        
        WriteLog(LL_Debug, "ptype: (%d)", s_Info->ptype);
        return (int32_t)s_Info->ptype == SelfPtypeFake;
    }
    else
        return false;
}

int FakeSelfManager::SceSblAuthMgrGetElfHeader(SelfContext* p_Context, Elf64_Ehdr** p_OutElfHeader)
{
    if (p_Context == nullptr)
        return -EAGAIN;
    
    if (p_Context->format == SelfFormat::Elf)
    {
        WriteLog(LL_Debug, "elf format");
        auto s_ElfHeader = reinterpret_cast<Elf64_Ehdr*>(p_Context->header);
        if (s_ElfHeader != nullptr)
            *p_OutElfHeader = s_ElfHeader;
        
        return 0;
    }
    else if (p_Context->format == SelfFormat::Self)
    {
        WriteLog(LL_Debug, "self format");
        auto s_SelfHeader = reinterpret_cast<SelfHeader*>(p_Context->header);
        size_t s_PdataSize = s_SelfHeader->headerSize - sizeof(SelfEntry) * s_SelfHeader->numEntries - sizeof(SelfHeader);
        if (s_PdataSize >= sizeof(Elf64_Ehdr) && (s_PdataSize & 0xF) == 0)
        {
            auto s_ElfHeader = reinterpret_cast<Elf64_Ehdr*>((uint8_t*)s_SelfHeader + sizeof(SelfHeader) + sizeof(SelfEntry) * s_SelfHeader->numEntries);
            if (s_ElfHeader)
                *p_OutElfHeader = s_ElfHeader;
            
            return 0;
        }

        WriteLog(LL_Error, "-EALREADY");
        return -EALREADY;
    }

    WriteLog(LL_Error, "-EAGAIN");
    return -EAGAIN;
}

int FakeSelfManager::BuildFakeSelfAuthInfo(SelfContext* p_Context, SelfAuthInfo* p_ParentAuthInfo, SelfAuthInfo* p_AuthInfo)
{
    auto _sceSblAuthMgrGetSelfInfo = (int (*)(SelfContext* ctx, void *exInfo))kdlsym(_sceSblAuthMgrGetSelfInfo);

    if (p_Context == nullptr || p_ParentAuthInfo == nullptr || p_AuthInfo == nullptr)
    {
        WriteLog(LL_Error, "invalid context (%p) || parentAuthInfo (%p) || authInfo (%p)", p_Context, p_ParentAuthInfo, p_AuthInfo);
        return -EINVAL;
    }
    
    if (!IsFakeSelf(p_Context))
    {
        WriteLog(LL_Error, "not fake self");
        return -EINVAL;
    }

    SelfExInfo* s_ExInfo = nullptr;
    int32_t s_Result = _sceSblAuthMgrGetSelfInfo(p_Context, &s_ExInfo);
    if (s_Result)
    {
        WriteLog(LL_Error, "could not get self info (%d).", s_Result);
        return s_Result;
    }

    Elf64_Ehdr* s_ElfHeader = nullptr;
    s_Result = SceSblAuthMgrGetElfHeader(p_Context, &s_ElfHeader);
    if (s_Result)
    {
        WriteLog(LL_Error, "could not get elf header (%d).", s_Result);
        return s_Result;
    }

    if (s_ElfHeader == nullptr)
    {
        WriteLog(LL_Error, "elf header invalid");
        return -ESRCH;
    }
    
    SelfAuthInfo s_Info = { 0 };
    s_Result = SceSblAuthMgrGetSelfAuthInfoFake(p_Context, &s_Info);
    if (s_Result)
    {
        switch (s_ElfHeader->e_type)
        {
        case ET_EXEC:
        case ET_SCE_EXEC:
        case ET_SCE_EXEC_ASLR:
            memcpy(&s_Info, FakeSelfManager::c_ExecAuthInfo, sizeof(s_Info));
            s_Result = 0;
            break;
        case ET_SCE_DYNAMIC:
            memcpy(&s_Info, FakeSelfManager::c_DynlibAuthInfo, sizeof(s_Info));
            s_Result = 0;
            break;
        default:
            s_Result = ENOTSUP;
            return s_Result;
        }

        s_Info.paid = s_ExInfo->paid;
    }

    if (p_AuthInfo)
        memcpy(p_AuthInfo, &s_Info, sizeof(*p_AuthInfo));

    return s_Result;
}

int FakeSelfManager::SceSblAuthMgrGetSelfAuthInfoFake(SelfContext* p_Context, SelfAuthInfo* p_Info)
{
    if (p_Context == nullptr)
    {
        WriteLog(LL_Error, "invalid context");
        return -EAGAIN;
    }
    
    if (p_Context->format == SelfFormat::Elf)
    {
        WriteLog(LL_Error, "invalid format");
        return -EAGAIN;
    }
    
    SelfHeader* s_Header = reinterpret_cast<SelfHeader*>(p_Context->header);
    SelfFakeAuthInfo* s_FakeInfo = reinterpret_cast<SelfFakeAuthInfo*>(p_Context->header + s_Header->headerSize + s_Header->metaSize - 0x100);

    if (s_FakeInfo->size == sizeof(s_FakeInfo->info))
    {
        memcpy(p_Info, &s_FakeInfo->info, sizeof(*p_Info));
        return 0;
    }

    WriteLog(LL_Error, "ealready");
    return -EALREADY;
}

int FakeSelfManager::SceSblAuthMgrSmLoadSelfBlock_Mailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto s_RequestMessage = static_cast<MailboxMessage*>(p_Request);
    if (s_RequestMessage == nullptr)
    {
        WriteLog(LL_Error, "invalid request message");
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }
    
    // Disgusting hack, we hook the caller of this, save the context, then continue as normal
    // Then we pick up the context later
    SelfContext* s_Context = FakeSelfManager::m_LastContext;
    // Check our context
    if (s_Context == nullptr)
    {
        WriteLog(LL_Error, "could not load self BLOCK, could not get the self context");
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    bool s_IsUnsigned = s_Context->format == SelfFormat::Elf;
    if (!s_IsUnsigned)
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);

    WriteLog(LL_Error, "unsigned/fake (s)elf detected");

    // TODO: Remove after testing
    auto s_Test1 = *(uint32_t*)&s_RequestMessage->unk08;
    auto s_Test2 = s_RequestMessage->unk08;
    if (s_Test1 != s_Test2)
        WriteLog(LL_Error, "test1 != test2");
    
    // Do these ever change?
    vm_offset_t s_SegmentDataGpuVa = *(uint32_t*)&s_RequestMessage->unk08;
    vm_offset_t s_CurrentDataGpuVa = *(uint32_t*)(((uint8_t*)p_Response) + 0x50);
    vm_offset_t s_CurrentData2GpuVa = *(uint32_t*)(((uint8_t*)p_Response) + 0x58);
    
    uint32_t s_DataOffset = *(uint32_t*)(((uint8_t*)p_Response) + 0x44);
    uint32_t s_DataSize = *(uint32_t*)(((uint8_t*)p_Response) + 0x48);

    vm_offset_t s_SegmentDataCpuVa = SceSblDriverGpuVaToCpuVa(s_SegmentDataGpuVa, nullptr);
    vm_offset_t s_CurrentDataCpuVa = SceSblDriverGpuVaToCpuVa(s_CurrentDataGpuVa, nullptr);
    vm_offset_t s_CurrentData2CpuVa = s_CurrentData2GpuVa ? SceSblDriverGpuVaToCpuVa(s_CurrentData2GpuVa, nullptr) : 0;

    if (s_SegmentDataCpuVa && s_CurrentDataCpuVa)
    {
        if (s_CurrentData2GpuVa && s_CurrentData2GpuVa != s_CurrentDataGpuVa && s_DataOffset > 0)
        {
            // xxx: data spans two consecutive memory's pages, so we need to copy twice
            uint32_t s_Size1 = PAGE_SIZE - s_DataOffset;
            memcpy((uint8_t*)s_SegmentDataCpuVa, (const uint8_t*)s_CurrentDataCpuVa, s_Size1);

            // PATCH: Prevent *potential* kpanic here
            if (s_CurrentData2CpuVa != 0)
                memcpy((uint8_t*)s_SegmentDataCpuVa + s_Size1, (const uint8_t*)s_CurrentData2CpuVa, s_DataSize - s_Size1);
        }
        else
            memcpy((uint8_t*)s_SegmentDataCpuVa, (const uint8_t*)s_CurrentDataCpuVa + s_DataOffset, s_DataSize);
    }

    s_RequestMessage->retVal = 0;
    return 0;
}

int FakeSelfManager::SceSblAuthMgrSmLoadSelfSegment_Mailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto s_RequestMessage = static_cast<MailboxMessage*>(p_Request);
    if (s_RequestMessage == nullptr)
    {
        WriteLog(LL_Error, "invalid response");
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }
    
    SelfContext* s_Context = FakeSelfManager::m_LastContext;
    if (s_Context == nullptr)
    {
        WriteLog(LL_Error, "could not load segment, could not get self context.");
        return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    bool s_IsUnsigned = s_Context && IsFakeSelf(s_Context);
    if (s_IsUnsigned)
    {
        WriteLog(LL_Debug, "unsigned/fake (s)elf detected clearing ret val");
        s_RequestMessage->retVal = 0;
        return 0;
    }
    
    return SceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
}

bool FakeSelfManager::OnLoad()
{
    // Clear out any stale contexts
    m_LastContext = nullptr;

    if (m_SceSblAuthMgrIsLoadable2Hook != nullptr)
        (void)m_SceSblAuthMgrIsLoadable2Hook->Enable();
    
    if (m_SceSblAuthMgrVerifyHeaderHook != nullptr)
        (void)m_SceSblAuthMgrVerifyHeaderHook->Enable();
    
    if (m_SceSblServiceMailboxHook != nullptr)
        (void)m_SceSblServiceMailboxHook->Enable();

    if (m__SceSblAuthMgrSmLoadSelfBlockHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfBlockHook->Enable();
    
    if (m__SceSblAuthMgrSmLoadSelfSegmentHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfSegmentHook->Enable();
    
    WriteLog(LL_Debug, "FakeSelfManager loaded...");
    return true;
}

bool FakeSelfManager::OnUnload()
{
    if (m_SceSblAuthMgrIsLoadable2Hook != nullptr)
        (void)m_SceSblAuthMgrIsLoadable2Hook->Disable();
    
    if (m_SceSblAuthMgrVerifyHeaderHook != nullptr)
        (void)m_SceSblAuthMgrVerifyHeaderHook->Disable();
    
    if (m_SceSblServiceMailboxHook != nullptr)
        (void)m_SceSblServiceMailboxHook->Disable();

    if (m__SceSblAuthMgrSmLoadSelfBlockHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfBlockHook->Disable();
    
    if (m__SceSblAuthMgrSmLoadSelfSegmentHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfSegmentHook->Disable();
    
    // Clear out any stale contexts
    m_LastContext = nullptr;
    
    WriteLog(LL_Debug, "FakeSelfManager unloaded...");
    return true;
}

bool FakeSelfManager::OnSuspend()
{
    // Don't touch the hooks here, leave them in-tact
    return true;
}

bool FakeSelfManager::OnResume()
{
    // Don't touch the hooks here, leave them in-tact
    return true;
}

int FakeSelfManager::On_SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg)
{
    if (FakeSelfManager::m_LastContext != p_Context)
    {
        WriteLog(LL_Debug, "%p -> %p", FakeSelfManager::m_LastContext, p_Context);
        FakeSelfManager::m_LastContext = p_Context;
    }   

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400
    return _SceSblAuthMgrSmLoadSelfSegment(p_Context, p_SegmentIndex, p_IsBlockTable, p_Data, p_Size, p_ReadCallback, p_CallbackArg);
#else
// We have to check the prototype on every version, 1.76 and 1.00 appear to be different (having 1 extra argument)
// 4.05-6.50 seem to all be the same
// 4.05 - 006167B0
// 1.76 - 005C9BB0
// 5.05 - 00642EF0
#error "_SceSblAuthMgrSmLoadSelfSegment prototype not checked"
return -EIO;
#endif 
}

int FakeSelfManager::_SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        return -1;
    }

    auto s_Manager = static_cast<FakeSelfManager*>(s_PluginManager->GetFakeSelfManager());
    if (s_Manager == nullptr)
    {
        WriteLog(LL_Error, "could not get fake self manager instance");
        return -1;
    }

    auto s_Hook = s_Manager->m__SceSblAuthMgrSmLoadSelfSegmentHook;
    if (s_Hook == nullptr)
    {
        WriteLog(LL_Error, "could not find the _sceSblAuthMgrSmLoadSelfSegmentHook hook");
        return -1;
    }
    if (s_Hook->GetOriginalFunctionAddress() == nullptr)
        return -1;

    int32_t s_Ret = -1;

    if (!s_Hook->Disable())
        return -1;
    
    auto s_Call = (int(*)(SelfContext *p_Context, uint32_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg))s_Hook->GetOriginalFunctionAddress();
    
    // Call the original
    s_Ret = s_Call(p_Context, p_SegmentIndex, p_IsBlockTable, p_Data, p_Size, p_ReadCallback, p_CallbackArg);

    (void)s_Hook->Enable();

    return s_Ret;
}

int FakeSelfManager::On_SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, uint32_t p_SegmentIndex, uint32_t p_BlockIndex, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg)
{
    if (FakeSelfManager::m_LastContext != p_Context)
    {
        WriteLog(LL_Debug, "%p -> %p", FakeSelfManager::m_LastContext, p_Context);
        FakeSelfManager::m_LastContext = p_Context;
    }    

    return _SceSblAuthMgrSmLoadSelfBlock(p_Context, p_SegmentIndex, p_BlockIndex, p_Data, p_Size, p_ReadCallback, p_CallbackArg);
}

int FakeSelfManager::_SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, uint32_t p_SegmentIndex, uint32_t p_BlockIndex, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg)
{
    auto s_PluginManager = Mira::Framework::GetFramework()->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager");
        return -1;
    }

    auto s_Manager = static_cast<FakeSelfManager*>(s_PluginManager->GetFakeSelfManager());
    if (s_Manager == nullptr)
    {
        WriteLog(LL_Error, "could not get fake self manager instance");
        return -1;
    }

    auto s_Hook = s_Manager->m__SceSblAuthMgrSmLoadSelfBlockHook;
    if (s_Hook == nullptr)
    {
        WriteLog(LL_Error, "could not find the _sceSblAuthMgrSmLoadSelfBlockHook hook");
        return -1;
    }
    if (s_Hook->GetOriginalFunctionAddress() == nullptr)
        return -1;

    int32_t s_Ret = -1;

    if (!s_Hook->Disable())
        return -1;
    
    auto s_Call = (int(*)(SelfContext* p_Context, uint32_t p_SegmentIndex, uint32_t p_BlockIndex, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg))s_Hook->GetOriginalFunctionAddress();
    
    // Call the original
    s_Ret = s_Call(p_Context, p_SegmentIndex, p_BlockIndex, p_Data, p_Size, p_ReadCallback, p_CallbackArg);

    (void)s_Hook->Enable();

    return s_Ret;
}