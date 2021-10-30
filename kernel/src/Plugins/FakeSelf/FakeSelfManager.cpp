// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//-V::668

/*
    Implemented from: https://github.com/xvortex/ps4-hen-vtx
    Ported by: kiwidog (@kd_tech_)

    Bugfixes: SiSTRo (https://github.com/SiSTR0), SocraticBliss (https://github.com/SocraticBliss)
*/

#include "FakeSelfManager.hpp"
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <Mira.hpp>
#include <Boot/Config.hpp>
#include <Plugins/PluginManager.hpp>

#include <OrbisOS/Utilities.hpp>

extern "C"
{
    #include <sys/sysent.h>
};

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

//SelfContext* FakeSelfManager::m_LastContext = nullptr;

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
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;

	uint8_t* s_TrampolineA = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_GET_PID].sy_call);
    uint8_t* s_TrampolineB = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_GET_PROC].sy_call);
    uint8_t* s_TrampolineC = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_SET_PROC].sy_call);
    uint8_t* s_TrampolineD = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_GET_FILE].sy_call);
    uint8_t* s_TrampolineE = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_GET_FD].sy_call);

    Utilities::HookFunctionCall(s_TrampolineA, reinterpret_cast<void*>(OnSceSblAuthMgrVerifyHeader), kdlsym(sceSblAuthMgrVerifyHeader_hookA));
    Utilities::HookFunctionCall(s_TrampolineB, reinterpret_cast<void*>(OnSceSblAuthMgrVerifyHeader), kdlsym(sceSblAuthMgrVerifyHeader_hookB));
    Utilities::HookFunctionCall(s_TrampolineC, reinterpret_cast<void*>(OnSceSblAuthMgrIsLoadable2), kdlsym(sceSblAuthMgrIsLoadable2_hook));
    Utilities::HookFunctionCall(s_TrampolineD, reinterpret_cast<void*>(SceSblAuthMgrSmLoadSelfSegment_Mailbox), kdlsym(sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook));
    Utilities::HookFunctionCall(s_TrampolineE, reinterpret_cast<void*>(SceSblAuthMgrSmLoadSelfBlock_Mailbox), kdlsym(sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook));
    //HookFunctionCall(s_TrampolineF, reinterpret_cast<void*>(SceSblAuthMgrIsLoadable_sceSblACMgrGetPathId), kdlsym(sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook));

    //m_SceSblServiceMailboxHook = new Utils::Hook(kdlsym(sceSblServiceMailbox), reinterpret_cast<void*>(OnSceSblServiceMailbox));
    //m_SceSblAuthMgrVerifyHeaderHook = new Utils::Hook(kdlsym(sceSblAuthMgrVerifyHeader), reinterpret_cast<void*>(OnSceSblAuthMgrVerifyHeader));
    //m_SceSblAuthMgrIsLoadable2Hook = new Utils::Hook(kdlsym(sceSblAuthMgrIsLoadable2), reinterpret_cast<void*>(OnSceSblAuthMgrIsLoadable2));
    //m_SceSblACMgrGetPathIdHook = new Utils::Hook(kdlsym(sceSblACMgrGetPathId), OnSceSblACMgrGetPathId);

    //m__SceSblAuthMgrSmLoadSelfBlockHook = new Utils::Hook(kdlsym(_sceSblAuthMgrSmLoadSelfBlock), reinterpret_cast<void*>(On_SceSblAuthMgrSmLoadSelfBlock));
    //m__SceSblAuthMgrSmLoadSelfSegmentHook = new Utils::Hook(kdlsym(_sceSblAuthMgrSmLoadSelfSegment), reinterpret_cast<void*>(On_SceSblAuthMgrSmLoadSelfSegment));
}

FakeSelfManager::~FakeSelfManager()
{
    /*if (m_SceSblServiceMailboxHook != nullptr)
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
    }*/
}

int FakeSelfManager::OnSceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo)
{
    auto sceSblAuthMgrIsLoadable2 = (int(*)(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo))kdlsym(sceSblAuthMgrIsLoadable2);

    if (p_Context == nullptr)
    {
        WriteLog(LL_Error, "invalid context");
        return sceSblAuthMgrIsLoadable2(p_Context, p_OldAuthInfo, p_PathId, p_NewAuthInfo);
    } 
    
    if (p_Context->format == SelfFormat::Elf || IsFakeSelf(p_Context))
    {
        WriteLog(LL_Debug, "building fake self information");
        return BuildFakeSelfAuthInfo(p_Context, p_OldAuthInfo, p_NewAuthInfo);
    }        
    else
        return sceSblAuthMgrIsLoadable2(p_Context, p_OldAuthInfo, p_PathId, p_NewAuthInfo);
}

/*int FakeSelfManager::OnSceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto sceSblServiceMailbox = (int(*)(uint32_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);

    auto s_Request = static_cast<MailboxMessage*>(p_Request);
    if (s_Request == nullptr)
    {
        WriteLog(LL_Error, "invalid request");
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    // Only hook on the needed service id
    if (p_ServiceId != 0)
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    
    switch (s_Request->funcId)
    {
    case LoadSelfSegment:
        return SceSblAuthMgrSmLoadSelfSegment_Mailbox(p_ServiceId, p_Request, p_Response);
    case LoadSelfBlock:
        return SceSblAuthMgrSmLoadSelfBlock_Mailbox(p_ServiceId, p_Request, p_Response);
    default:
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }
}*/

int FakeSelfManager::AuthSelfHeader(SelfContext* p_Context)
{    
    auto sceSblAuthMgrVerifyHeader = (int(*)(SelfContext* p_Context))kdlsym(sceSblAuthMgrVerifyHeader);

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
        auto s_Result = sceSblAuthMgrVerifyHeader(p_Context);

        // Restore everything
        memcpy(p_Context->header, s_Temp, s_NewTotalHeaderSize);
        p_Context->format = s_OldFormat;
        p_Context->totalHeaderSize = s_OldTotalHeaderSize;

        delete [] s_Temp;

        return s_Result;
    }
    else
        return sceSblAuthMgrVerifyHeader(p_Context);
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

    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto s_SblDrvMsgMtx = (struct mtx*)kdlsym(sbl_drv_msg_mtx);

    SblMapListEntry* s_Entry = *(SblMapListEntry**)kdlsym(gpu_va_page_list);
    SblMapListEntry* s_FinalEntry = nullptr;

    // Lock before we iterate this list, because other paths can absolutely use it concurrently
    _mtx_lock_flags(s_SblDrvMsgMtx, 0, __FILE__, __LINE__);

    while (s_Entry)
    {
        if (s_Entry->gpuVa == p_GpuVa)
        {
            s_FinalEntry = s_Entry;
            break;
        }

        s_Entry = s_Entry->next;
    }

    _mtx_unlock_flags(s_SblDrvMsgMtx, 0, __FILE__, __LINE__);
    return s_FinalEntry;
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
    if (p_Context->format == SelfFormat::Self)
    {
        if (_sceSblAuthMgrGetSelfInfo(p_Context, &s_Info))
            return false;
        
        return s_Info->ptype == SelfPtypeFake;
    }

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
            auto s_ElfHeader = reinterpret_cast<Elf64_Ehdr*>(((uint8_t*)s_SelfHeader + sizeof(SelfHeader)) + (sizeof(SelfEntry) * s_SelfHeader->numEntries));
            if (s_ElfHeader)
                *p_OutElfHeader = s_ElfHeader;
            
            return 0;
        }

        return -EALREADY;
    }

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
    
    SelfAuthInfo s_FakeAuthInfo = { 0 };
    s_Result = SceSblAuthMgrGetSelfAuthInfoFake(p_Context, &s_FakeAuthInfo);
    if (s_Result)
    {
        switch (s_ElfHeader->e_type)
        {
        case ET_EXEC:
        case ET_SCE_EXEC:
        case ET_SCE_EXEC_ASLR:
            memcpy(&s_FakeAuthInfo, FakeSelfManager::c_ExecAuthInfo, sizeof(s_FakeAuthInfo));
            s_Result = 0;
            break;
        case ET_SCE_DYNAMIC:
            memcpy(&s_FakeAuthInfo, FakeSelfManager::c_DynlibAuthInfo, sizeof(s_FakeAuthInfo));
            s_Result = 0;
            break;
        default:
            s_Result = ENOTSUP;
            return s_Result;
        }

        s_FakeAuthInfo.paid = s_ExInfo->paid;
    }

    // p_AuthInfo is checked already
    memcpy(p_AuthInfo, &s_FakeAuthInfo, sizeof(*p_AuthInfo));

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
    
    SelfHeader* s_Header = p_Context->header;
    auto s_Data = reinterpret_cast<const char*>(p_Context->header);
    auto s_FakeInfo = reinterpret_cast<const SelfFakeAuthInfo*>(s_Data + s_Header->headerSize + s_Header->metaSize - 0x100);
    if (s_FakeInfo->size == sizeof(s_FakeInfo->info))
    {
        memcpy(p_Info, &s_FakeInfo->info, sizeof(*p_Info));
        return 0;
    }

    WriteLog(LL_Error, "ealready (no valid authinfo)");
    return -EALREADY;
}

int FakeSelfManager::SceSblAuthMgrSmLoadSelfSegment_Mailbox(uint64_t p_ServiceId, void* p_Request, void* p_Response)
{
    auto sceSblServiceMailbox = (int(*)(uint32_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wframe-address"
	// self_context is first param of caller. 0x08 = sizeof(struct self_context*)
	uint8_t* frame = (uint8_t*)__builtin_frame_address(1);
#pragma clang diagnostic pop

	SelfContext* s_Context = *(SelfContext**)(frame - 0x08);

    auto s_RequestMessage = static_cast<MailboxMessage*>(p_Request);
    if (s_RequestMessage == nullptr)
    {
        WriteLog(LL_Error, "invalid response");
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    if (s_Context == nullptr)
    {
        WriteLog(LL_Error, "could not load segment, could not get self context.");
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    }

    bool s_IsUnsigned = IsFakeSelf(s_Context);
    if (s_IsUnsigned)
    {
        WriteLog(LL_Debug, "unsigned/fake (s)elf detected clearing ret val");
        s_RequestMessage->retVal = 0;
        return 0;
    }
    
    return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
}

int FakeSelfManager::SceSblAuthMgrSmLoadSelfBlock_Mailbox(uint64_t p_ServiceId, uint8_t* p_Request, void* p_Response)
{
    auto sceSblServiceMailbox = (int(*)(uint64_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);

    // self_context is first param of caller. 0x08 = sizeof(struct self_context*)
    uint8_t* frame = (uint8_t*)__builtin_frame_address(1);
    SelfContext* p_Context = *(SelfContext**)(frame - 0x08);

    bool s_IsUnsigned = p_Context && (p_Context->format == SelfFormat::Elf || IsFakeSelf(p_Context));

    if (!s_IsUnsigned)
        return sceSblServiceMailbox(p_ServiceId, p_Request, p_Response);
    else 
    {
        //WriteLog(LL_Debug, "unsigned/fake (s)elf detected");

        vm_offset_t s_SegmentDataGpuVa = *(uint64_t*)(p_Request + 0x08);
        vm_offset_t s_CurrentDataGpuVa = *(uint64_t*)(p_Request + 0x50);
        vm_offset_t s_CurrentData2GpuVa = *(uint64_t*)(p_Request + 0x58);

        uint32_t s_DataOffset = *(uint32_t*)(p_Request + 0x44);
        uint32_t s_DataSize = *(uint32_t*)(p_Request + 0x48);

        /* looking into lists of GPU's mapped memory regions */
        vm_offset_t s_SegmentDataCpuVa = SceSblDriverGpuVaToCpuVa(s_SegmentDataGpuVa, NULL);
        vm_offset_t s_CurrentDataCpuVa = SceSblDriverGpuVaToCpuVa(s_CurrentDataGpuVa, NULL);
        vm_offset_t s_CurrentData2CpuVa = s_CurrentData2GpuVa ? SceSblDriverGpuVaToCpuVa(s_CurrentData2GpuVa, NULL) : 0;

        if (s_SegmentDataCpuVa && s_CurrentDataCpuVa) 
        {
            if (s_CurrentData2GpuVa && s_CurrentData2GpuVa != s_CurrentDataGpuVa && s_DataOffset > 0) 
            {

                /* data spans two consecutive memory's pages, so we need to copy twice */
                uint32_t s_Size = PAGE_SIZE - s_DataOffset;
                memcpy((char*)s_SegmentDataCpuVa, (char*)s_CurrentDataCpuVa + s_DataOffset, s_Size);

                // prevent *potential* kpanic here
                if (s_CurrentData2CpuVa) 
                {
                    memcpy((char *) s_SegmentDataCpuVa + s_Size, (char *) s_CurrentData2CpuVa, s_DataSize - s_Size);
                }
            }
            else 
            {
                memcpy((char*)s_SegmentDataCpuVa, (char*)s_CurrentDataCpuVa + s_DataOffset, s_DataSize);
            }
        }

        /* setting error field to zero, thus we have no errors */
        *(int*)(p_Request + 0x04) = 0;

        return 0;
    }
}

int FakeSelfManager::SceSblAuthMgrIsLoadable_sceSblACMgrGetPathId(const char* path) {
    auto strstr = (char *(*)(const char *haystack, const char *needle) )kdlsym(strstr);
    auto sceSblACMgrGetPathId = (int(*)(const char* path))kdlsym(sceSblACMgrGetPathId);

    static const char* s_SelfDirPrefix = "/data/self/";
    const char* p;

    if (path) 
    {
        p = strstr(path, s_SelfDirPrefix);
        if (p)
            path = p + strlen(s_SelfDirPrefix);
    }

    return sceSblACMgrGetPathId(path);
}

bool FakeSelfManager::OnLoad()
{
    // Clear out any stale contexts
    //m_LastContext = nullptr;

    /*if (m_SceSblAuthMgrIsLoadable2Hook != nullptr)
        (void)m_SceSblAuthMgrIsLoadable2Hook->Enable();
    
    if (m_SceSblAuthMgrVerifyHeaderHook != nullptr)
        (void)m_SceSblAuthMgrVerifyHeaderHook->Enable();
    
    if (m_SceSblServiceMailboxHook != nullptr)
        (void)m_SceSblServiceMailboxHook->Enable();

    if (m__SceSblAuthMgrSmLoadSelfBlockHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfBlockHook->Enable();
    
    if (m__SceSblAuthMgrSmLoadSelfSegmentHook != nullptr)
        (void)m__SceSblAuthMgrSmLoadSelfSegmentHook->Enable();*/
    
    WriteLog(LL_Debug, "FakeSelfManager loaded...");
    return true;
}

bool FakeSelfManager::OnUnload()
{
    /*if (m_SceSblAuthMgrIsLoadable2Hook != nullptr)
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
    m_LastContext = nullptr;*/
    
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
