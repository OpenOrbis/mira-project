#include "SelfDecrypt.hpp"

#include <Utils/SysWrappers.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/fcntl.h>
    #include <sys/unistd.h>
};

#define ALIGN(size, alignment) \
    (((size) + ((alignment) - 1)) & ~((alignment) - 1))

#define ALIGN_PAGE(size) \
    ALIGN(size, PAGE_SIZE)

#define sceSblServiceMailbox_locked(ret, id, iptr, optr) do { \
    _sx_xlock(s_sm_sxlock, 0); \
    ret = sceSblServiceMailbox((id), (iptr), (optr)); \
    _sx_xunlock(s_sm_sxlock); \
} while (0)

using namespace Mira::OrbisOS;

SelfDecrypt::SelfDecrypt(const char* p_FilePath) :
    m_Self { 0 }
{
    // Before we do anything assign a invalid fd
    m_Self.fd = -1;
    m_Self.svc_id = -1;

    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
    if (s_ThreadManager == nullptr)
    {
        WriteLog(LL_Error, "could not get thread manager");
        return;
    }

    auto s_FileThread = s_ThreadManager->GetFileIoThread();
    if (s_FileThread == nullptr)
    {
        WriteLog(LL_Error,"could not get the file io thread");
        return;
    }

    // Open the descriptor
    auto s_Descriptor = kopen_t(p_FilePath, O_RDONLY, 0, s_FileThread);
    if (s_Descriptor < 0)
    {
        WriteLog(LL_Error, "could not open (%s): (%d).", p_FilePath, s_Descriptor);
        return;
    }

    // Seek to the end of the file to get the size
    auto s_FileSize = klseek_t(s_Descriptor, 0, SEEK_END, s_FileThread);
    if (s_FileSize < 0)
    {
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not seek (%d) (%lld).", s_Descriptor, s_FileSize);
        return;
    }

    // Reset back to the beginning
    if (klseek_t(s_Descriptor, 0, SEEK_SET, s_FileThread) < 0)
    {
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not seek back to start");
        return;
    }

    // Read out the self header
    auto s_BytesRead = kread_t(s_Descriptor, &m_Self.header, sizeof(m_Self.header), s_FileThread);
    if (s_BytesRead != sizeof(m_Self.header))
    {
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "did not read the correct amount of data (%lld) != (%lld).", s_BytesRead, sizeof(m_Self.header));
        return;
    }

    // Validate the self magic
    if (m_Self.header.magic != SELF_MAGIC)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid self magic");
        return;
    }

    // Validate self version
    if (m_Self.header.version != SELF_VERSION)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid self version");
        return;
    }

    if (m_Self.header.mode != SELF_MODE)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid self mode");
        return;
    }

    if (m_Self.header.endian != SELF_ENDIANNESS)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid self endianness");
        return;
    }

    if (m_Self.header.file_size != s_FileSize)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid self file size");
        return;
    }

    // Allocate new self entries
    auto s_EntriesSize = m_Self.header.num_entries * sizeof(self_entry_t);
    auto s_Entries = new self_entry_t[m_Self.header.num_entries];
    if (s_Entries == nullptr)
    {
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not allocate self entries cnt: (%d).", m_Self.header.num_entries);
        return;
    }
    memset(s_Entries, 0, s_EntriesSize);

    // Read out all of the self entries
    s_BytesRead = kread_t(s_Descriptor, s_Entries, s_EntriesSize, s_FileThread);
    if (s_BytesRead != s_EntriesSize)
    {
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "bytes read (%lld) != size (%lld).", s_BytesRead, s_EntriesSize);
        return;
    }

    // Copy the contents
    auto s_Contents = new uint8_t[s_FileSize];
    if (s_Contents == nullptr)
    {
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not allocate file size (%llx).", s_FileSize);
        return;
    }

    if (klseek_t(s_Descriptor, 0, SEEK_SET, s_FileThread) < 0)
    {
        delete [] s_Contents;
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not allocate file size (%llx).", s_FileSize);
        return;
    }

    s_BytesRead = kread_t(s_Descriptor, s_Contents, s_FileSize, s_FileThread);
    if (s_BytesRead != s_FileSize)
    {
        delete[] s_Contents;
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "bytes read (%lld) != size (%lld).", s_BytesRead, s_FileSize);
        return;
    }

    auto s_FileNameLength = strlen(p_FilePath);
    if (s_FileNameLength <= 0 || s_FileNameLength > 260)
    {
        delete [] s_Contents;
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "invalid file path size (%d).", s_FileNameLength);
        return;
    }

    char* s_FileName = new char[s_FileNameLength];
    if (s_FileName == nullptr)
    {
        delete [] s_Contents;
        delete [] s_Entries;
        memset(&m_Self.header, 0, sizeof(m_Self.header));
        kclose_t(s_Descriptor, s_FileThread);
        WriteLog(LL_Error, "could not allocate file path size (%d).", s_FileNameLength);
        return;
    }
    memset(s_FileName, 0, s_FileNameLength);
    memcpy(s_FileName, p_FilePath, s_FileNameLength);

    // Update all of the self information
    m_Self.fd = s_Descriptor;
    m_Self.file_size = s_FileSize;
    m_Self.entries_size = s_EntriesSize;
    m_Self.entries = s_Entries;
    m_Self.data = s_Contents;
    m_Self.file_path = s_FileName;
    m_Self.ctx_id = -1;
}

bool SelfDecrypt::VerifyHeader()
{
    if (m_Self.fd <= 0)
        return false;

    auto ctxStatus = static_cast<uint32_t*>(kdlsym(ctxStatus));
    auto _sceSblAuthMgrSmFinalize = (int(*)(void* ctx))kdlsym(_sceSblAuthMgrSmFinalize);
    auto ctxTable = (struct self_context_t*)kdlsym(ctxTable);
    auto sceSblDriverMapPages = (int(*)(uint64_t *gpu_paddr, void *cpu_vaddr, uint32_t npages, uint64_t flags, uint64_t unk, uint64_t *gpu_desc))kdlsym(sceSblDriverMapPages);
    auto sceSblDriverUnmapPages = (int(*)(uint64_t gpu_desc))kdlsym(sceSblDriverUnmapPages);
    auto sceSblServiceMailbox = (int(*)(uint32_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);
    auto s_sm_sxlock = (struct sx*)kdlsym(s_sm_sxlock);
    auto _sx_xlock = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto _sx_xunlock = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);

    int s_ContextId = -1;

    if (m_Self.ctx_id == -1)
    {
        m_Self.ctx_id = 0;
        while (ctxStatus[s_ContextId] != 3)
            s_ContextId = (s_ContextId + 1) % SELF_MAX_CONTEXTS;
        
        WriteLog(LL_Debug, "found available context id: (%d).", s_ContextId);
        ctxStatus[s_ContextId] = 1;
        m_Self.ctx_id = s_ContextId;
    }

    if (m_Self.svc_id == -1)
        m_Self.svc_id = *static_cast<int*>(kdlsym(s_taskId));
    
    if (m_Self.ctx_id < 0 || m_Self.ctx_id > 3)
    {
        ReleaseContext();

        WriteLog(LL_Error, "invalid self context id (%d).", m_Self.ctx_id);
        return false;
    }

    m_Self.ctx = &ctxTable[m_Self.ctx_id];
    _sceSblAuthMgrSmFinalize(m_Self.ctx);

    size_t s_HeaderDataSize = ALIGN_PAGE(m_Self.header.header_size + m_Self.header.meta_size);
    uint8_t* s_HeaderData = new uint8_t[s_HeaderDataSize];
    if (s_HeaderData == nullptr)
    {
        ReleaseContext();

        WriteLog(LL_Error, "could not allocate header data (%lld).", s_HeaderDataSize);
        return false;
    }

    memset(s_HeaderData, 0, s_HeaderDataSize);
    memcpy(s_HeaderData, m_Self.data, MIN(m_Self.file_size, s_HeaderDataSize));

    uint64_t s_HeaderDataMapped = 0;
    uint64_t s_HeaderDataMapDesc = 0;
    auto s_Ret = sceSblDriverMapPages(&s_HeaderDataMapped, s_HeaderData, 1, 0x61, 0, &s_HeaderDataMapDesc);
    if (s_Ret != 0)
    {
        delete [] s_HeaderData;

        ReleaseContext();

        WriteLog(LL_Error, "could not map driver pages (%d).", s_Ret);
        return false;
    }

    size_t s_AuthInfoSize = ALIGN_PAGE(SELF_AUTH_INFO_SIZE);
    auto s_AuthInfo = new uint8_t[s_AuthInfoSize];
    if (s_AuthInfo == nullptr)
    {
        s_Ret = sceSblDriverUnmapPages(s_HeaderDataMapDesc);
        if (s_Ret != 0)
            WriteLog(LL_Error, "could not unmap header driver pages (%d).", s_Ret);
        
        delete [] s_HeaderData;

        ReleaseContext();

        WriteLog(LL_Error, "could not allocate auth info");
        return false;
    }
    memset(s_AuthInfo, 0, s_AuthInfoSize);

    uint64_t s_AuthInfoMapped = 0;
    uint64_t s_AuthInfoMapDesc = 0;
    s_Ret = sceSblDriverMapPages(&s_AuthInfoMapped, s_AuthInfo, 1, 0x61, 0, &s_AuthInfoMapDesc);
    if (s_Ret != 0)
    {
        s_Ret = sceSblDriverUnmapPages(s_AuthInfoMapDesc);
        if (s_Ret != 0)
            WriteLog(LL_Error, "could not unmap auth driver pages (%d).", s_Ret);
        s_Ret = sceSblDriverUnmapPages(s_HeaderDataMapDesc);
        if (s_Ret != 0)
            WriteLog(LL_Error, "could not unmap header driver pages (%d).", s_Ret);
        
        delete [] s_AuthInfo;
        delete [] s_HeaderData;

        ReleaseContext();
        
        WriteLog(LL_Error, "could not map auth info driver pages (%d).", s_Ret);
        return false;
    }

    // Send command
    uint8_t s_Payload[0x80] = { 0 };
    auto s_Args = reinterpret_cast<sbl_authmgr_verify_header_t*>(&s_Payload[0]);
    memset(s_Payload, 0, sizeof(s_Payload));

    s_Args->function = AUTHMGR_CMD_VERIFY_HEADER;
    s_Args->status = 0;
    s_Args->header_addr = s_HeaderDataMapped;
    s_Args->header_size = m_Self.header.header_size + m_Self.header.meta_size;
    s_Args->context_id = m_Self.ctx_id;
    s_Args->auth_info_addr = s_AuthInfoMapped;
    s_Args->key_id = 0;
    memset(&s_Args->key, 0, SELF_KEY_SIZE);

    WriteLog(LL_Info, "Sending AUTHMGR_CMD_VERIFY_HEADER...");
    sceSblServiceMailbox_locked(s_Ret, m_Self.svc_id, &s_Payload, &s_Payload);
    if (s_Ret != 0 || s_Args->status != 0)
    {
        s_Ret = sceSblDriverUnmapPages(s_AuthInfoMapDesc);
        if (s_Ret != 0)
            WriteLog(LL_Error, "could not unmap auth driver pages (%d).", s_Ret);
        s_Ret = sceSblDriverUnmapPages(s_HeaderDataMapDesc);
        if (s_Ret != 0)
            WriteLog(LL_Error, "could not unmap header driver pages (%d).", s_Ret);

        delete [] s_AuthInfo;
        delete [] s_HeaderData;

        ReleaseContext();

        WriteLog(LL_Error, "sceSblServiceMailbox returned (%d) status: (%d) func: (%d)", s_Ret, s_Args->status, s_Args->function);
        return false;
    }

    WriteLog(LL_Debug, "confirmed context id: (%d).", m_Self.ctx_id);
    m_Self.auth_ctx_id = s_Args->context_id;
    memcpy(&m_Self.auth_info, s_AuthInfo, sizeof(m_Self.auth_info));
    m_Self.verified = true;

    if (s_AuthInfoMapped != 0)
    {
        s_Ret = sceSblDriverUnmapPages(s_AuthInfoMapDesc);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not unmap driver pages (%d).", s_Ret);
            return false;
        }
    }

    if (s_HeaderDataMapped != 0)
    {
        s_Ret = sceSblDriverUnmapPages(s_HeaderDataMapDesc);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not unmap header data driver pages (%d).", s_Ret);
            return false;
        }
    }

    return true;
}

bool SelfDecrypt::ReleaseContext()
{
    auto ctxStatus = static_cast<uint32_t*>(kdlsym(ctxStatus));
    int32_t s_ContextId = m_Self.ctx_id;
    if (0 <= s_ContextId && s_ContextId <= 3)
    {
        ctxStatus[s_ContextId] = 3;
        m_Self.ctx_id = -1;
    }

    return true;
}