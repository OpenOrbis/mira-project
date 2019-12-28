#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "TrainerLoader.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#else
#include "TrainerLoader.hpp"
//#include <Utils/New.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Logger.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
    #include <sys/proc.h>
    #include <sys/mman.h>
};

using namespace TrainerLoader;

extern void WriteNotificationLog(const char* text);
#endif

/* values for type */
#define ELF_RELOC_REL	1
#define ELF_RELOC_RELA	2

// https://github.com/lattera/freebsd/blob/master/sys/kern/kern_linker.c
// https://github.com/lattera/freebsd/blob/401a161083850a9a4ce916f37520c084cff1543b/sys/amd64/amd64/elf_machdep.c
// https://github.com/lattera/freebsd/blob/master/sys/kern/link_elf.c
// https://github.com/lattera/freebsd/tree/master/include
// https://github.com/OpenOrbis/mira-project/blob/master/Firmware/MiraFW/src/mira/plugins/debugger/debugger_plugin.c

Loader::Loader(int p_TargetProcessId, const void* p_Elf, uint32_t p_ElfSize) :
	m_SourceElf(p_Elf),
	m_SourceElfSize(p_ElfSize),
	m_SourceHeader(nullptr),
	m_SourceProgramHeadersStart(nullptr),
	m_LoadableSegments{ 0 },
	m_LoadableSegmentsCount(0),
	m_SourcePhphdr(nullptr),
	m_SourceDynamicProgramHeader(nullptr),
	m_SourceInterpreterProgramHeader(nullptr),
	m_UserlandAllocatedMap(nullptr),
	m_AllocatedMapSize(0),
	m_UserlandDynamic(nullptr),
	m_BucketsCount(0),
	m_ChainsCount(0),
	m_UserlandBuckets(nullptr),
	m_UserlandChains(nullptr),
	m_UserlandStringTable(nullptr),
	m_StringTableSize(0),
	m_UserlandSymbolTable(nullptr),
	m_UserlandGlobalOffsetTable(nullptr),
	m_UserlandRel(nullptr),
	m_RelSize(0),
	m_UserlandPltRel(nullptr),
	m_PltRelSize(0),
	m_PltRela(nullptr),
	m_PltRelaSize(0),
	m_UserlandRela(nullptr),
	m_RelaSize(0),
	m_Debug(0),
	m_DdbSymbolTable(nullptr),
	m_DdbSymbolCount(0),
	m_DdbStringTable(nullptr),
	m_DdbStringCount(0),
	m_PcpuStart(0),
	m_PcpuStop(0),
	m_PcpuBase(0),
	m_UserlandSymbolTableBase(0),
	m_SymbolTableSize(0),
	m_StringTableBase(0),
	m_StringTableBaseSize(0),
	m_UserlandEntrypoint(nullptr),
	m_TargetProcessId(p_TargetProcessId)
{
	if (!Load())
        WriteLog(LL_Error, "could not load properly");
}


Loader::~Loader()
{
}

bool Loader::ElfReloc(Elf64_Addr p_RelocationBase, const void * p_Data, int32_t p_RelocationType)
{
	return ElfRelocInternal(p_RelocationBase, p_Data, p_RelocationType, false);
}

bool Loader::ElfRelocLocal(Elf64_Addr p_RelocationBase, const void * p_Data, int32_t p_RelocationType)
{
	return ElfRelocInternal(p_RelocationBase, p_Data, p_RelocationType, true);
}

bool Loader::ElfRelocInternal(Elf64_Addr p_RelocationBase, const void * p_Data, int32_t p_RelocationType, bool p_Local)
{
    auto printf = (void(*)(const char *format, ...))kdlsym(printf);

	Elf64_Addr* s_Where, s_Val;
	uint32_t* s_Where32, s_Val32;
	Elf64_Addr s_Addr;
	Elf64_Addr s_Addend;
	Elf64_Size s_Type, s_SymbolIndex;

	Elf64_Rel s_Rel;
	Elf64_Rela s_Rela;

	switch (p_RelocationType)
	{
		case ELF_RELOC_REL:
		{
			if (ReadProcessMemory(m_TargetProcessId, const_cast<void*>(p_Data), sizeof(Elf64_Rela), &s_Rel) != sizeof(Elf64_Rela))
			{
				WriteLog(LL_Error, "could not read rel entry from (%p)", p_Data);
				return false;
			}
			s_Where = reinterpret_cast<Elf64_Addr*>(p_RelocationBase + s_Rel.r_offset);

			s_Type = ELF64_R_TYPE(s_Rel.r_info);
			s_SymbolIndex = ELF64_R_SYM(s_Rel.r_info);

			switch (s_Type)
			{
				/* Addend is 32 bit on 32 bit relocs */
			case R_X86_64_PC32:
			case R_X86_64_32S:
				if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(Elf64_Addr), &s_Addend) != sizeof(Elf64_Addr))
				{
					WriteLog(LL_Error, "could not read addend from (%p)", s_Where);
					return false;
				}
				//s_Addend = *(Elf64_Addr*)s_Where;
				break;
			default:
				if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(Elf64_Addr), &s_Addend) != sizeof(Elf64_Addr))
				{
					WriteLog(LL_Error, "could not read addend from (%p)", s_Where);
					return false;
				}
				//s_Addend = *s_Where;
				break;
			}
			break;
		}
		case ELF_RELOC_RELA:
		{
			if (ReadProcessMemory(m_TargetProcessId, const_cast<void*>(p_Data), sizeof(s_Rela), &s_Rela) != sizeof(s_Rela))
			{
				WriteLog(LL_Error, "could not read rela entry from (%p)", p_Data);
				return false;
			}

			//s_Rela = static_cast<const Elf64_Rela*>(p_Data);
			s_Where = (Elf64_Addr*)(p_RelocationBase + s_Rela.r_offset);
			s_Addend = s_Rela.r_addend;
			s_Type = ELF64_R_TYPE(s_Rela.r_info);
			s_SymbolIndex = ELF64_R_SYM(s_Rela.r_info);
			break;
		}
		default:
			WriteLog(LL_Error, "unknown reloc type %d\n", p_RelocationType);		
			return false;
		}

		switch (s_Type)
		{
		case R_X86_64_NONE:	/* none */
			break;

		case R_X86_64_64:		/* S + A */
		{
			Elf64_Addr s_Temp = 0;
			if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Temp), &s_Temp) != sizeof(s_Temp))
			{
				WriteLog(LL_Error, "could not handle R_X86_64_64 from (%p)", s_Where);
				return false;
			}
			s_Addr = LookupInUserland(s_SymbolIndex, true);
			s_Val = s_Addr + s_Addend;
			if (s_Addr == 0)
				return false;
			if (s_Temp != s_Val)
			{
				if (WriteProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Val), &s_Val) != sizeof(s_Val))
				{
					WriteLog(LL_Error, "could not write (%p) to (%p)", s_Val, s_Where);
					return false;
				}
			}
			/*if (*s_Where != s_Val)
				*s_Where = s_Val;*/
			break;
		}
		case R_X86_64_PC32:	/* S + A - P */
		{
			uint32_t s_Temp = 0;
			if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Temp), &s_Temp) != sizeof(s_Temp))
			{
				WriteLog(LL_Error, "could not handle R_X86_64_64 from (%p)", s_Where);
				return false;
			}

			s_Addr = LookupInUserland(s_SymbolIndex, true);
			s_Where32 = (uint32_t *)s_Where;
			s_Val32 = (uint32_t)(s_Addr + s_Addend - (Elf64_Addr)s_Where);
			if (s_Addr == 0)
				return false;
			if (s_Temp != s_Val32)
			{
				if (WriteProcessMemory(m_TargetProcessId, s_Where32, sizeof(s_Val32), &s_Val32) != sizeof(s_Val32))
				{
					WriteLog(LL_Error, "could not write (%p) to (%p)", s_Val32, s_Where);
					return false;
				}
			}
			/*if (*s_Where32 != s_Val32)
				*s_Where32 = s_Val32;*/
			break;
		}
		case R_X86_64_32S:	/* S + A sign extend */
		{
			uint32_t s_Temp = 0;
			if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Temp), &s_Temp) != sizeof(s_Temp))
			{
				WriteLog(LL_Error, "could not handle R_X86_64_64 from (%p)", s_Where);
				return false;
			}

			s_Addr = LookupInUserland(s_SymbolIndex, true);
			s_Val32 = (uint32_t)(s_Addr + s_Addend);
			s_Where32 = (uint32_t *)s_Where;
			if (s_Addr == 0)
				return false;

			if (s_Temp != s_Val32)
			{
				if (WriteProcessMemory(m_TargetProcessId, s_Where32, sizeof(s_Val32), &s_Val32) != sizeof(s_Val32))
				{
					WriteLog(LL_Error, "could not write (%p) to (%p)", s_Val32, s_Where);
					return false;
				}
			}
			/*if (*s_Where32 != s_Val32)
				*s_Where32 = s_Val32;*/
			break;
		}
		case R_X86_64_COPY:	/* none */
			/*
			* There shouldn't be copy relocations in kernel
			* objects.
			*/
			printf("kldload: unexpected R_COPY relocation\n");
			return false;
		case R_X86_64_GLOB_DAT:	/* S */
		case R_X86_64_JMP_SLOT:	/* XXX need addend + offset */
		{
			Elf64_Addr s_Temp = 0;
			if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Temp), &s_Temp) != sizeof(s_Temp))
			{
				WriteLog(LL_Error, "could not handle R_X86_64_64 from (%p)", s_Where);
				return false;
			}
			s_Addr = LookupInUserland(s_SymbolIndex, true);
			if (s_Addr == 0)
				return false;
			
			if (s_Temp != s_Addr)
			{
				if (WriteProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Addr), &s_Addr) != sizeof(s_Addr))
				{
					WriteLog(LL_Error, "could not write (%p) to (%p)", s_Addr, s_Where);
					return false;
				}
			}
			/*if (*s_Where != s_Addr)
				*s_Where = s_Addr;*/
			break;
		}
		case R_X86_64_RELATIVE:	/* B + A */
		{
			Elf64_Addr s_Temp = 0;
			if (ReadProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Temp), &s_Temp) != sizeof(s_Temp))
			{
				WriteLog(LL_Error, "could not handle R_X86_64_64 from (%p)", s_Where);
				return false;
			}
			s_Addr = p_RelocationBase + s_Addend;
			s_Val = s_Addr;

			if (s_Temp != s_Val)
			{
				if (WriteProcessMemory(m_TargetProcessId, s_Where, sizeof(s_Val), &s_Val) != sizeof(s_Val))
				{
					WriteLog(LL_Error, "could not write (%p) to (%p)", s_Val, s_Where);
					return false;
				}
			}
			/*if (*s_Where != s_Val)
				*s_Where = s_Val;*/
			break;
		}
		default:
			printf("kldload: unexpected relocation type %lld\n",
			s_Type);
			return false;
	}

	return true;
}

Elf64_Addr Loader::LookupInUserland(Elf64_Size p_SymbolIndex, bool p_CheckDependencies)
{
	if (p_SymbolIndex >= m_ChainsCount)
		return 0;

	Elf64_Sym s_Symbol;
	if (ReadProcessMemory(m_TargetProcessId, (void*)(m_UserlandSymbolTable + p_SymbolIndex), sizeof(s_Symbol), &s_Symbol) != sizeof(s_Symbol))
	{
		WriteLog(LL_Error, "could not read symbol from (%p)", (m_UserlandSymbolTable + p_SymbolIndex));
		return 0;
	}// = m_UserlandSymbolTable + p_SymbolIndex;
	//const Elf64_Sym* s_Symbol = m_UserlandSymbolTable + p_SymbolIndex;
	const char* s_SymbolName = "";

	// Skip doing a full lookup when the symbol is local, it may even fail because it may not be found through hash tables
	if (ELF64_ST_BIND(s_Symbol.st_info) == STB_LOCAL)
	{
		// FreeBSD dev's are smoking crack wtf
		if (s_Symbol.st_shndx == SHN_UNDEF || s_Symbol.st_value == 0)
			return 0;
		return reinterpret_cast<Elf64_Addr>(m_UserlandAllocatedMap) + s_Symbol.st_value;
	}

	s_SymbolName = m_UserlandStringTable + s_Symbol.st_name;
	if (s_SymbolName == 0)
		return 0;


	// TODO: Implement
	//const auto s_Symbol2 = LinkerFileLookupSymbol(s_SymbolName, p_CheckDependencies);

    WriteLog(LL_Error, "Lookup not implemented\n");
	return 0;
}

Elf64_Addr Loader::LinkerFileLookupSymbol(const char * p_Name, bool p_CheckDependencies)
{
	// TODO: Implement
	//Elf64_Sym* s_Symbol = LinkerFileLookupSymbolInternal(p_Name, true);

    WriteLog(LL_Error, "LinkerFileLookupSymbol not implemented\n");
	return 0;
}

/**
 * 
*/
Elf64_Sym* Loader::LinkerFileLookupSymbolInternal(const char * p_NameInUserlandStringTable, bool p_CheckDependencies)
{
	if (m_UserlandBuckets == nullptr || m_BucketsCount == 0)
	{
        WriteLog(LL_Error, "LinkerFileLookupSymbolInternal: missing symbol hash table\n");
		return nullptr;
	}

	Elf64_Sym s_UserlandSymp;
	const char* s_UserlandStrp = "";
	auto s_Hash = HashUser(p_NameInUserlandStringTable);
	Elf64_Hashelt s_SymbolNum;// = m_UserlandBuckets[s_Hash % m_BucketsCount];
	if (ReadProcessMemory(m_TargetProcessId, (void*)(&m_UserlandBuckets[s_Hash % m_BucketsCount]), sizeof(s_SymbolNum), &s_SymbolNum) != sizeof(s_SymbolNum))
	{
		WriteLog(LL_Error, "could not read userland buckets (%p)", (&m_UserlandBuckets[s_Hash % m_BucketsCount]));
		return nullptr;
	}

	while (s_SymbolNum != STN_UNDEF)
	{
		if (s_SymbolNum >= m_ChainsCount)
		{
            WriteLog(LL_Error, "corrupt symbol table\n");
			return nullptr;
		}

		//s_UserlandSymp = m_UserlandSymbolTable + s_SymbolNum;
		if (ReadProcessMemory(m_TargetProcessId, (void*)(m_UserlandSymbolTable + s_SymbolNum), sizeof(s_UserlandSymp), &s_UserlandSymp))
		{
			WriteLog(LL_Error, "could not read userland symp (%p)", (m_UserlandSymbolTable + s_SymbolNum));
			return nullptr;
		}

		if (s_UserlandSymp.st_name == 0)
		{
            WriteLog(LL_Error, "corrupt symbol table\n");
			return nullptr;
		}

		s_UserlandStrp = m_UserlandStringTable + s_UserlandSymp.st_name;
		if (StrcmpUser(p_NameInUserlandStringTable, s_UserlandStrp) == 0)
		{
			if (s_UserlandSymp.st_shndx != SHN_UNDEF ||
				(s_UserlandSymp.st_value != 0 && ELF64_ST_TYPE(s_UserlandSymp.st_info) == STT_FUNC))
			{
				return const_cast<Elf64_Sym*>(m_UserlandSymbolTable + s_SymbolNum);
			}

			return nullptr;
		}

		if (ReadProcessMemory(m_TargetProcessId, (void*)&m_UserlandChains[s_SymbolNum], sizeof(s_SymbolNum), &s_SymbolNum) != sizeof(s_SymbolNum))
		{
			WriteLog(LL_Error, "could not read userland chain (%p)", (void*)&m_UserlandChains[s_SymbolNum]);
			return nullptr;
		}
		//s_SymbolNum = m_UserlandChains[s_SymbolNum];
	}

	// If we have not found it, look at the full table (if loaded)
	if (m_UserlandSymbolTable == m_DdbSymbolTable)
		return nullptr;

	// Exhaustive search
	
	for (auto i = 0; i < m_DdbSymbolCount; i++)
	{
		//s_UserlandSymp = m_DdbSymbolTable + i;
		if (ReadProcessMemory(m_TargetProcessId, (void*)(m_DdbSymbolTable + i), sizeof(s_UserlandSymp), &s_UserlandSymp) != sizeof(s_UserlandSymp))
		{
			WriteLog(LL_Error, "could not read ddbsymbol table index (%d)", i);
			return nullptr;
		}

		s_UserlandStrp = m_DdbStringTable + s_UserlandSymp.st_name;
		if (StrcmpUser(p_NameInUserlandStringTable, s_UserlandStrp) == 0)
		{
			if (s_UserlandSymp.st_shndx != SHN_UNDEF ||
				(s_UserlandSymp.st_value != 0 && ELF64_ST_TYPE(s_UserlandSymp.st_info) == STT_FUNC))
			{
				return const_cast<Elf64_Sym*>(m_DdbSymbolTable + i);
			}
			return nullptr;
		}
	}
    WriteLog(LL_Error, "symbol not found\n");
	return nullptr;
}

bool Loader::RelocateFile()
{
	const Elf64_Rel* s_RelLimit = nullptr;
	const Elf64_Rel* s_Rel = nullptr;
	const Elf64_Rela* s_RelaLimit = nullptr;
	const Elf64_Rela* s_Rela = nullptr;


	// Perform relocations without addend if there are any
	if ((s_Rel = m_UserlandRel) != nullptr)
	{
		s_RelLimit = reinterpret_cast<const Elf64_Rel*>(reinterpret_cast<const uint8_t*>(m_UserlandRel) + m_RelSize);
		while (s_Rel < s_RelLimit)
		{
			if (!ElfReloc(reinterpret_cast<Elf64_Addr>(m_UserlandAllocatedMap), s_Rel, ELF_RELOC_REL))
                WriteLog(LL_Warn, "could not elf reloc local for rel's");

			s_Rel++;
		}
	}

	// Perform relocations with addend if there are any
	if ((s_Rela = m_UserlandRela) != nullptr)
	{
		s_RelaLimit = reinterpret_cast<const Elf64_Rela*>(reinterpret_cast<const uint8_t*>(m_UserlandRela) + m_RelaSize);
		while (s_Rela < s_RelaLimit)
		{
			if (!ElfReloc(reinterpret_cast<Elf64_Addr>(m_UserlandAllocatedMap), s_Rela, ELF_RELOC_RELA))
                WriteLog(LL_Warn, "could not elf reloc local for rela's");				

			s_Rela++;
		}
	}

	return true;
}

bool Loader::SetProtection(void * p_Address, Elf64_Xword p_Size, Elf64_Word p_Protection)
{
	if (p_Address == nullptr)
		return false;

#ifdef _WIN32
	DWORD s_Error = ERROR_SUCCESS;
	DWORD s_OldProtection = 0;
	if (!VirtualProtect(p_Address, p_Size, p_Protection, &s_OldProtection))
	{
		s_Error = GetLastError();
		WriteLog(LL_Error, "VirtualProtect failed (%d)\n", s_Error);
		return false;
	}
#else
    // TODO: pmap_protect
#endif

/*
auto s_MainThread = GetMainThreadByPid()
    if (kmprotect_t(p_Address, p_Size, p_Protection) < 0)
    {
        WriteNotificationLog("mprotect error\n");
        return false;
    }*/
	return true;
}

bool Loader::SetUserProtection(int32_t p_ProcessId, void * p_Address, Elf64_Xword p_Size, Elf64_Word p_Protection)
{
	if (p_Address == nullptr)
		return false;

    auto s_MainThread = GetMainThreadByPid(p_ProcessId);
    if (s_MainThread == nullptr)
        return false;
    
    if (kmprotect_t(p_Address, p_Size, p_Protection, s_MainThread) < 0)
    {
        WriteLog(LL_Error, "mprotect error\n");
        return false;
    }

	return true;
}

bool Loader::ParseDynamic()
{
	if (m_UserlandDynamic == nullptr)
		return false;

	auto s_PltType = DT_REL;

	// TODO: More bounds checking
	// TODO: Convert this to reading from the process instead of direct deref
	// TODO: Fix all instances of dereferencing and pray to whoever that this works
	auto s_DynIndex = 0;
	Elf64_Dyn s_Dyn;
	do
	{
		if (ReadProcessMemory(m_TargetProcessId, (m_UserlandDynamic + s_DynIndex), sizeof(s_Dyn), &s_Dyn) != sizeof(s_Dyn))
		{
			WriteLog(LL_Error, "could not read dyn");
			return false;
		}

		switch (s_Dyn.d_tag)
		{
			case DT_HASH:
			{
				// Stole from: src/libexec/rtld-elf/rtld.c
				const Elf64_Hashelt* l_UserlandHashTable = reinterpret_cast<const Elf64_Hashelt*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);

				if (ReadProcessMemory(m_TargetProcessId, (void*)l_UserlandHashTable, sizeof(Elf64_Hashelt), &m_BucketsCount) != sizeof(Elf64_Hashelt))
					WriteLog(LL_Error, "could not read bucket count");
				
				if (ReadProcessMemory(m_TargetProcessId, (void*)(l_UserlandHashTable + 1), sizeof(Elf64_Hashelt), &m_ChainsCount) != sizeof(Elf64_Hashelt))
					WriteLog(LL_Error, "could not read chains count");
				
				m_UserlandBuckets = l_UserlandHashTable + 2;
				m_UserlandChains = m_UserlandBuckets + m_BucketsCount;
				/*m_BucketsCount = l_HashTable[0];
				m_ChainsCount = l_HashTable[1];
				m_Buckets = l_HashTable + 2;
				m_Chains = m_Buckets + m_BucketsCount;*/
				break;
			}
			case DT_STRTAB:
				m_UserlandStringTable = static_cast<char*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr;
				break;
			case DT_STRSZ:
				m_StringTableSize = s_Dyn.d_un.d_val;
				break;
			case DT_SYMTAB:
				m_UserlandSymbolTable = reinterpret_cast<const Elf64_Sym*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);
				break;
			case DT_SYMENT:
				if (s_Dyn.d_un.d_val != sizeof(Elf64_Sym))
				{
					WriteLog(LL_Error, "elf64_sym size isn't correct in this elf wtf\n");

					return false;
				}
				break;
			case DT_PLTGOT:
				m_UserlandGlobalOffsetTable = reinterpret_cast<const Elf64_Addr*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);
				break;
			case DT_REL:
				m_UserlandRel = reinterpret_cast<const Elf64_Rel*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);
				break;
			case DT_RELSZ:
				m_RelSize = s_Dyn.d_un.d_val;
				break;
			case DT_RELENT:
				if (s_Dyn.d_un.d_val != sizeof(Elf64_Rel))
				{
					WriteLog(LL_Error, "elf64_rel size isn't the correct size in this elf\n");
					return false;
				}
				break;
			case DT_JMPREL:
				m_UserlandPltRel = reinterpret_cast<const Elf64_Rel*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);
				break;
			case DT_PLTRELSZ:
				m_PltRelSize = s_Dyn.d_un.d_val;
				break;
			case DT_RELA:
				m_UserlandRela = reinterpret_cast<const Elf64_Rela*>(static_cast<uint8_t*>(m_UserlandAllocatedMap) + s_Dyn.d_un.d_ptr);
				break;
			case DT_RELASZ:
				m_RelaSize = s_Dyn.d_un.d_val;
				break;
			case DT_RELAENT:
				if (s_Dyn.d_un.d_val != sizeof(Elf64_Rela))
				{
					WriteLog(LL_Error, "elf64_rela size isn't the correct size in this elf\n");
					return false;
				}
				break;
			case DT_PLTREL:
				s_PltType = static_cast<int>(s_Dyn.d_un.d_val);
				if (s_PltType != DT_REL ||
					s_PltType != DT_RELA)
				{
					WriteLog(LL_Error, "pltrel is not DT_REL or DT_RELA\n");
					return false;
				}
				break;
			case DT_DEBUG:
				// TODO: Implement if needed
				break;
		}

		s_DynIndex++;
	} while (s_Dyn.d_tag != DT_NULL);

	// Switch over if we are a rela type
	if (s_PltType == DT_RELA)
	{
		m_PltRela = reinterpret_cast<const Elf64_Rela*>(m_UserlandPltRel);
		m_UserlandPltRel = nullptr;
		m_PltRelaSize = m_PltRelSize;
		m_PltRelSize = 0;
	}

	m_DdbSymbolTable = m_UserlandSymbolTable;
	m_DdbSymbolCount = m_ChainsCount;
	m_DdbStringTable = m_UserlandStringTable;
	m_DdbStringCount = m_StringTableSize;

	return true;
}

bool Loader::ParseDpcpu()
{
	m_Debug = 0;
	m_PcpuBase = 0;
	m_PcpuStart = 0;
	m_PcpuStop = 0;

    WriteLog(LL_Warn, "ParseDpcpu not implemented.\n");

	return true;
}

const Elf64_Phdr * Loader::GetSourceProgramHeaderByIndex(uint32_t p_Index)
{
	if (m_SourceHeader == nullptr)
		return nullptr;

	if (p_Index >= m_SourceHeader->e_phnum)
		return nullptr;

	return m_SourceProgramHeadersStart + p_Index;
}

void* k_malloc(size_t size)
{
	if (!size)
		size = sizeof(uint64_t);
	
	auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
	vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));

	uint8_t* data = (uint8_t*)kmem_alloc(map, size);
	if (!data)
		return NULL;

	// Set our pointer header
	(*(uint64_t*)data) = size;

	// Return the start of the requested data
	return data + sizeof(uint64_t);
}

void k_free(void* address)
{
	if (!address)
		return;

	vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_free = (void(*)(void* map, void* addr, size_t size))kdlsym(kmem_free);

	
	uint8_t* data = ((uint8_t*)address) - sizeof(uint64_t);

	uint64_t size = *(uint64_t*)data;

	kmem_free(map, data, size);
}

void * Loader::Allocate(Elf64_Xword p_Size)
{
    void* s_Allocation = k_malloc(p_Size); //(void*)kmem_alloc(map, p_Size);

    //auto s_Allocation = new uint8_t[p_Size]; 

    // Zero allocation
    memset(s_Allocation, 0, p_Size);

    // return happy
    return s_Allocation;
}

void* Loader::AllocateUser(int p_ProcessId, Elf64_Xword p_Size)
{
    if (p_ProcessId < 0 || p_Size == 0)
        return nullptr;

    auto s_MainThread = GetMainThreadByPid(p_ProcessId);
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread for pid (%d)\n", p_ProcessId);
        return nullptr;
    }

    auto s_UserMap = kmmap_t(nullptr, p_Size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PREFAULT_READ, -1, 0, s_MainThread);
    if (s_UserMap == nullptr || s_UserMap == MAP_FAILED)
    {
        WriteLog(LL_Error, "could not allocate (%llx) bytes in pid (%d)\n", p_Size, p_ProcessId);
        return nullptr;
    }

    return static_cast<void*>(s_UserMap);
}

struct thread* Loader::GetMainThreadByPid(int32_t p_ProcessId)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);

    auto s_Process = pfind(p_ProcessId);
    if (s_Process == nullptr)
    {
        WriteLog(LL_Error, "could not find pid (%d)\n", p_ProcessId);
        return nullptr;
    }
    PROC_UNLOCK(s_Process);

    // Get the process main thread so we can allocate space within that process vmspace
    thread* s_MainThread = s_Process->p_singlethread != nullptr ? s_Process->p_singlethread : s_Process->p_threads.tqh_first;
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get pid (%d) main thread\n", p_ProcessId);
        return nullptr;
    }

    return s_MainThread;
}

void Loader::Free(void * p_Data)
{
#ifdef _WIN32
	free(p_Data);
#else
		k_free(p_Data);
#endif
}

uint32_t Loader::Hash(const char * p_Name)
{
	const unsigned char *p = (const unsigned char *)p_Name;
	unsigned long h = 0;
	unsigned long g;

	while (*p != '\0') {
		h = (h << 4) + *p++;
		if ((g = h & 0xf0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	}
	return (h);
}


uint32_t Loader::HashUser(const char * p_Name)
{
	unsigned char c = 0;
	unsigned char cc = 0;

	const unsigned char *p = (const unsigned char *)p_Name;
	unsigned long h = 0;
	unsigned long g;

	do
	{
		if (ReadProcessMemory(m_TargetProcessId, (void*)p, sizeof(c), &c) != sizeof(c))
			return (h);
		if (ReadProcessMemory(m_TargetProcessId, (void*)p++, sizeof(cc), &cc) != sizeof(c))
			return (h);

		h = (h << 4) + cc;
		if ((g = h & 0xf0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	} while (c != '\0');
	
	/*while (*p != '\0') {
		h = (h << 4) + *p++;
		if ((g = h & 0xf0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	}*/
	return (h);
}

bool Loader::Load()
{
	// Validate that our source header information is correct
	if (m_SourceElf == nullptr || m_SourceElfSize <= sizeof(Elf64_Ehdr))
		return false;

	// Save out our source information header
	m_SourceHeader = static_cast<const Elf64_Ehdr*>(m_SourceElf);

	// Validate the elf magic
	if (!IS_ELF(*m_SourceHeader))
	{
        WriteLog(LL_Error, "file is not elf\n");
		return false;
	}

	// Check that we are x64-little endian
	if (m_SourceHeader->e_ident[EI_CLASS] != ELFCLASS64 ||
		m_SourceHeader->e_ident[EI_DATA] != ELFDATA2LSB)
	{
        WriteLog(LL_Error, "unsupported file layout\n");
		return false;
	}

	// Validate elf version
	if (m_SourceHeader->e_ident[EI_VERSION] != EV_CURRENT)
	{
        WriteLog(LL_Error, "unsupported file version\n");
		return false;
	}

	// We only support executables and shared libraries
	if (m_SourceHeader->e_type != ET_EXEC && m_SourceHeader->e_type != ET_DYN && m_SourceHeader->e_type != ET_REL)
	{
        WriteLog(LL_Error, "non-executable elf format\n");
		return false;
	}

	// Validate correct machine arch
	if (m_SourceHeader->e_machine != EM_X86_64)
	{
        WriteLog(LL_Error, "unsupported arch type\n");
		return false;
	}

	// Validate that everthing is the correct sizes
	if (m_SourceHeader->e_phentsize != sizeof(Elf64_Phdr))
	{
        WriteLog(LL_Error, "invalid program header entry size\n");
		return false;
	}

	// Validate that all program headers are within bounds
	const auto s_TotalProgramHeaderSize = m_SourceHeader->e_phnum * m_SourceHeader->e_phentsize;
	if (m_SourceHeader->e_phoff >= m_SourceElfSize || m_SourceHeader->e_phoff + s_TotalProgramHeaderSize > m_SourceElfSize)
	{
        WriteLog(LL_Error, "corrupt elf program headers\n");
		return false;
	}

	// Update our loaders program header location
	m_SourceProgramHeadersStart = reinterpret_cast<const Elf64_Phdr*>(static_cast<const uint8_t*>(m_SourceElf) + m_SourceHeader->e_phoff);

	// Get all of the program headers
	for (auto l_ProgramHeaderIndex = 0; l_ProgramHeaderIndex < m_SourceHeader->e_phnum; ++l_ProgramHeaderIndex)
	{
		auto l_ProgramHeader = GetSourceProgramHeaderByIndex(l_ProgramHeaderIndex);
		if (l_ProgramHeader == nullptr)
		{
            WriteLog(LL_Error, "could not get program header (idx: %d).\n", l_ProgramHeaderIndex);
			continue;
		}
		
		switch (l_ProgramHeader->p_type)
		{
		case PT_LOAD:
			if (m_LoadableSegmentsCount == MaxSegments)
			{
                WriteLog(LL_Error, "too many loadable segments\n");
				return false;
			}

			// FreeBSD trusts that they are in correct order, so fuck it.
			m_LoadableSegments[m_LoadableSegmentsCount] = l_ProgramHeader;
			++m_LoadableSegmentsCount;
			break;
		case PT_PHDR:
			m_SourcePhphdr = l_ProgramHeader;
			break;
		case PT_DYNAMIC:
			m_SourceDynamicProgramHeader = l_ProgramHeader;
			break;
		case PT_INTERP:
			m_SourceInterpreterProgramHeader = l_ProgramHeader;
			break;
		}
	}

	// Validate that we actually got a dynamic program header
	if (m_SourceDynamicProgramHeader == nullptr)
	{
        WriteLog(LL_Error, "could not find dynamic program header\n");
		return false;
	}

	// Validate that we have any loadable segments
	if (m_LoadableSegmentsCount == 0 || m_LoadableSegmentsCount > MaxSegments)
	{
        WriteLog(LL_Error, "no loadable segments found\n");
		return false;
	}

	// We want to allocate the *entire* address space, so we have to calculate the min/max range
	Elf64_Addr s_VirtualAddressMinimum = __UINT64_MAX__;
	Elf64_Addr s_VirtualAddressMaximum = 0;

	for (Elf64_Xword l_LoadableIndex = 0; l_LoadableIndex < m_LoadableSegmentsCount; ++l_LoadableIndex)
	{
		auto l_ProgramHeader = m_LoadableSegments[l_LoadableIndex];
		if (l_ProgramHeader == nullptr)
		{
            WriteLog(LL_Error, "loadable program header (%lld) is invalid\n", l_LoadableIndex);
			continue;
		}

		// Skip any loadable segments without any file data
		if (l_ProgramHeader->p_filesz == 0)
			continue;

		// Check to see if this VA is before our min VA, update it if so
		if (l_ProgramHeader->p_vaddr < s_VirtualAddressMinimum)
			s_VirtualAddressMinimum = l_ProgramHeader->p_vaddr;

		if (l_ProgramHeader->p_vaddr + l_ProgramHeader->p_memsz > s_VirtualAddressMaximum)
			s_VirtualAddressMaximum = l_ProgramHeader->p_vaddr + l_ProgramHeader->p_memsz;
	}

	// Check to see if we got any loadable segments
	if (s_VirtualAddressMinimum == __UINT64_MAX__ || s_VirtualAddressMaximum == 0)
	{
        WriteLog(LL_Error, "could not calculate loadable range\n");
		return false;
	}

	// Get the total VA range size
	Elf64_Xword s_MapSize = s_VirtualAddressMaximum - s_VirtualAddressMinimum;

	// Allocate pre-zeroed map
	auto s_AllocatedMap = AllocateUser(m_TargetProcessId, s_MapSize);
	if (s_AllocatedMap == nullptr)
	{
        WriteLog(LL_Error, "could not allocate map\n");
		return false;
	}

	// Update our loaders maps
	m_UserlandAllocatedMap = s_AllocatedMap;
	m_AllocatedMapSize = s_MapSize;

	// Read the text, data, sections and zero bss
	for (Elf64_Xword l_LoadableSegmentIndex = 0; l_LoadableSegmentIndex < m_LoadableSegmentsCount; ++l_LoadableSegmentIndex)
	{
		auto l_LoadableSegment = m_LoadableSegments[l_LoadableSegmentIndex];
		if (l_LoadableSegment == nullptr)
		{
            WriteLog(LL_Error, "could not load loadable segment (%lld).\n", l_LoadableSegmentIndex);
			return false;
		}

		// Calculate where this loadable segment base is
		auto l_SegmentBase = static_cast<uint8_t*>(m_UserlandAllocatedMap) + (l_LoadableSegment->p_vaddr - s_VirtualAddressMinimum);
		// TODO: DO we need to check segment base?

		// Zero the entire segment base
		auto l_MemorySize = l_LoadableSegment->p_memsz;
		{
			auto l_ZeroAllocation = (uint8_t*)Allocate(l_MemorySize);
			if (l_ZeroAllocation == nullptr)
			{
				WriteLog(LL_Error, "could not allocate zero page.");
				return false;
			}
			(void)memset(l_ZeroAllocation, 0, l_MemorySize);

			if (WriteProcessMemory(m_TargetProcessId, l_SegmentBase, l_MemorySize, l_ZeroAllocation) != l_MemorySize)
				WriteLog(LL_Warn, "did not write the expected amount of bytes");
			
			delete [] l_ZeroAllocation;
		}
		//memset(l_SegmentBase, 0, l_LoadableSegment->p_memsz);

		// Copy over just the file size from source
		auto l_FileSize = l_LoadableSegment->p_filesz;
		if (WriteProcessMemory(m_TargetProcessId, l_SegmentBase, l_FileSize, ((uint8_t*)(m_SourceElf) + l_LoadableSegment->p_offset)) != l_FileSize)
			WriteLog(LL_Warn, "did not write the expected amount of bytes");
		//memcpy(l_SegmentBase, static_cast<const uint8_t*>(m_SourceElf) + l_LoadableSegment->p_offset, l_LoadableSegment->p_filesz);

		// Update permissions on this loadable segment

		// Calculate data protection starting from ---
		Elf64_Word l_DataProtection = 0;

#ifdef _WIN32
		/*if (l_LoadableSegment->p_flags & PF_R)
			l_DataProtection = PAGE_READONLY;
		if (l_LoadableSegment->p_flags & PF_W)
			l_DataProtection = PAGE_READWRITE;
		if (l_LoadableSegment->p_flags & PF_X)
			l_DataProtection = PAGE_EXECUTE_READ;*/
		l_DataProtection = PAGE_EXECUTE_READWRITE;
#else
		if (l_LoadableSegment->p_flags & PF_R)
			l_DataProtection |= PF_R;
		if (l_LoadableSegment->p_flags & PF_W)
			l_DataProtection |= PF_W;
		if (l_LoadableSegment->p_flags & PF_X)
			l_DataProtection |= PF_X;
#endif

		// Update the protection on this crap
		if (!SetUserProtection(m_TargetProcessId, reinterpret_cast<void*>(l_SegmentBase), l_LoadableSegment->p_memsz, l_DataProtection))
		{
            WriteLog(LL_Error, "could update memory protections\n");
			return false;
		}
	}

	// Save the dynamic location
	m_UserlandDynamic = reinterpret_cast<Elf64_Dyn*>(((uint8_t*)m_UserlandAllocatedMap) + (m_SourceDynamicProgramHeader->p_vaddr - s_VirtualAddressMinimum));

	// Parse the dynamic information
	if (!ParseDynamic())
	{
        WriteLog(LL_Error, "could not parse dynamics\n");
		return false;
	}

	// Parse dpcpu
	if (!ParseDpcpu())
	{
        WriteLog(LL_Error, "could not parse pcpu info\n");
		return false;
	}

	// TODO: Load dependencies
	if (!RelocateFile())
	{
        WriteLog(LL_Error, "could not elf reloc local\n");
		return false;
	}

	// Update the entrypoint
	m_UserlandEntrypoint = static_cast<uint8_t*>(m_UserlandAllocatedMap) + (m_SourceHeader->e_entry - s_VirtualAddressMinimum);
    WriteLog(LL_Info, "map: %p src: %p ep: %p", GetAllocatedMap(), m_SourceElf, m_UserlandEntrypoint);

	// TODO: Attempt to load symbol table (if it fucking exists, fuck you)
	auto s_SectionHeaderSize = m_SourceHeader->e_shnum * m_SourceHeader->e_shentsize;
	if (s_SectionHeaderSize == 0 || m_SourceHeader->e_shoff == 0 || m_SourceHeader->e_shoff + s_SectionHeaderSize > m_SourceElfSize)
	{
        WriteLog(LL_Warn, "no symbol information exists\n");
		return true; // Ensure we return true here
	}

	auto s_SectionHeaderData = static_cast<Elf64_Shdr*>(Allocate(s_SectionHeaderSize));
	if (s_SectionHeaderData == nullptr)
	{
        WriteLog(LL_Error, "could not allocate section header data\n");
		return true; // Elf is still ready to go
	}

	// Copy the table information
	memcpy(s_SectionHeaderData, static_cast<const uint8_t*>(m_SourceElf) + m_SourceHeader->e_shoff, s_SectionHeaderSize);


	// Find the symbol table index, and the string table index
	auto s_SymTabIndex = -1;
	auto s_SymStrIndex = -1;

	// Yerrrrrrr
	for (auto i = 0; i < m_SourceHeader->e_shnum; ++i)
	{
		if (s_SectionHeaderData[i].sh_type == SHT_SYMTAB)
		{
			s_SymTabIndex = i;
			s_SymStrIndex = s_SectionHeaderData[i].sh_link;
		}
	}

	if (s_SymTabIndex < 0 || s_SymStrIndex < 0)
	{
        WriteLog(LL_Warn, "could not find symbol table index (%d) or symbol string table index (%d)", s_SymTabIndex, s_SymStrIndex);
		
		// Don't forget to free memory
		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

	// Allocate our symbol table
	m_SymbolTableSize = s_SectionHeaderData[s_SymTabIndex].sh_size;
	m_UserlandSymbolTableBase = reinterpret_cast<Elf64_Addr>(AllocateUser(m_TargetProcessId, m_SymbolTableSize));
	if (m_UserlandSymbolTableBase == reinterpret_cast<Elf64_Addr>(nullptr))
	{
		m_SymbolTableSize = 0;
        WriteLog(LL_Error, "could not allocate symbol table (sz: 0x%llx)", m_SymbolTableSize);
		
		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

    WriteLog(LL_Info, "copying data from (%p 0x%llx) -> (%p 0x%llx)", static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymTabIndex].sh_offset, m_SymbolTableSize, reinterpret_cast<void*>(m_UserlandSymbolTableBase), m_SymbolTableSize);

	memcpy(reinterpret_cast<void*>(m_UserlandSymbolTableBase), static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymTabIndex].sh_offset, m_SymbolTableSize);

	m_StringTableBaseSize = s_SectionHeaderData[s_SymStrIndex].sh_size;
	m_StringTableBase = reinterpret_cast<Elf64_Addr>(Allocate(m_StringTableBaseSize));
	if (m_StringTableBase == reinterpret_cast<Elf64_Addr>(nullptr))
	{
		Free(reinterpret_cast<void*>(m_UserlandSymbolTableBase));
		m_UserlandSymbolTableBase = 0;

		m_SymbolTableSize = 0;
        WriteLog(LL_Error, "could not allocate string table (sz: 0x%llx)", m_StringTableBaseSize);
		m_StringTableBaseSize = 0;

		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

    WriteLog(LL_Info, "2 copying data from (%p 0x%llx) -> (%p 0x%llx)", static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymStrIndex].sh_offset, m_StringTableBaseSize, reinterpret_cast<void*>(m_StringTableBase), m_StringTableBaseSize);
	
	memcpy(reinterpret_cast<void*>(m_StringTableBase), static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymStrIndex].sh_offset, m_StringTableBaseSize);

	m_DdbSymbolCount = m_SymbolTableSize / sizeof(Elf64_Sym);
	m_DdbSymbolTable = (const Elf64_Sym*)m_UserlandSymbolTableBase;
	m_DdbStringCount = m_StringTableBaseSize;
	m_DdbStringTable = reinterpret_cast<const char*>(m_StringTableBase);

	if (s_SectionHeaderData != nullptr)
	{
		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;
		//s_SectionHeaderSize = 0;
	}

	return true;
}

int Loader::Strcmp(const char* s1, const char* s2)
{
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (0);
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

int Loader::StrcmpUser(const char* s1, const char* s2)
{
	char s11 = 0;
	char s22 = 0;
	do
	{
		
		if (ReadProcessMemory(m_TargetProcessId, (void*)s1, sizeof(s11), &s11) != sizeof(s11))
		{
			WriteLog(LL_Error, "could not strcmp (%p)", s1);
			break;
		}

		if (ReadProcessMemory(m_TargetProcessId, (void*)s2, sizeof(s22), &s22) != sizeof(s22))
		{
			WriteLog(LL_Error, "could not strcmp (%p)", s2);
			break;
		}

		auto s_Ret = s1++;
		char s111 = 0;
		if (ReadProcessMemory(m_TargetProcessId, (void*)s_Ret, sizeof(s111), &s111) != sizeof(s111))
		{
			WriteLog(LL_Error, "could not strcmp (%p)", s_Ret);
			break;
		}

		if (s111 == '\0')
			return (0);

	} while (s11 == s22);
	
	/*while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (0);*/
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

uint32_t Loader::ReadProcessMemory(int32_t p_Process, void* p_Address, uint32_t p_Size, void* p_OutputBuffer)
{
	// Validate that we have a valid process
	if (p_Process < 0)
		return 0;
	
	// Check if the buffer is valid
	if (p_OutputBuffer == nullptr)
	{
		WriteLog(LL_Error, "invalid output buffer");
		return 0;
	}

	// Zero out the output buffer
	(void)memset(p_OutputBuffer, 0, p_Size);

	// Validate that the process exists
	if (!IsProcessAlive(p_Process))
		return 0;
	
	size_t s_OutputSize = p_Size;
	size_t s_Ret = proc_rw_mem_pid(p_Process, p_Address, p_Size, p_OutputBuffer, &s_OutputSize, false);
	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "proc_rw_mem_pid ret: (%d) could not read process (%d) memory (%p) size: (%x) outputSize: (%llx)", s_Ret, p_Process, p_Address, p_Size, s_OutputSize);
		return s_OutputSize;
	}

	return s_OutputSize;
}

uint32_t Loader::WriteProcessMemory(int32_t p_Process, void* p_Address, uint32_t p_Size, void* p_InputBuffer)
{
	if (p_Process < 0)
		return 0;

	if (p_InputBuffer == nullptr)
	{
		WriteLog(LL_Error, "invalid input buffer");
		return 0;
	}
	(void)memset(p_InputBuffer, 0, p_Size);

	if (!IsProcessAlive(p_Process))
	{
		WriteLog(LL_Error, "process (%d) is not alive.", p_Process);
		return 0;
	}
	
	size_t s_InputSize = p_Size;
	auto s_Ret = proc_rw_mem_pid(p_Process, p_Address, p_Size, p_InputBuffer, &s_InputSize,  true);
	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "proc_rw_mem_pid ret: (%d) could not read process (%d) memory (%p) size: (%x) inputSize: (%llx)", s_Ret, p_Process, p_Address, p_Size, s_InputSize);
		return 0;
	}

	return s_InputSize;
}

bool Loader::IsProcessAlive(int32_t p_ProcessId)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	
    struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		return false;
	}
	PROC_UNLOCK(s_Process);
	return true;
}