// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifdef _disable_me_self_decrypt
#include "SelfDecrypt.hpp"

#include <Utils/SysWrappers.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>

#include <Mira.hpp>
#include <OrbisOS/ThreadManager.hpp>

extern "C"
{
    #include <sys/fcntl.h>
    #include <sys/unistd.h>
};

#ifdef ALIGN
#undef ALIGN
#define ALIGN(size, alignment) \
    (((size) + ((alignment) - 1)) & ~((alignment) - 1))
#endif

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

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return;
    }

    auto s_ThreadManager = s_Framework->GetThreadManager();
    if (s_ThreadManager == nullptr)
    {
        WriteLog(LL_Error, "could not get thread manager");
        return;
    }

    auto s_FileThread = s_ThreadManager->GetIoThread();
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

void SelfDecrypt::Close()
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return;
    }
    auto s_ThreadManager = s_Framework->GetThreadManager();
    if (s_ThreadManager == nullptr)
    {
        WriteLog(LL_Error, "could not get thread manager");
        return;
    }

    auto s_IoThread = s_ThreadManager->GetIoThread();
    if (s_IoThread == nullptr)
    {
        WriteLog(LL_Error, "could not get the io thread");
        return;
    }

    struct blob_t* s_Blob = m_Self.blobs;
    struct blob_t* s_Next = nullptr;
    if (s_Blob == nullptr)
        return;
    
    while (s_Blob != nullptr)
    {
        s_Next = s_Blob->next;

        delete [] s_Blob->data;
        delete s_Blob;

        s_Blob = s_Next;
    }

    // Close the file
    if (m_Self.fd > -1)
    {
        kclose_t(m_Self.fd, s_IoThread);
        m_Self.fd = -1;
    }
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

bool SelfDecrypt::DecryptSegment(uint8_t* p_InputData, size_t p_InputDataLength, uint64_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_OutputData, uint64_t* p_InOutOutputSize)
{
    //auto sceSblDriverMapPages = (int(*)(uint64_t *gpu_paddr, void *cpu_vaddr, uint32_t npages, uint64_t flags, uint64_t unk, uint64_t *gpu_desc))kdlsym(sceSblDriverMapPages);
    auto sceSblDriverUnmapPages = (int(*)(uint64_t gpu_desc))kdlsym(sceSblDriverUnmapPages);
    auto sceSblServiceMailbox = (int(*)(uint32_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);
    auto make_chunk_table_system = (int(*)(uint64_t *segment_info_gpu_paddr, uint64_t *segment_info_gpu_desc, void *segment_info_cpu_vaddr, size_t segment_info_size, void *chunk_table_cpu_vaddr, size_t chunk_table_size, int type))kdlsym(make_chunk_table_system);
    auto map_chunk_table = (int(*)(uint64_t *gpu_paddr, uint64_t *gpu_desc, void *cpu_vaddr))kdlsym(map_chunk_table);
    auto s_sm_sxlock = (struct sx*)kdlsym(s_sm_sxlock);
    auto _sx_xlock = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto _sx_xunlock = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);

    bool s_Success = false;
    uint64_t s_SegmentDataGpuPAddr = 0;
    uint64_t s_SegmentDataGpuDesc = 0;
    uint8_t* s_SegmentData = nullptr;
    uint64_t s_SegmentDataSize = 0;
    uint64_t s_ChunkTableGpuPAddr = 0;
    uint64_t s_ChunkTableGpuDesc = 0;
    uint8_t* s_ChunkTable = nullptr;
    //uint64_t s_InputMapped = 0;
    //uint64_t s_InputMapDesc = 0;
    int32_t s_Ret = 0;
    const size_t c_ChunkTableSize = 0x4000;
    uint8_t s_Payload[0x80] = { 0 };

    // This is assigned to payload, don't attempt to free
    sbl_authmgr_load_self_segment_t* s_Command = nullptr;

    if (p_OutputData == nullptr)
    {
        WriteLog(LL_Error, "no output data provided.");
        goto cleanup;
    }

    if (p_InOutOutputSize == nullptr)
    {
        WriteLog(LL_Error, "no input/output size provided");
        goto cleanup;
    }

    // Create the segment data and copy it over
    s_SegmentDataSize = ALIGN_PAGE(p_InputDataLength);
    s_SegmentData = new uint8_t[s_SegmentDataSize];
    if (s_SegmentData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate segment data");
        goto cleanup;
    }
    memset(s_SegmentData, 0, s_SegmentDataSize);
    memcpy(s_SegmentData, p_InputData, p_InputDataLength);
    
    // Create a new chunk table buffer
    
    s_ChunkTable = new uint8_t[c_ChunkTableSize];
    if (s_ChunkTable == nullptr)
    {
        WriteLog(LL_Error, "could not allocate chunk table");
        goto cleanup;
    }
    memset(s_ChunkTable, 0, c_ChunkTableSize);
    
    // Create the chunk table system
    s_Ret = make_chunk_table_system(
        &s_SegmentDataGpuPAddr,
        &s_SegmentDataGpuDesc,
        s_SegmentData,
        s_SegmentDataSize,
        s_ChunkTable,
        c_ChunkTableSize,
        1);
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "make_chunk_table_system returned: (%d).", s_Ret);
        goto cleanup;
    }
    
    // Validate our returned arguments
    if (s_SegmentDataGpuPAddr == 0 || s_SegmentDataGpuDesc == 0)
    {
        WriteLog(LL_Error, "cinvalid segment data paddr (%llx) or segment data gpu desc (%llx)", s_SegmentDataGpuPAddr, s_SegmentDataGpuDesc);
        goto cleanup;
    }

    // Map the chunk table
    s_Ret = map_chunk_table(
        &s_ChunkTableGpuPAddr,
        &s_ChunkTableGpuDesc,
        s_ChunkTable);
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "map_chunk_table returned: (%d).", s_Ret);
        goto cleanup;
    }

    // Decrypt the segment
    s_Command = reinterpret_cast<sbl_authmgr_load_self_segment_t*>(&s_Payload[0]);
    memset(s_Command, 0, sizeof(s_Payload));

    s_Command->function = AUTHMGR_CMD_LOAD_SELF_SEGMENT;
    s_Command->status = 0;
    s_Command->chunk_table_addr = s_ChunkTableGpuPAddr;
    s_Command->segment_index = p_SegmentIndex;
    s_Command->is_block_table = p_IsBlockTable;
    s_Command->context_id = m_Self.auth_ctx_id;

    WriteLog(LL_Debug, "Sending load self segment");
    sceSblServiceMailbox_locked(s_Ret, m_Self.svc_id, &s_Payload, &s_Payload);

    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "sceSblServiceMailbox_locked returned: (%d).", s_Ret);
        goto cleanup;
    }

    if (s_Command->status != 0)
    {
        WriteLog(LL_Error, "command status returned: (%d).", s_Command->status);
        goto cleanup;
    }

    if (s_Command->function != AUTHMGR_CMD_LOAD_SELF_SEGMENT)
    {
        WriteLog(LL_Error, "invalid function");
        goto cleanup;
    }

    // Bounds check the output size
    if (*p_InOutOutputSize < p_InputDataLength)
    {
        WriteLog(LL_Error, "not enough output space: (%lld) < (%lld) bytes.", *p_InOutOutputSize, p_InputDataLength);
        goto cleanup;
    }

    // Copy out the data
    memcpy(p_OutputData, s_SegmentData, p_InputDataLength);

    // Set our success
    s_Success = true;

cleanup:
    // Free up all of the resources
    if (s_ChunkTableGpuPAddr)
    {
        if ((s_Ret = sceSblDriverUnmapPages(s_ChunkTableGpuDesc)) != 0)
            WriteLog(LL_Error, "could not unmap pages: (%d).", s_Ret);
    }

    if (s_SegmentDataGpuPAddr)
    {
        if ((s_Ret = sceSblDriverUnmapPages(s_SegmentDataGpuDesc)) != 0)
            WriteLog(LL_Error, "could not unmap pages: (%d).", s_Ret);
    }

    if (s_ChunkTable)
        delete [] s_ChunkTable;
    
    if (s_SegmentData)
        delete [] s_SegmentData;

    return s_Success;
}

bool SelfDecrypt::DecryptBlock(uint8_t* p_BlobData, uint64_t p_BlobSize, self_block_info_t* p_Block, int p_SegmentIndex, int p_ContextId)
{
    auto sceSblDriverMapPages = (int(*)(uint64_t *gpu_paddr, void *cpu_vaddr, uint32_t npages, uint64_t flags, uint64_t unk, uint64_t *gpu_desc))kdlsym(sceSblDriverMapPages);
    /*auto sceSblDriverUnmapPages = (int(*)(uint64_t gpu_desc))kdlsym(sceSblDriverUnmapPages);
    auto sceSblServiceMailbox = (int(*)(uint32_t p_ServiceId, void* p_Request, void* p_Response))kdlsym(sceSblServiceMailbox);
    auto make_chunk_table_system = (int(*)(uint64_t *segment_info_gpu_paddr, uint64_t *segment_info_gpu_desc, void *segment_info_cpu_vaddr, size_t segment_info_size, void *chunk_table_cpu_vaddr, size_t chunk_table_size, int type))kdlsym(make_chunk_table_system);
    auto map_chunk_table = (int(*)(uint64_t *gpu_paddr, uint64_t *gpu_desc, void *cpu_vaddr))kdlsym(map_chunk_table);
    auto s_sm_sxlock = (struct sx*)kdlsym(s_sm_sxlock);
    auto _sx_xlock = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto _sx_xunlock = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);*/

    uint8_t* s_InputData = nullptr;
    uint64_t s_InputSize = 0;
    uint64_t s_InputMapped = 0;
    uint64_t s_InputMapDesc = 0;

    uint8_t* s_OutputData = nullptr;

    uint64_t s_OutputSize = 0;
    uint64_t s_OutputMapped = 0;
    uint64_t s_OutputMapDesc = 0;

    uint8_t s_Payload[0x80] = { 0 };

    // Not allocated do not free
    sbl_authmgr_load_self_block_t* s_Command = nullptr;

    bool s_Success = false;

    int32_t s_Ret = 0;

    // Get the input data size and allocate new buffers
    s_InputSize = ALIGN_PAGE(p_BlobSize);
    s_InputData = new uint8_t[s_InputSize];
    if (s_InputData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate input data of size (%llx).", s_InputSize);
        goto cleanup;
    }
    memset(s_InputData, 0, s_InputSize);
    memcpy(s_InputData, p_BlobData, p_BlobSize);

    if ((s_Ret = sceSblDriverMapPages(&s_InputMapped, s_InputData, 1, 0x61, 0, &s_InputMapDesc)) != 0)
    {
        WriteLog(LL_Error, "could not map driver pages: (%d).", s_Ret);
        goto cleanup;
    }

    s_OutputSize = ALIGN_PAGE(p_BlobSize);
    s_OutputData = new uint8_t[s_OutputSize];
    if (s_OutputData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate output data of size (%llx).", s_OutputData);
        goto cleanup;
    }
    memset(s_OutputData, 0, s_OutputSize);

    if ((s_Ret = sceSblDriverMapPages(&s_OutputMapped, s_OutputData, 1, 0x61, 0, &s_OutputMapDesc)) != 0)
    {
        WriteLog(LL_Error, "could not map driver pages: (%d).", s_Ret);
        goto cleanup;
    }

    // Decrypt block
    s_Command = reinterpret_cast<sbl_authmgr_load_self_block_t*>(&s_Payload[0]);
    memset(s_Payload, 0, sizeof(s_Payload));
    
    memcpy(&s_Command->digest, &p_Block->digest, sizeof(p_Block->digest));
    memcpy(&s_Command->extent, &p_Block->extent, sizeof(p_Block->extent));

    s_Command->function = AUTHMGR_CMD_LOAD_SELF_BLOCK;
    s_Command->status = 0;
    s_Command->pages_addr = s_OutputMapped;
    s_Command->segment_index = p_SegmentIndex;
    s_Command->context_id = p_ContextId;
    s_Command->block_index = p_Block->index;

cleanup:
    return s_Success;
}
#endif