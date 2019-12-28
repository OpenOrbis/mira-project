#pragma once
#ifndef _WIN32
#include <Utils/Types.hpp>
#endif

#include <sys/elf64.h>
#include <sys/elf_common.h>

struct thread;
struct proc;

namespace TrainerLoader
{
    class Loader
    {
    private:
        enum { MaxSegments = 16 };

        const void* m_SourceElf;
        uint32_t m_SourceElfSize;

        // Incoming source header information
        const Elf64_Ehdr* m_SourceHeader;

        // Program header start
        const Elf64_Phdr* m_SourceProgramHeadersStart;

        const Elf64_Phdr* m_LoadableSegments[MaxSegments];
        uint32_t m_LoadableSegmentsCount;

        const Elf64_Phdr* m_SourcePhphdr; // :( what the fuck is this

        const Elf64_Phdr* m_SourceDynamicProgramHeader;
        const Elf64_Phdr* m_SourceInterpreterProgramHeader;

        // Allocated data for all loadable segments
        void* m_UserlandAllocatedMap;
        Elf64_Xword m_AllocatedMapSize;

        // Dynamic in mapped elf
        Elf64_Dyn* m_UserlandDynamic;

        // Hash, buckets and chains in mapped elf
        Elf64_Hashelt m_BucketsCount;
        Elf64_Hashelt m_ChainsCount;
        const Elf64_Hashelt* m_UserlandBuckets;
        const Elf64_Hashelt* m_UserlandChains;

        // String table in mapped elf
        const char* m_UserlandStringTable;
        Elf64_Xword m_StringTableSize;

        // Symbol table in mapped elf
        const Elf64_Sym* m_UserlandSymbolTable;

        // GOT in mapped elf
        const Elf64_Addr* m_UserlandGlobalOffsetTable;

        // Relocations (rel) in mapped elf
        const Elf64_Rel* m_UserlandRel;
        Elf64_Xword m_RelSize;

        // PLT relocations (rel/rela) in mapped elf
        const Elf64_Rel* m_UserlandPltRel;
        Elf64_Xword m_PltRelSize;

        const Elf64_Rela* m_PltRela;
        Elf64_Xword m_PltRelaSize;

        // Relocations (rela) in mapped elf
        const Elf64_Rela* m_UserlandRela;
        Elf64_Xword m_RelaSize;

        // Debug in mapped elf
        Elf64_Addr m_Debug;

        // Symbol table in mapped elf (we use this one, not source)
        const Elf64_Sym* m_DdbSymbolTable;

        // Number of symbols in mapped elf
        Elf64_Xword m_DdbSymbolCount;

        // String table in mapped elf
        const char* m_DdbStringTable;

        // Total bytes in the string table in mapped elf
        Elf64_Xword m_DdbStringCount;

        // Pre-relocation pcpu set start
        Elf64_Addr m_PcpuStart;

        // Pre-relocation pcpu set stop
        Elf64_Addr m_PcpuStop;

        // Relocated pcpu set address
        Elf64_Addr m_PcpuBase;

        // Allocated symbol table information
        Elf64_Addr m_UserlandSymbolTableBase;
        Elf64_Xword m_SymbolTableSize;

        // Allocated string table base
        Elf64_Addr m_StringTableBase;
        Elf64_Xword m_StringTableBaseSize;

        // Entrypoint tracking
        void* m_UserlandEntrypoint;

        int32_t m_TargetProcessId;

    public:
        Loader(int32_t p_TargetProcessId, const void* p_Elf, uint32_t p_ElfSize);
        ~Loader();

        void* GetEntrypoint() { return m_UserlandEntrypoint; }
        void* GetAllocatedMap() { return m_UserlandAllocatedMap; }
        Elf64_Xword GetAllocatedMapSize() { return m_AllocatedMapSize; }

    #ifdef _WIN32
        void DumpLoadedElf(const char* p_Path);
        void* DumpAllocatedMap() { return m_AllocatedMap; }
        Elf64_Xword DumpAllocatedMapSize() { return m_AllocatedMapSize; }
    #endif
    protected:
        bool Load();

        bool ElfReloc(Elf64_Addr p_RelocationBase, const void* p_Data, int32_t p_RelocationType);
        bool ElfRelocLocal(Elf64_Addr p_RelocationBase, const void* p_Data, int32_t p_RelocationType);
        bool ElfRelocInternal(Elf64_Addr p_RelocationBase, const void* p_Data, int32_t p_RelocationType, bool p_Local);

        
        // elf_lookup
        Elf64_Addr LookupInUserland(Elf64_Size p_SymbolIndex, bool p_CheckDependencies);

        Elf64_Addr LinkerFileLookupSymbol(const char* p_Name, bool p_CheckDependencies);
        Elf64_Sym* LinkerFileLookupSymbolInternal(const char* p_Name, bool p_CheckDependencies);

        bool ParseDynamic();
        bool ParseDpcpu();

        bool RelocateFile();

        bool SetProtection(void* p_Address, Elf64_Xword p_Size, Elf64_Word p_Protection);
        bool SetUserProtection(int32_t p_ProcessId, void* p_UserAddress, Elf64_Xword p_Size, Elf64_Word p_Protection);

        const Elf64_Phdr* GetSourceProgramHeaderByIndex(uint32_t p_Index);

    private:
        void* Allocate(Elf64_Xword p_Size);
        void* AllocateUser(int32_t p_ProcessId, Elf64_Xword p_Size);
        struct thread* GetMainThreadByPid(int32_t p_ProcessId);

        void Free(void* p_Data);
        static int Strcmp(const char* s1, const char* s2);
        int StrcmpUser(const char* s1, const char* s2);
        static uint32_t Hash(const char* p_Name);
        uint32_t HashUser(const char* p_UserlandName);

        uint32_t ReadProcessMemory(int32_t p_Process, void* p_Address, uint32_t p_Size, void* p_OutputBuffer);
        uint32_t WriteProcessMemory(int32_t p_Process, void* p_Address, uint32_t p_Size, void* p_InputBuffer);

        static bool IsProcessAlive(int32_t p_ProcessId);
    };
}

