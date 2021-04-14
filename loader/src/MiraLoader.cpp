// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "MiraLoader.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#else
#include <Boot/MiraLoader.hpp>
//#include <Utils/New.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>

#include <sys/mman.h>
using namespace MiraLoader;

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

Loader::Loader(const void* p_Elf, uint32_t p_ElfSize, ElfLoaderType_t p_Type) :
	m_SourceElf(p_Elf),
	m_SourceElfSize(p_ElfSize),
	m_LoaderType(p_Type),
	m_SourceHeader(nullptr),
	m_SourceProgramHeadersStart(nullptr),
	m_LoadableSegments{ 0 },
	m_LoadableSegmentsCount(0),
	m_SourcePhphdr(nullptr),
	m_SourceDynamicProgramHeader(nullptr),
	m_SourceInterpreterProgramHeader(nullptr),
	m_AllocatedMap(nullptr),
	m_AllocatedMapSize(0),
	m_Dynamic(nullptr),
	m_BucketsCount(0),
	m_ChainsCount(0),
	m_Buckets(nullptr),
	m_Chains(nullptr),
	m_StringTable(nullptr),
	m_StringTableSize(0),
	m_SymbolTable(nullptr),
	m_GlobalOffsetTable(nullptr),
	m_Rel(nullptr),
	m_RelSize(0),
	m_PltRel(nullptr),
	m_PltRelSize(0),
	m_PltRela(nullptr),
	m_PltRelaSize(0),
	m_Rela(nullptr),
	m_RelaSize(0),
	m_Debug(0),
	m_DdbSymbolTable(nullptr),
	m_DdbSymbolCount(0),
	m_DdbStringTable(nullptr),
	m_DdbStringCount(0),
	m_PcpuStart(0),
	m_PcpuStop(0),
	m_PcpuBase(0),
	m_SymbolTableBase(0),
	m_SymbolTableSize(0),
	m_StringTableBase(0),
	m_StringTableBaseSize(0),
	m_Entrypoint(nullptr)
{
	if (!Load())
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not load properly\n");
		}
		else
			WriteNotificationLog("err: could not load properly\n");
	}
		
}


Loader::~Loader()
{
}

#ifdef _WIN32
void Loader::DumpLoadedElf(const char * p_Path)
{
	FILE* s_Handle = nullptr;
	auto s_Ret = fopen_s(&s_Handle, p_Path, "wb");
	if (s_Ret != ERROR_SUCCESS)
	{
		printf("err: could not write output file (%s) (0x%x)\n", p_Path, s_Ret);
		return;
	}

	if (fwrite(m_AllocatedMap, sizeof(uint8_t), m_AllocatedMapSize, s_Handle) < 0)
	{
		fclose(s_Handle);
		s_Handle = nullptr;

		printf("err: could not write output file\n");
		return;
	}

	fclose(s_Handle);
	s_Handle = nullptr;

	printf("info: elf dumped successfully\n");
}
#endif

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
	Elf64_Addr* s_Where, s_Val;
	uint32_t* s_Where32, s_Val32;
	Elf64_Addr s_Addr;
	Elf64_Addr s_Addend;
	Elf64_Size s_Type, s_SymbolIndex;

	const Elf64_Rel* s_Rel;
	const Elf64_Rela* s_Rela;

	switch (p_RelocationType)
	{
	case ELF_RELOC_REL:
	{
		s_Rel = static_cast<const Elf64_Rel*>(p_Data);
		s_Where = reinterpret_cast<Elf64_Addr*>(p_RelocationBase + s_Rel->r_offset);

		s_Type = ELF64_R_TYPE(s_Rel->r_info);
		s_SymbolIndex = ELF64_R_SYM(s_Rel->r_info);

		switch (s_Type)
		{
			/* Addend is 32 bit on 32 bit relocs */
		case R_X86_64_PC32:
		case R_X86_64_32S:
			s_Addend = *(Elf64_Addr*)s_Where;
			break;
		default:
			s_Addend = *s_Where;
			break;
		}
		break;
	}
	case ELF_RELOC_RELA:
	{
		s_Rela = static_cast<const Elf64_Rela*>(p_Data);
		s_Where = (Elf64_Addr*)(p_RelocationBase + s_Rela->r_offset);
		s_Addend = s_Rela->r_addend;
		s_Type = ELF64_R_TYPE(s_Rela->r_info);
		s_SymbolIndex = ELF64_R_SYM(s_Rela->r_info);
		break;
	}
	default:
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: unknown reloc type %d\n", p_RelocationType);
		}
		else
			WriteNotificationLog("err: unknown reloc type\n");
		
		return false;
	}

	switch (s_Type)
	{
	case R_X86_64_NONE:	/* none */
		break;

	case R_X86_64_64:		/* S + A */
		s_Addr = Lookup(s_SymbolIndex, true);
		s_Val = s_Addr + s_Addend;
		if (s_Addr == 0)
			return false;
		if (*s_Where != s_Val)
			*s_Where = s_Val;
		break;

	case R_X86_64_PC32:	/* S + A - P */
		s_Addr = Lookup(s_SymbolIndex, true);
		s_Where32 = (uint32_t *)s_Where;
		s_Val32 = (uint32_t)(s_Addr + s_Addend - (Elf64_Addr)s_Where);
		if (s_Addr == 0)
			return false;
		if (*s_Where32 != s_Val32)
			*s_Where32 = s_Val32;
		break;

	case R_X86_64_32S:	/* S + A sign extend */
		s_Addr = Lookup(s_SymbolIndex, true);
		s_Val32 = (uint32_t)(s_Addr + s_Addend);
		s_Where32 = (uint32_t *)s_Where;
		if (s_Addr == 0)
			return false;
		if (*s_Where32 != s_Val32)
			*s_Where32 = s_Val32;
		break;

	case R_X86_64_COPY:	/* none */
		/*
		 * There shouldn't be copy relocations in kernel
		 * objects.
		 */
		
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("kldload: unexpected R_COPY relocation\n");
		}
		else
			WriteNotificationLog("kldload: unexpected R_COPY relocation\n");
		
		return false;

	case R_X86_64_GLOB_DAT:	/* S */
	case R_X86_64_JMP_SLOT:	/* XXX need addend + offset */
		s_Addr = Lookup(s_SymbolIndex, true);
		if (s_Addr == 0)
			return false;
		if (*s_Where != s_Addr)
			*s_Where = s_Addr;
		break;

	case R_X86_64_RELATIVE:	/* B + A */
		s_Addr = p_RelocationBase + s_Addend;
		s_Val = s_Addr;
		if (*s_Where != s_Val)
			*s_Where = s_Val;
		break;

	default:
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("kldload: unexpected relocation type %lld\n",
			s_Type);
		}
		else
			WriteNotificationLog("kldload: unexpected relocation type\n");
		
		return false;
	}

	return true;
}

Elf64_Addr Loader::Lookup(Elf64_Size p_SymbolIndex, bool p_CheckDependencies)
{
	if (p_SymbolIndex >= m_ChainsCount)
		return 0;

	const Elf64_Sym* s_Symbol = m_SymbolTable + p_SymbolIndex;
	const char* s_SymbolName = "";

	// Skip doing a full lookup when the symbol is local, it may even fail because it may not be found through hash tables
	if (ELF64_ST_BIND(s_Symbol->st_info) == STB_LOCAL)
	{
		// FreeBSD dev's are smoking crack wtf
		if (s_Symbol->st_shndx == SHN_UNDEF || s_Symbol->st_value == 0)
			return 0;
		return reinterpret_cast<Elf64_Addr>(m_AllocatedMap) + s_Symbol->st_value;
	}

	s_SymbolName = m_StringTable + s_Symbol->st_name;
	if (*s_SymbolName == 0)
		return 0;


	// TODO: Implement
	//const auto s_Symbol2 = LinkerFileLookupSymbol(s_SymbolName, p_CheckDependencies);
	
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("err: Lookup not implemented\n");
	}
	else
		WriteNotificationLog("err: Lookup not implemented\n");
	return 0;
}

Elf64_Addr Loader::LinkerFileLookupSymbol(const char * p_Name, bool p_CheckDependencies)
{
	// TODO: Implement
	//Elf64_Sym* s_Symbol = LinkerFileLookupSymbolInternal(p_Name, true);

	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("err: LinkerFileLookupSymbol not implemented\n");
	}
	else
		WriteNotificationLog("err: LinkerFileLookupSymbol not implemented\n");
	return 0;
}

Elf64_Sym* Loader::LinkerFileLookupSymbolInternal(const char * p_Name, bool p_CheckDependencies)
{
	if (m_Buckets == nullptr || m_BucketsCount == 0)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: LinkerFileLookupSymbolInternal: missing symbol hash table\n");
		}
		else
			WriteNotificationLog("err: LinkerFileLookupSymbolInternal: missing symbol hash table\n");
		
		return nullptr;
	}

	const Elf64_Sym* s_Symp = nullptr;
	const char* s_Strp = "";
	auto s_Hash = Hash(p_Name);
	auto s_SymbolNum = m_Buckets[s_Hash % m_BucketsCount];

	while (s_SymbolNum != STN_UNDEF)
	{
		if (s_SymbolNum >= m_ChainsCount)
		{
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: corrupt symbol table\n");
			}
			else
				WriteNotificationLog("err: corrupt symbol table\n");
			return nullptr;
		}

		s_Symp = m_SymbolTable + s_SymbolNum;
		if (s_Symp->st_name == 0)
		{
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: corrupt symbol table\n");
			}
			else
				WriteNotificationLog("err: corrupt symbol table\n");
			return nullptr;
		}

		s_Strp = m_StringTable + s_Symp->st_name;
		if (Strcmp(p_Name, s_Strp) == 0)
		{
			if (s_Symp->st_shndx != SHN_UNDEF ||
				(s_Symp->st_value != 0 && ELF64_ST_TYPE(s_Symp->st_info) == STT_FUNC))
			{
				return const_cast<Elf64_Sym*>(s_Symp);
			}

			return nullptr;
		}

		s_SymbolNum = m_Chains[s_SymbolNum];
	}

	// If we have not found it, look at the full table (if loaded)
	if (m_SymbolTable == m_DdbSymbolTable)
		return nullptr;

	// Exhaustive search
	
	for (auto i = 0; i < m_DdbSymbolCount; i++)
	{
		s_Symp = m_DdbSymbolTable + i;
		s_Strp = m_DdbStringTable + s_Symp->st_name;
		if (Strcmp(p_Name, s_Strp) == 0)
		{
			if (s_Symp->st_shndx != SHN_UNDEF ||
				(s_Symp->st_value != 0 && ELF64_ST_TYPE(s_Symp->st_info) == STT_FUNC))
			{
				return const_cast<Elf64_Sym*>(s_Symp);
			}
			return nullptr;
		}
	}

	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("err: symbol not found\n");
	}
	else
		WriteNotificationLog("err: symbol not found\n");
	return nullptr;
}

bool Loader::RelocateFile()
{
	const Elf64_Rel* s_RelLimit = nullptr;
	const Elf64_Rel* s_Rel = nullptr;
	const Elf64_Rela* s_RelaLimit = nullptr;
	const Elf64_Rela* s_Rela = nullptr;

	// Perform relocations without addend if there are any
	if ((s_Rel = m_Rel) != nullptr)
	{
		s_RelLimit = reinterpret_cast<const Elf64_Rel*>(reinterpret_cast<const uint8_t*>(m_Rel) + m_RelSize);
		while (s_Rel < s_RelLimit)
		{
			if (!ElfReloc(reinterpret_cast<Elf64_Addr>(m_AllocatedMap), s_Rel, ELF_RELOC_REL))
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("warn: could not elf reloc local for rel's\n");
				}
				else
					WriteNotificationLog("warn: could not elf reloc local for rel's\n");
			}

			s_Rel++;
		}
	}

	// Perform relocations with addend if there are any
	if ((s_Rela = m_Rela) != nullptr)
	{
		s_RelaLimit = reinterpret_cast<const Elf64_Rela*>(reinterpret_cast<const uint8_t*>(m_Rela) + m_RelaSize);
		while (s_Rela < s_RelaLimit)
		{
			if (!ElfReloc(reinterpret_cast<Elf64_Addr>(m_AllocatedMap), s_Rela, ELF_RELOC_RELA))
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("warn: could not elf reloc local for rela's\n");
				}
				else
					WriteNotificationLog("warn: could not elf reloc local for rela's\n");
			}
				

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
		printf("err: VirtualProtect failed (%d)\n", s_Error);
		return false;
	}
#else
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("pmap_protect: %p %llx %x\n", p_Address, p_Size, p_Protection);

		/*auto pmap_protect = (void(*)(pmap_t, vm_offset_t, vm_offset_t, vm_prot_t))kdlsym(pmap_protect);

		// TODO: pmap_protect
		uint64_t s_StartAddress = ((uint64_t)p_Address) & ~(uint64_t)(PAGE_SIZE - 1);
		uint64_t s_EndAddress = ((uint64_t)p_Address + p_Size + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);

		WriteLog(LL_Debug, "pmap_protect: %p %llx %x", p_Address, p_Size, p_Protection);*/
	}
	else if (m_LoaderType == ElfLoaderType_t::UserProc ||
		m_LoaderType == ElfLoaderType_t::UserTrainer)
	{
		if ((int64_t)syscall3(SYS_MPROTECT, p_Address, (void*)p_Size, (void*)(int64_t)p_Protection) < 0)
		{
			WriteNotificationLog("mprotect error\n");
			return false;
		}
	}
#endif

	return true;
}

bool Loader::ParseDynamic()
{
	if (m_Dynamic == nullptr)
		return false;

	auto s_PltType = DT_REL;

	// TODO: More bounds checking
	for (Elf64_Dyn* l_Dynamic = m_Dynamic; l_Dynamic->d_tag != DT_NULL; l_Dynamic++)
	{
		switch (l_Dynamic->d_tag)
		{
		case DT_HASH:
		{
			// Stole from: src/libexec/rtld-elf/rtld.c
			const Elf64_Hashelt* l_HashTable = reinterpret_cast<const Elf64_Hashelt*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);

			m_BucketsCount = l_HashTable[0];
			m_ChainsCount = l_HashTable[1];
			m_Buckets = l_HashTable + 2;
			m_Chains = m_Buckets + m_BucketsCount;
			break;
		}
		case DT_STRTAB:
			m_StringTable = static_cast<char*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr;
			break;
		case DT_STRSZ:
			m_StringTableSize = l_Dynamic->d_un.d_val;
			break;
		case DT_SYMTAB:
			m_SymbolTable = reinterpret_cast<const Elf64_Sym*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);
			break;
		case DT_SYMENT:
			if (l_Dynamic->d_un.d_val != sizeof(Elf64_Sym))
			{	
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("err: elf64_sym size isn't correct in this elf wtf\n");
				}
				else
					WriteNotificationLog("err: elf64_sym size isn't correct in this elf wtf\n");
				
				return false;
			}
			break;
		case DT_PLTGOT:
			m_GlobalOffsetTable = reinterpret_cast<const Elf64_Addr*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);
			break;
		case DT_REL:
			m_Rel = reinterpret_cast<const Elf64_Rel*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);
			break;
		case DT_RELSZ:
			m_RelSize = l_Dynamic->d_un.d_val;
			break;
		case DT_RELENT:
			if (l_Dynamic->d_un.d_val != sizeof(Elf64_Rel))
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("err: elf64_rel size isn't the correct size in this elf\n");
				}
				else
					WriteNotificationLog("err: elf64_rel size isn't the correct size in this elf\n");
				return false;
			}
			break;
		case DT_JMPREL:
			m_PltRel = reinterpret_cast<const Elf64_Rel*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);
			break;
		case DT_PLTRELSZ:
			m_PltRelSize = l_Dynamic->d_un.d_val;
			break;
		case DT_RELA:
			m_Rela = reinterpret_cast<const Elf64_Rela*>(static_cast<uint8_t*>(m_AllocatedMap) + l_Dynamic->d_un.d_ptr);
			break;
		case DT_RELASZ:
			m_RelaSize = l_Dynamic->d_un.d_val;
			break;
		case DT_RELAENT:
			if (l_Dynamic->d_un.d_val != sizeof(Elf64_Rela))
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("err: elf64_rela size isn't the correct size in this elf\n");
				}
				else
					WriteNotificationLog("err: elf64_rela size isn't the correct size in this elf\n");
				return false;
			}
			break;
		case DT_PLTREL:
			s_PltType = static_cast<int>(l_Dynamic->d_un.d_val);
			if (s_PltType != DT_REL ||
				s_PltType != DT_RELA)
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("err: pltrel is not DT_REL or DT_RELA\n");
				}
				else
					WriteNotificationLog("err: pltrel is not DT_REL or DT_RELA\n");
				return false;
			}
			break;
		case DT_DEBUG:
			// TODO: Implement if needed
			break;
		}
	}

	// Switch over if we are a rela type
	if (s_PltType == DT_RELA)
	{
		m_PltRela = reinterpret_cast<const Elf64_Rela*>(m_PltRel);
		m_PltRel = nullptr;
		m_PltRelaSize = m_PltRelSize;
		m_PltRelSize = 0;
	}

	m_DdbSymbolTable = m_SymbolTable;
	m_DdbSymbolCount = m_ChainsCount;
	m_DdbStringTable = m_StringTable;
	m_DdbStringCount = m_StringTableSize;

	return true;
}

bool Loader::ParseDpcpu()
{
	m_Debug = 0;
	m_PcpuBase = 0;
	m_PcpuStart = 0;
	m_PcpuStop = 0;

	// TODO: Implement
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("warn: ParseDpcpu not implemented.\n");
	}
	else
		WriteNotificationLog("warn: ParseDpcpu not implemented.\n");
	return true;
}

const Elf64_Phdr * Loader::GetProgramHeaderByIndex(uint32_t p_Index)
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
	
	auto kmem_alloc = (vm_offset_t(*)(struct vm_map * map, vm_size_t size))kdlsym(kmem_alloc);
	struct vm_map * map = (struct vm_map *)(*(uint64_t *)(kdlsym(kernel_map)));

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

	struct vm_map * map = (struct vm_map *)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_free = (void(*)(void* map, void* addr, size_t size))kdlsym(kmem_free);

	
	uint8_t* data = ((uint8_t*)address) - sizeof(uint64_t);

	uint64_t size = *(uint64_t*)data;

	kmem_free(map, data, size);
}

void * Loader::Allocate(Elf64_Xword p_Size)
{
	// Validate that loader type is valid
	if (m_LoaderType <= ElfLoaderType_t::Invalid ||
		m_LoaderType >= ElfLoaderType_t::MAX)
		return nullptr;

#ifdef _WIN32
	auto s_Allocation = malloc(p_Size);
	if (s_Allocation == nullptr)
		return nullptr;

	// Zero allocation
	memset(s_Allocation, 0, p_Size);
	return s_Allocation;
#else
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		//auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
		//vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));

		void* s_Allocation = k_malloc(p_Size); //(void*)kmem_alloc(map, p_Size);

		//auto s_Allocation = new uint8_t[p_Size]; 

		// Zero allocation
		memset(s_Allocation, 0, p_Size);

		// return happy
		return s_Allocation;
	}
	else if (m_LoaderType == ElfLoaderType_t::UserProc ||
		m_LoaderType == ElfLoaderType_t::UserTrainer)
	{
		// Get allocation
		auto s_AllocationData = _mmap(NULL, p_Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (s_AllocationData == nullptr)
			return nullptr;
		
		memset(s_AllocationData, 0, p_Size);

		return s_AllocationData;
	}
#endif

	return nullptr;
}

void Loader::Free(void * p_Data)
{
	// Validate that loader type is valid
	if (m_LoaderType <= ElfLoaderType_t::Invalid ||
		m_LoaderType >= ElfLoaderType_t::MAX)
		return;

#ifdef _WIN32
	free(p_Data);
#else
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		k_free(p_Data);
		//delete [] static_cast<uint8_t*>(p_Data);
	}
	else if (m_LoaderType == ElfLoaderType_t::UserProc ||
		m_LoaderType == ElfLoaderType_t::UserTrainer)
	{
		// TODO: unmap
		WriteNotificationLog("err: Free for userland not implemented\n");
		return;
	}
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

bool Loader::CheckKernelElf(const void* p_Elf, uint32_t p_ElfSize)
{
	if (p_Elf == nullptr || p_ElfSize < sizeof(Elf64_Ehdr))
		return false;
	
	auto s_ElfHeader = static_cast<const Elf64_Ehdr*>(p_Elf);
	if (!IS_ELF(*s_ElfHeader))
		return false;
	
	// Check that we are x64-little endian
	if (s_ElfHeader->e_ident[EI_CLASS] != ELFCLASS64 ||
		s_ElfHeader->e_ident[EI_DATA] != ELFDATA2LSB)
		return false;

	// Validate elf version
	if (s_ElfHeader->e_ident[EI_VERSION] != EV_CURRENT)
		return false;
	
	// We only support executables and shared libraries
	if (s_ElfHeader->e_type != ET_EXEC && s_ElfHeader->e_type != ET_DYN && s_ElfHeader->e_type != ET_REL)
		return false;
	
	// Validate correct machine arch
	if (s_ElfHeader->e_machine != EM_X86_64)
		return false;
	
	// Validate that everthing is the correct sizes
	if (s_ElfHeader->e_phentsize != sizeof(Elf64_Phdr))
		return false;
	
	// Validate that all program headers are within bounds
	const auto s_TotalProgramHeaderSize = s_ElfHeader->e_phnum * s_ElfHeader->e_phentsize;
	if (s_ElfHeader->e_phoff >= p_ElfSize || s_ElfHeader->e_phoff + s_TotalProgramHeaderSize > p_ElfSize)
		return false;
	
	// Update our loaders program header location
	auto s_SourceProgramHeadersStart = reinterpret_cast<const Elf64_Phdr*>(static_cast<const uint8_t*>(p_Elf) + s_ElfHeader->e_phoff);

	// Get all of the program headers
	for (auto l_ProgramHeaderIndex = 0; l_ProgramHeaderIndex < s_ElfHeader->e_phnum; ++l_ProgramHeaderIndex)
	{
		auto l_ProgramHeader = s_SourceProgramHeadersStart + l_ProgramHeaderIndex;
		if (!l_ProgramHeader)
			continue;
		
		// We create a empty segment inside of the mira elf for .kern, it's PT_INTERP
		if (l_ProgramHeader->p_type != PT_INTERP)
			continue;
		
		// Don't read beyond the bounds of the data we have
		if (l_ProgramHeader->p_offset >= (p_ElfSize - 4))
			continue;

		// Check that this has enough data for us to read
		if (l_ProgramHeader->p_filesz < 4)
			continue;
		
		// Get the kernel string location
		const char* kernString = static_cast<const char*>(p_Elf) + l_ProgramHeader->p_offset;

		// Check that this equals kern
		if (kernString[0] != 'k' ||
			kernString[1] != 'e' ||
			kernString[2] != 'r' ||
			kernString[3] != 'n')
			continue;
		
		// It does, we are a kernel elf
		return true;
	}

	return false;
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
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: file is not elf\n");
		}
		else
			WriteNotificationLog("err: file is not elf\n");
		return false;
	}

	// Check that we are x64-little endian
	if (m_SourceHeader->e_ident[EI_CLASS] != ELFCLASS64 ||
		m_SourceHeader->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: unsupported file layout\n");
		}
		else
			WriteNotificationLog("err: unsupported file layout\n");
		return false;
	}

	// Validate elf version
	if (m_SourceHeader->e_ident[EI_VERSION] != EV_CURRENT)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: unsupported file version\n");
		}
		else
			WriteNotificationLog("err: unsupported file version\n");
		return false;
	}

	// We only support executables and shared libraries
	if (m_SourceHeader->e_type != ET_EXEC && m_SourceHeader->e_type != ET_DYN && m_SourceHeader->e_type != ET_REL)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: non-executable elf format\n");
		}
		else
			WriteNotificationLog("err: non-executable elf format\n");
		return false;
	}

	// Validate correct machine arch
	if (m_SourceHeader->e_machine != EM_X86_64)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: unsupported arch type\n");
		}
		else
			WriteNotificationLog("err: unsupported arch type\n");
		return false;
	}

	// Validate that everthing is the correct sizes
	if (m_SourceHeader->e_phentsize != sizeof(Elf64_Phdr))
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: invalid program header entry size\n");
		}
		else
			WriteNotificationLog("err: invalid program header entry size\n");
		return false;
	}

	// Validate that all program headers are within bounds
	const auto s_TotalProgramHeaderSize = m_SourceHeader->e_phnum * m_SourceHeader->e_phentsize;
	if (m_SourceHeader->e_phoff >= m_SourceElfSize || m_SourceHeader->e_phoff + s_TotalProgramHeaderSize > m_SourceElfSize)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: corrupt elf program headers\n");
		}
		else
			WriteNotificationLog("err: corrupt elf program headers\n");
		return false;
	}

	// Update our loaders program header location
	m_SourceProgramHeadersStart = reinterpret_cast<const Elf64_Phdr*>(static_cast<const uint8_t*>(m_SourceElf) + m_SourceHeader->e_phoff);

	// Get all of the program headers
	for (auto l_ProgramHeaderIndex = 0; l_ProgramHeaderIndex < m_SourceHeader->e_phnum; ++l_ProgramHeaderIndex)
	{
		auto l_ProgramHeader = GetProgramHeaderByIndex(l_ProgramHeaderIndex);
		if (l_ProgramHeader == nullptr)
		{
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: could not get program header (idx: %d).\n", l_ProgramHeaderIndex);
			}
			else
				WriteNotificationLog("err: could not get program header\n");
			continue;
		}
		
		switch (l_ProgramHeader->p_type)
		{
		case PT_LOAD:
			if (m_LoadableSegmentsCount == MaxSegments)
			{
				if (m_LoaderType == ElfLoaderType_t::KernelProc)
				{
					auto printf = (void(*)(const char *format, ...))kdlsym(printf);
					printf("err: too many loadable segments\n");
				}
				else
					WriteNotificationLog("err: too many loadable segments\n");
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
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not find dynamic program header\n");
		}
		else
			WriteNotificationLog("err: could not find dynamic program header\n");
		return false;
	}

	// Validate that we have any loadable segments
	if (m_LoadableSegmentsCount == 0 || m_LoadableSegmentsCount > MaxSegments)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: no loadable segments found\n");
		}
		else
			WriteNotificationLog("err: no loadable segments found\n");
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
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: loadable program header (%lld) is invalid\n", l_LoadableIndex);
			}
			else
				WriteNotificationLog("err: loadable program header\n");
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
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not calculate loadable range\n");
		}
		else
			WriteNotificationLog("err: could not calculate loadable range\n");
		return false;
	}

	// Get the total VA range size
	Elf64_Xword s_MapSize = s_VirtualAddressMaximum - s_VirtualAddressMinimum;

	// Allocate pre-zeroed map
	auto s_AllocatedMap = Allocate(s_MapSize);
	if (s_AllocatedMap == nullptr)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not allocate map\n");
		}
		else
			WriteNotificationLog("err: could not allocate map\n");
		return false;
	}

	// Update our loaders maps
	m_AllocatedMap = s_AllocatedMap;
	m_AllocatedMapSize = s_MapSize;

	// Read the text, data, sections and zero bss
	for (Elf64_Xword l_LoadableSegmentIndex = 0; l_LoadableSegmentIndex < m_LoadableSegmentsCount; ++l_LoadableSegmentIndex)
	{
		auto l_LoadableSegment = m_LoadableSegments[l_LoadableSegmentIndex];
		if (l_LoadableSegment == nullptr)
		{
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: could not load loadable segment (%lld).\n", l_LoadableSegmentIndex);
			}
			else
				WriteNotificationLog("err: could not load loadable segment\n");
			return false;
		}

		// Calculate where this loadable segment base is
		auto l_SegmentBase = static_cast<uint8_t*>(m_AllocatedMap) + (l_LoadableSegment->p_vaddr - s_VirtualAddressMinimum);
		// TODO: DO we need to check segment base?

		// Zero the entire segment base
		memset(l_SegmentBase, 0, l_LoadableSegment->p_memsz);

		// Copy over just the file size from source
		memcpy(l_SegmentBase, static_cast<const uint8_t*>(m_SourceElf) + l_LoadableSegment->p_offset, l_LoadableSegment->p_filesz);

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
		if (!SetProtection(reinterpret_cast<void*>(l_SegmentBase), l_LoadableSegment->p_memsz, l_DataProtection))
		{
			if (m_LoaderType == ElfLoaderType_t::KernelProc)
			{
				auto printf = (void(*)(const char *format, ...))kdlsym(printf);
				printf("err: could update memory protections\n");
			}
			else
				WriteNotificationLog("err: could update memory protections\n");
			return false;
		}
	}

	// Save the dynamic location
	m_Dynamic = reinterpret_cast<Elf64_Dyn*>(static_cast<uint8_t*>(m_AllocatedMap) + (m_SourceDynamicProgramHeader->p_vaddr - s_VirtualAddressMinimum));

	// Parse the dynamic information
	if (!ParseDynamic())
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not parse dynamics\n");
		}
		else
			WriteNotificationLog("err: could not parse dynamics\n");
		return false;
	}

	// TODO: Parse dpcpu
	if (!ParseDpcpu())
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not parse pcpu info\n");
		}
		else
			WriteNotificationLog("err: could not parse pcpu info\n");
		return false;
	}

	// TODO: Load dependencies

	if (!RelocateFile())
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not elf reloc local\n");
		}
		else
			WriteNotificationLog("err: could not elf reloc local\n");
		return false;
	}

	// Update the entrypoint
	m_Entrypoint = static_cast<uint8_t*>(m_AllocatedMap) + (m_SourceHeader->e_entry - s_VirtualAddressMinimum);
	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("info: map: %p src: %p ep: %p\n", GetAllocatedMap(), m_SourceElf, m_Entrypoint);
	}

	// TODO: Attempt to load symbol table (if it fucking exists, fuck you)
	auto s_SectionHeaderSize = m_SourceHeader->e_shnum * m_SourceHeader->e_shentsize;
	if (s_SectionHeaderSize == 0 || m_SourceHeader->e_shoff == 0 || m_SourceHeader->e_shoff + s_SectionHeaderSize > m_SourceElfSize)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("warn: no symbol information exists\n");
		}
		else
			WriteNotificationLog("warn: no symbol information exists\n");
		return true; // Ensure we return true here
	}

	auto s_SectionHeaderData = static_cast<Elf64_Shdr*>(Allocate(s_SectionHeaderSize));
	if (s_SectionHeaderData == nullptr)
	{
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not allocate section header data\n");
		}
		else
			WriteNotificationLog("err: could not allocate section header data\n");
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
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("warn: could not find symbol table index (%d) or symbol string table index (%d)\n", s_SymTabIndex, s_SymStrIndex);
		}
		else
			WriteNotificationLog("warn: could not find symbol table or symbol string table\n");
		
		// Don't forget to free memory
		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

	// Allocate our symbol table
	m_SymbolTableSize = s_SectionHeaderData[s_SymTabIndex].sh_size;
	m_SymbolTableBase = reinterpret_cast<Elf64_Addr>(Allocate(m_SymbolTableSize));
	if (m_SymbolTableBase == reinterpret_cast<Elf64_Addr>(nullptr))
	{
		m_SymbolTableSize = 0;
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not allocate symbol table (sz: 0x%llx)\n", m_SymbolTableSize);
		}
		else
			WriteNotificationLog("err: could not allocate symbol table\n");
		
		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("info: copying data from (%p 0x%llx) -> (%p 0x%llx)\n", static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymTabIndex].sh_offset, m_SymbolTableSize, reinterpret_cast<void*>(m_SymbolTableBase), m_SymbolTableSize);
	}
	else
		WriteNotificationLog("info: copying data\n");

	memcpy(reinterpret_cast<void*>(m_SymbolTableBase), static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymTabIndex].sh_offset, m_SymbolTableSize);

	m_StringTableBaseSize = s_SectionHeaderData[s_SymStrIndex].sh_size;
	m_StringTableBase = reinterpret_cast<Elf64_Addr>(Allocate(m_StringTableBaseSize));
	if (m_StringTableBase == reinterpret_cast<Elf64_Addr>(nullptr))
	{
		Free(reinterpret_cast<void*>(m_SymbolTableBase));
		m_SymbolTableBase = 0;

		m_SymbolTableSize = 0;
		if (m_LoaderType == ElfLoaderType_t::KernelProc)
		{
			auto printf = (void(*)(const char *format, ...))kdlsym(printf);
			printf("err: could not allocate string table (sz: 0x%llx)\n", m_StringTableBaseSize);
		}
		else
			WriteNotificationLog("err: could not allocate string table\n");
		m_StringTableBaseSize = 0;

		Free(s_SectionHeaderData);
		s_SectionHeaderData = nullptr;

		return true;
	}

	if (m_LoaderType == ElfLoaderType_t::KernelProc)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("info: 2 copying data from (%p 0x%llx) -> (%p 0x%llx)\n", static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymStrIndex].sh_offset, m_StringTableBaseSize, reinterpret_cast<void*>(m_StringTableBase), m_StringTableBaseSize);
	}
	else
		WriteNotificationLog("info: 2 copying data\n");
	
	memcpy(reinterpret_cast<void*>(m_StringTableBase), static_cast<const uint8_t*>(m_SourceElf) + s_SectionHeaderData[s_SymStrIndex].sh_offset, m_StringTableBaseSize);

	m_DdbSymbolCount = m_SymbolTableSize / sizeof(Elf64_Sym);
	m_DdbSymbolTable = (const Elf64_Sym*)m_SymbolTableBase;
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