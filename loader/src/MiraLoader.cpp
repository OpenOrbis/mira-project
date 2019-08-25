#include "MiraLoader.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Dynlib.hpp>
#include <Utils/Logger.hpp>

#include <sys/mman.h>
#include <sys/malloc.h>
#include <stdarg.h>

using namespace MiraLoader;
using namespace Mira::Utils;

#ifndef UINT64_MAX
#define UINT64_MAX 0xFFFFFFFFFFFFFFFF
#endif

Loader::Loader(uint8_t* p_ElfData, uint64_t p_ElfDataLength, bool p_IsInKernel) :
    m_Data(p_ElfData),
    m_DataLength(p_ElfDataLength),
    m_AllocatedData(nullptr),
    m_AllocatedDataSize(0),
    m_EntryPoint(nullptr),
    m_IsInKernel(p_IsInKernel)
{
    auto s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
    auto s_LoadableSegmentSize = GetLoadableSegmentsSize();
	if (s_LoadableSegmentSize == 0)
		return;

	uint32_t s_AlignedLoadableSegmentSize = RoundUp(s_LoadableSegmentSize, 0x4000);

	uint8_t* s_LoadableData = static_cast<uint8_t*>(Allocate(s_AlignedLoadableSegmentSize));
	if (s_LoadableData == NULL)
	{
		if (m_IsInKernel)
			WriteLog(LL_Info, "could not load allocate (%llx).\n", s_AlignedLoadableSegmentSize);

		return;
	}
	Memset(s_LoadableData, 0, s_AlignedLoadableSegmentSize);

    m_AllocatedData = s_LoadableData;
	m_AllocatedDataSize = s_AlignedLoadableSegmentSize;
	
	if (m_IsInKernel)
		WriteLog(LL_Info, "loadableData: (%p).\n", s_LoadableData);

	if (!LoadSegments())
	{
		if (m_IsInKernel)
			WriteLog(LL_Info, "could not load segments.\n");

		return;
	}

	if (m_IsInKernel)
		WriteLog(LL_Info, "loaded segments\n", s_LoadableData);

	if (!RelocateElf())
	{
		if (m_IsInKernel)
			WriteLog(LL_Info, "could not relocate elf.\n");

		return;
	}

	if (m_IsInKernel)
		WriteLog(LL_Info, "relocated segments\n", s_LoadableData);

    if (!UpdateProtections())
    {
        if (m_IsInKernel)
            WriteLog(LL_Error, "could not update protections");
        
        return;
    }

	if (m_IsInKernel)
		WriteLog(LL_Info, "updated protections\n", s_LoadableData);

	Elf64_Phdr* s_EntryProgramHeader = GetProgramHeaderByFileOffset(s_Header->e_entry);
	if (s_EntryProgramHeader == NULL)
	{
		if (m_IsInKernel)
			WriteLog(LL_Error, "could not get entry point program header.\n");

		return;
	}

	m_EntryPoint = reinterpret_cast<void(*)(void*)>(s_EntryProgramHeader->p_vaddr + s_Header->e_entry);
	return;
}

Loader::~Loader()
{
    // Free all of the elf resources and terminate the process
}

uint64_t Loader::RoundUp(uint64_t p_Number, uint64_t p_Multiple)
{
    if (p_Multiple == 0)
		return p_Number;

	uint64_t s_Remainder = p_Number % p_Multiple;
	if (s_Remainder == 0)
		return p_Number;

	return p_Number + p_Multiple - s_Remainder;
}

int32_t Loader::Strcmp(const char* p_First, const char* p_Second)
{
    	while (*p_First == *p_Second++)
		if (*p_First++ == '\0')
			return (0);
	return (*(const unsigned char *)p_First - *(const unsigned char *)(p_Second - 1));
}

void Loader::Memcpy(void* p_Destination, void* p_Source, uint64_t p_Size)
{
    for (uint64_t i = 0; i < p_Size; ++i)
        ((uint8_t*)p_Destination)[i] = ((uint8_t*)p_Source)[i];
}

void Loader::Memset(void* p_Address, uint8_t p_Value, uint64_t p_Length)
{
    volatile uint8_t c = (uint8_t)p_Value;

    for (uint64_t i = 0; i < p_Length; ++i)
        *(((uint8_t*)p_Address) + i) = c;
}

void Loader::WriteNotificationLog(const char* p_Message)
{
    if (p_Message == nullptr)
		return;

	// Load the sysutil module, needs to be rooted for this to work
	int32_t moduleId = -1;
	Dynlib::LoadPrx("/system/common/lib/libSceSysUtil.sprx", &moduleId);

	// Validate that the module loaded properly
	if (moduleId == -1)
		return;

	int(*sceSysUtilSendSystemNotificationWithText)(int messageType, const char* message) = NULL;

	// Resolve the symbol
	Dynlib::Dlsym(moduleId, "sceSysUtilSendSystemNotificationWithText", &sceSysUtilSendSystemNotificationWithText);

	if (sceSysUtilSendSystemNotificationWithText)
		sceSysUtilSendSystemNotificationWithText(222, p_Message);

	Dynlib::UnloadPrx(moduleId);
}

void* Loader::Allocate(uint32_t p_Size)
{
    void* s_AllocationData = nullptr;

#ifdef _WIN32
    s_AllocationData = Win32Allocate(p_Size);
#else
    if (m_IsInKernel)
    {
        auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
		vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));

		s_AllocationData = (void*)kmem_alloc(map, p_Size);
    }
    else
    {
        s_AllocationData = _mmap(NULL, p_Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    }    
#endif

    if (s_AllocationData != nullptr)
        Memset(s_AllocationData, 0, p_Size);

    return s_AllocationData;
}

#ifdef _WIN32
void* Loader::Win32Allocate(uint32_t p_Size)
{
	return malloc(p_Size);
}
#endif

bool Loader::SetProtection(void* p_Address, uint64_t p_Size, int32_t p_Protection)
{
    if (p_Address == nullptr)
        return false;
    
#ifdef _WIN32
    DWORD oldProtection = 0;
	if (!VirtualProtect(data, dataSize, protection, &oldProtection))
		return false;
#else
    if (m_IsInKernel)
    {
        // TODO: pmap_protect
    }
    else
    {
        if ((int64_t)syscall3(SYS_MPROTECT, p_Address, (void*)p_Size, (void*)(int64_t)p_Protection) < 0)
            return false;     
    }
#endif

    return true;
}

bool Loader::LoadSegments()
{
    if (m_Data == nullptr || m_DataLength == 0 || m_AllocatedData == nullptr || m_AllocatedDataSize == 0)
        return false;

	if (m_DataLength < sizeof(Elf64_Ehdr))
		return false;

	if (m_IsInKernel)
		WriteLog(LL_Info, "here\n");

    auto s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
    for (Elf64_Half l_ProgramIndex = 0; l_ProgramIndex < s_Header->e_phnum; ++l_ProgramIndex)
    {
        auto l_ProgramHeader = GetProgramHeaderByIndex(l_ProgramIndex);
        if (l_ProgramHeader == nullptr)
            continue;

		if (m_IsInKernel)
			WriteLog(LL_Info, "here\n");
        
        if (l_ProgramHeader->p_type != PT_LOAD)
            continue;
        
		if (m_IsInKernel)
			WriteLog(LL_Info, "here\n");

        if (l_ProgramHeader->p_filesz > l_ProgramHeader->p_memsz)
			continue;

		if (l_ProgramHeader->p_filesz == 0)
			continue;

		if (l_ProgramHeader->p_offset + l_ProgramHeader->p_filesz > m_DataLength)
			continue;
		
		if (m_IsInKernel)
			WriteLog(LL_Info, "here\n");
		// Save the previous va in the pa
		//l_ProgramHeader->p_paddr = l_ProgramHeader->p_vaddr;

		// Copy over the data
		// TODO: Bounds check the memcpy
		if (l_ProgramHeader->p_vaddr > m_AllocatedDataSize)
		{
			if (m_IsInKernel)
				WriteLog(LL_Error, "attempted to write out of bounds for va: (%p).", l_ProgramHeader->p_vaddr);
			
			continue;
		}

		if (l_ProgramHeader->p_offset > m_DataLength)
		{
			if (m_IsInKernel)
				WriteLog(LL_Error, "attempted to read past the data length off: (%p).", l_ProgramHeader->p_offset);
			
			continue;
		}

		Memcpy((m_AllocatedData + l_ProgramHeader->p_vaddr), (m_Data + l_ProgramHeader->p_offset), l_ProgramHeader->p_filesz);

		if (m_IsInKernel)
			WriteLog(LL_Info, "here\n");
			
		l_ProgramHeader->p_vaddr = reinterpret_cast<Elf64_Addr>((m_AllocatedData + l_ProgramHeader->p_paddr));

		if (m_IsInKernel)
			WriteLog(LL_Info, "here\n");

		// Calculate data protection starting from ---
		int32_t l_DataProtection = 0;

#ifdef _WIN32
		if (l_ProgramHeader->p_flags & PF_R)
			l_DataProtection |= PAGE_EXECUTE_READ;
		if (l_ProgramHeader->p_flags & PF_W)
			l_DataProtection |= PAGE_EXECUTE_READWRITE;
		if (l_ProgramHeader->p_flags & PF_X)
			l_DataProtection |= PAGE_EXECUTE_READ;
#else
		if (l_ProgramHeader->p_flags & PF_R)
			l_DataProtection |= PF_R;
		if (l_ProgramHeader->p_flags & PF_W)
			l_DataProtection |= PF_W;
		if (l_ProgramHeader->p_flags & PF_X)
			l_DataProtection |= PF_X;
#endif

		// Update the protection on this crap
		if (!SetProtection(reinterpret_cast<void*>(l_ProgramHeader->p_vaddr), l_ProgramHeader->p_memsz, l_DataProtection))
		{
			if (m_IsInKernel)
				WriteLog(LL_Error, "could not set protection");
			else
				WriteNotificationLog("could not set protection");
		}
    }

	if (m_IsInKernel)
		WriteLog(LL_Debug, "successfully loaded all segments");

    return true;
}

bool Loader::RelocateElf()
{
    if (m_Data == nullptr || m_DataLength == 0 || m_AllocatedData == nullptr || m_AllocatedDataSize == 0)
        return false;
    
    auto s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
    Elf64_Sym* s_SymbolTable = nullptr;
    Elf64_Rela* s_RelocationTable = nullptr;
    Elf64_Xword s_RelocationEntSize = 0;
    Elf64_Xword s_RelocationCount = 0;

    const char* s_StringTable = nullptr;
    Elf64_Xword s_StringTableSize = 0;

    for (Elf64_Half s_SectionIndex = 0; s_SectionIndex < s_Header->e_shnum; ++s_SectionIndex)
    {
        auto l_SectionHeader = GetSectionHeaderByIndex(s_SectionIndex);
        if (l_SectionHeader == nullptr)
            continue;
        
        if (l_SectionHeader->sh_offset > m_DataLength)
            continue;
        
        switch (l_SectionHeader->sh_type)
        {
        case SHT_SYMTAB:
			{
				s_SymbolTable = reinterpret_cast<Elf64_Sym*>((m_Data + l_SectionHeader->sh_offset));

				Elf64_Word l_StringSectionIndex = l_SectionHeader->sh_link;
				if (l_StringSectionIndex > s_Header->e_shnum)
					continue;

				Elf64_Shdr* l_StringTableHeader = GetSectionHeaderByIndex(l_StringSectionIndex);
				if (l_StringTableHeader == nullptr)
					continue;

				s_StringTable = reinterpret_cast<const char*>((m_Data + l_StringTableHeader->sh_offset));
				s_StringTableSize = l_StringTableHeader->sh_size;
				break;
			}
		case SHT_RELA:
			{
				s_RelocationEntSize = l_SectionHeader->sh_entsize;
				if (s_RelocationEntSize == 0)
				{
					if (m_IsInKernel)
						WriteLog(LL_Error, "relocation entry size is zero.\n");

					continue;
				}
				s_RelocationCount = l_SectionHeader->sh_size / s_RelocationEntSize;
				s_RelocationTable = reinterpret_cast<Elf64_Rela*>((m_Data + l_SectionHeader->sh_offset));
				break;
			}
        }
    }

    // Validate that we have any relocations
    if (s_RelocationTable == nullptr || s_RelocationCount == 0)
    {
        if (m_IsInKernel)
            WriteLog(LL_Error, "no relocation data");
        else
            WriteNotificationLog("no relocation data");
        
        // Should be good to go?
        return true;
    }

    // Check if we have a string table
	if (s_StringTableSize == 0 || s_StringTable == nullptr)
	{
		if (m_IsInKernel)
			WriteLog(LL_Error, "no string table.\n");
		else
			WriteNotificationLog("no string table.\n");

		// If we don't have any, proceed, but check all usages before we do anything
	}

    for (Elf64_Xword l_RelocationEntryIndex = 0; l_RelocationEntryIndex < s_RelocationCount; ++l_RelocationEntryIndex)
	{
		// Get the relocation entry that we need
		Elf64_Rela* l_RelocationEntry = s_RelocationTable + l_RelocationEntryIndex;

		Elf64_Word l_SymbolIndex = ELF64_R_SYM(l_RelocationEntry->r_info);
		Elf64_Word l_SymbolType = ELF64_R_TYPE(l_RelocationEntry->r_info);

		const Elf64_Sym* l_Symbol = s_SymbolTable + l_SymbolIndex;

		const char* l_SymbolName = nullptr;
		if (s_StringTable == nullptr || l_Symbol->st_name > s_StringTableSize)
			l_SymbolName = "(null)";
		else
			l_SymbolName = s_StringTable + l_Symbol->st_name;

		Elf64_Addr* l_Location = (Elf64_Addr*)(m_AllocatedData + l_RelocationEntry->r_offset);

		/*if (m_IsInKernel)
			WriteLog(LL_Error, "relocating (%s) -> (%p).\n", l_SymbolName, l_Location);*/

		switch (l_SymbolType)
		{
		case R_X86_64_64:
			*l_Location = (Elf64_Addr)(m_AllocatedData + l_Symbol->st_value + l_RelocationEntry->r_addend);
			break;
		case R_X86_64_PC32:
			*l_Location = (Elf64_Addr)(m_AllocatedData + l_RelocationEntry->r_addend - l_RelocationEntry->r_offset);
			break;
		case R_X86_64_32:
			*l_Location = (Elf64_Addr)(m_AllocatedData + l_RelocationEntry->r_addend);
			break;
		case R_X86_64_32S:
			*l_Location = (Elf64_Addr)(m_AllocatedData + l_RelocationEntry->r_addend);
			break;
		case R_X86_64_JMP_SLOT:
		case R_X86_64_GLOB_DAT:
			*l_Location = (Elf64_Addr)0x4141414141414141; // TODO: Implement // (Elf64_Addr)elfloader_resolve(loader, symbolName);
			break;
		case R_X86_64_RELATIVE:
			*l_Location = (Elf64_Addr)(m_AllocatedData + l_RelocationEntry->r_addend);
			break;
		}
	}

	return true;
}

bool Loader::UpdateProtections()
{
    return true;
}

uint64_t Loader::GetLoadableSegmentsSize()
{
    if (m_Data == nullptr || m_DataLength == 0)
        return 0;
    
    Elf64_Ehdr* elfHeader = reinterpret_cast<Elf64_Ehdr*>(m_Data);

	uint64_t s_VaMin = UINT64_MAX;
	uint64_t s_VaMax = 0;

	for (Elf64_Half l_ProgramIndex = 0; l_ProgramIndex < elfHeader->e_phnum; ++l_ProgramIndex)
	{
		Elf64_Phdr* l_ProgramHeader = GetProgramHeaderByIndex(l_ProgramIndex);
		if (l_ProgramHeader == NULL)
			continue;

		if (l_ProgramHeader->p_type != PT_LOAD)
			continue;

		if (l_ProgramHeader->p_filesz > l_ProgramHeader->p_memsz)
			continue;

		if (l_ProgramHeader->p_filesz == 0)
			continue;

		if (l_ProgramHeader->p_offset + l_ProgramHeader->p_filesz > m_DataLength)
			continue;

		if (l_ProgramHeader->p_vaddr < s_VaMin)
			s_VaMin = l_ProgramHeader->p_vaddr;

		if (l_ProgramHeader->p_vaddr + l_ProgramHeader->p_memsz > s_VaMax)
			s_VaMax = l_ProgramHeader->p_vaddr + l_ProgramHeader->p_memsz;
	}

	if (s_VaMin == UINT64_MAX || s_VaMax == 0)
		return 0;

	return s_VaMax - s_VaMin;
}

Elf64_Dyn* Loader::GetDynamicByTag(uint64_t p_Tag)
{
    if (m_Data == nullptr || m_DataLength == 0)
        return nullptr;
    
    Elf64_Ehdr* s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);

    for (Elf64_Half l_ProgramIndex = 0; l_ProgramIndex < s_Header->e_phnum; ++l_ProgramIndex)
    {
        Elf64_Phdr* l_ProgramHeader = GetProgramHeaderByIndex(l_ProgramIndex);
		if (l_ProgramHeader == nullptr)
			continue;

		if (l_ProgramHeader->p_type != PT_DYNAMIC)
			continue;

		Elf64_Xword l_DynamicCount = l_ProgramHeader->p_filesz / sizeof(Elf64_Dyn);
		for (Elf64_Xword l_DynamicIndex = 0; l_DynamicIndex < l_DynamicCount; ++l_DynamicIndex)
		{
			Elf64_Dyn* l_Dynamic = reinterpret_cast<Elf64_Dyn*>(((m_Data + l_ProgramHeader->p_offset) + (sizeof(Elf64_Dyn) * l_DynamicIndex)));

			if (l_Dynamic->d_tag == p_Tag)
				return l_Dynamic;
		}
    }

    return nullptr;
}

Elf64_Phdr* Loader::GetProgramHeaderByIndex(uint32_t p_Index)
{
    if (m_Data == nullptr || m_DataLength == 0)
        return nullptr;
    
    Elf64_Ehdr* s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
    if (p_Index >= s_Header->e_phnum)
        return nullptr;

	return reinterpret_cast<Elf64_Phdr*>(((m_Data + s_Header->e_phoff) + (s_Header->e_phentsize * p_Index)));
}

Elf64_Shdr* Loader::GetSectionHeaderByIndex(uint32_t p_Index)
{
    if (m_Data == nullptr || m_DataLength == 0)
        return nullptr;

    Elf64_Ehdr* s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
	if (p_Index >= s_Header->e_shnum)
		return nullptr;

	return reinterpret_cast<Elf64_Shdr*>(((m_Data + s_Header->e_shoff) + (s_Header->e_shentsize * p_Index)));
}

Elf64_Phdr* Loader::GetProgramHeaderByFileOffset(uint64_t p_FileOffset)
{
    if (m_Data == nullptr || m_DataLength == 0)
        return nullptr;

    Elf64_Ehdr* s_Header = reinterpret_cast<Elf64_Ehdr*>(m_Data);
	for (Elf64_Half l_ProgramIndex = 0; l_ProgramIndex < s_Header->e_phnum; ++l_ProgramIndex)
	{
		Elf64_Phdr* programHeader = GetProgramHeaderByIndex(l_ProgramIndex);
		if (programHeader == nullptr)
			continue;

		if (programHeader->p_type != PT_LOAD)
			continue;

		if (p_FileOffset >= programHeader->p_paddr && p_FileOffset < programHeader->p_paddr + programHeader->p_memsz)
			return programHeader;
	}

	return nullptr;
}

