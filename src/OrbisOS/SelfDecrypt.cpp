#include "SelfDecrypt.hpp"
#include "Utils/SysWrappers.hpp"    // TODO: Brackets
#include "Utils/Logger.hpp"
#include "Utils/Kernel.hpp"

#include "Mira.hpp"

extern "C"
{
    #include "sys/fcntl.h"
    #include "sys/unistd.h"
}

using namespace Mira::OrbisOS;

SelfDecrypt::SelfDecrypt(const char* p_FilePath) :
    m_Self { 0 }
{
    // Before we do anything assign a invalid fd
    m_Self.fd = -1;

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

    return true;
}