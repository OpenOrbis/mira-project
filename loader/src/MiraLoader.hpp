#pragma once
#include <Utils/Types.hpp>
#include <sys/elf64.h>

namespace MiraLoader
{
    class Loader
    {
    private:
        // Reference to the data
        uint8_t* m_Data;

        // Data length
        uint64_t m_DataLength;

        // The allocated data for all segments
        uint8_t* m_AllocatedData;

        // Allocated data size for all segments
        uint64_t m_AllocatedDataSize;

        // Entrypoint to jump to
        void(*m_EntryPoint)(void*);

        // Should we use kernel functions or userland
        bool m_IsInKernel;

    public:
        Loader(uint8_t* p_ElfBuffer, uint64_t p_BufferSize, bool p_IsInKernel = false);
        ~Loader();

        auto GetEntrypoint() -> void(*)(void*) { return m_EntryPoint; }
        static void Memcpy(void* p_Destination, void* p_Source, uint64_t p_Size);
        static void Memset(void* p_Address, uint8_t p_Value, uint64_t p_Length);
        static void WriteNotificationLog(const char* p_Message);

        const uint8_t* GetAllocatedData() { return m_AllocatedData; }
        const uint64_t GetAllocatedDataSize() { return m_AllocatedDataSize; }
    private:
        uint64_t RoundUp(uint64_t p_Number, uint64_t p_Multiple);
        static int32_t Strcmp(const char* p_First, const char* p_Second);
        void* Allocate(uint32_t p_Size);
        #ifdef _WIN32
        void* Win32Allocate(uint32_t p_Size);
        #endif
        bool SetProtection(void* p_Address, uint64_t p_Size, int32_t p_Protection);
        //void WriteLog(const char* p_Function, int32_t p_Line, const char* p_Format, ...);
        // Processing functions
        /*
            In order to properly load an elf you should do these things in this order

            ctor(elfBuffer, elfBufferSize);
            LoadSegments();
            RelocateELf();
            UpdateProtections();
            GetEntrypoint();
        */

        // Loads all of the segments into the m_AllocatedData(/Size)
        bool LoadSegments();

        // Handle all of the relocations
        bool RelocateElf();

        bool UpdateProtections();

    protected:
        uint64_t GetLoadableSegmentsSize();

        Elf64_Dyn* GetDynamicByTag(uint64_t p_Tag);
        Elf64_Phdr* GetProgramHeaderByIndex(uint32_t p_Index);
        Elf64_Shdr* GetSectionHeaderByIndex(uint32_t p_Index);

        Elf64_Phdr* GetProgramHeaderByFileOffset(uint64_t p_FileOffset);
    };
}