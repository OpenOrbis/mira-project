// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Hook.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

extern "C"
{
    #include <sys/mman.h>
    #include <hde64/hde64.h>
};

using namespace Mira::Utils;

#define HOOK_LENGTH	14

Hook::Hook() :
    m_TargetAddress(nullptr),
    m_HookAddress(nullptr),
    m_TrampolineAddress(nullptr),
    m_TrampolineSize(0),
    m_BackupData(nullptr),
    m_BackupLength(0),
    m_Enabled(false)
{

}

Hook::Hook(void* p_TargetAddress, void* p_HookAddress) :
    m_TargetAddress(p_TargetAddress),
    m_HookAddress(p_HookAddress),
    m_BackupData(nullptr),
    m_BackupLength(0),
    m_Enabled(false)
{
    if (p_TargetAddress == nullptr || p_HookAddress == nullptr)
        return;
    
    int32_t s_BackupDataLength = GetMinimumHookSize(p_TargetAddress);
    if (s_BackupDataLength < 0)
    {
        WriteLog(LL_Error, "could not backup hook for (%p).", p_TargetAddress);
        return;
    }

    uint8_t* s_BackupData = new uint8_t[s_BackupDataLength];
    if (s_BackupData == nullptr)
    {
        WriteLog(LL_Error, "could not allocate space for backup data.");
        return;
    }
    // Zero out and copy the beginning of the function
    memset(s_BackupData, 0, s_BackupDataLength);
	memcpy(s_BackupData, p_TargetAddress, s_BackupDataLength);

    // Assign our variables for tracking later
    m_BackupData = s_BackupData;
    m_BackupLength = s_BackupDataLength;

    // if (!CreateTrampoline(m_TrampolineAddress, m_TrampolineSize))
    // {
    //     WriteLog(LL_Error, "error creating trampoline.");
    // }
}

Hook::~Hook()
{
    // Disable hooks on destruction
    if (m_Enabled)
        Disable();
    
    // We don't want to keep references to any previous code sections
    m_TargetAddress = nullptr;
    m_HookAddress = nullptr;

    // Free the backup data that was created
    if (m_BackupData != nullptr && m_BackupLength != 0)
        delete [] m_BackupData;

    m_BackupData = nullptr;
    m_BackupLength = 0;

    if (m_TrampolineAddress != nullptr)
        k_free(m_TrampolineAddress);
    
    m_TrampolineAddress = nullptr;
    m_TrampolineSize = 0;

    m_Enabled = false;
}

bool Hook::Enable()
{
    // Return false if the hook is already enabled
    if (m_Enabled)
        return false;
    
    // TODO: Enable the hook
    if (m_HookAddress == nullptr || m_TargetAddress == nullptr)
        return false;
    
    auto critical_enter = (void(*)(void))kdlsym(critical_enter);
	auto critical_exit = (void(*)(void))kdlsym(critical_exit);

	uint8_t jumpBuffer[] = {
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// # jmp    QWORD PTR [rip+0x0]
		0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,	// # DQ: AbsoluteAddress
	}; // Shit takes 14 bytes

	uint64_t* jumpBufferAddress = (uint64_t*)(jumpBuffer + 6);

	// Assign the address
	*jumpBufferAddress = (uint64_t)m_HookAddress;

	// Change permissions and apply the hook
	critical_enter();
	cpu_disable_wp();
	memcpy(m_TargetAddress, jumpBuffer, sizeof(jumpBuffer));
	cpu_enable_wp();
	critical_exit();

    // Set the enabled flag
    m_Enabled = true;

    // Return successfully
    return true;
}

bool Hook::Disable()
{
    // Don't allow disabling of already disabled hooks
    if (!m_Enabled)
        return false;

    if (m_HookAddress == nullptr || m_TargetAddress == nullptr)
		return false;

	if (m_BackupData == nullptr || m_BackupLength == 0)
		return false;

    auto critical_enter = (void(*)(void))kdlsym(critical_enter);
	auto critical_exit = (void(*)(void))kdlsym(critical_exit);

	// Change permissions and apply the hook
	critical_enter();
	cpu_disable_wp();
	memcpy(m_TargetAddress, m_BackupData, m_BackupLength);
	cpu_enable_wp();
	critical_exit();

    // Set the disabled flag
    m_Enabled = false;

    // Return successfully
    return true;
}


void* Hook::GetOriginalFunctionAddress()
{
    return m_TargetAddress;
}

void* Hook::GetHookedFunctionAddress()
{
    return m_HookAddress;
}

void* Hook::GetTrampoline(uint32_t* p_OutSize)
{
    if (p_OutSize != nullptr)
        *p_OutSize = m_TrampolineSize;

    return m_TrampolineAddress;
}

/*
    This function will calculate the minimum safe size to overwrite
    This will not chop opcodes in half (ex, len 15 opcode overwriting only 2/15 bytes)
    This is the amount that will be backed up
*/
int32_t Hook::GetMinimumHookSize(void* p_Target)
{
    hde64s hs;

	uint32_t hookSize = HOOK_LENGTH;
	uint32_t totalLength = 0;

	
	while (totalLength < hookSize)
	{
		uint32_t length = hde64_disasm(p_Target, &hs);
		if (hs.flags & F_ERROR)
			return -1;

		totalLength += length;
	}

	return totalLength;
}

bool Hook::CreateTrampoline(void*& p_OutTrampoline, uint32_t& p_OutTrampolineSize)
{
    // Set our output variable defaults
    p_OutTrampoline = nullptr;
    p_OutTrampolineSize = 0;

    // First we calculate the min backup size (overwritten bytes by hook + correct ending)
    auto s_BackupBytesSize = GetMinimumHookSize(m_TargetAddress);
    WriteLog(LL_Debug, "BackupBytesSize: (%d).", s_BackupBytesSize);
    if (s_BackupBytesSize < HOOK_LENGTH)
    {
        WriteLog(LL_Error, "min bytes not large enough.");
        return false;
    }

    // Allocate a new trampoline using RWX memory
    auto s_TrampolineSize = s_BackupBytesSize + HOOK_LENGTH;
    uint8_t* s_Trampoline = static_cast<uint8_t*>(k_malloc(s_TrampolineSize));
    if (s_Trampoline == nullptr)
    {
        WriteLog(LL_Error, "could not allocate trampoline.");
        return false;
    }
    memset(s_Trampoline, 0, s_TrampolineSize);

    // Create a temporary jump buffer to hold our jmp rip
    uint8_t s_JumpBuffer[] = {
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// # jmp    QWORD PTR [rip+0x0]
		0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,	// # DQ: AbsoluteAddress
	}; // Shit takes 14 bytes

    // Get the offset to write the address
    uint64_t* s_JumpBufferAddress = (uint64_t*)(&s_JumpBuffer[6]);

    // Write the midfunction address into the jump buffer
    *s_JumpBufferAddress = reinterpret_cast<uint64_t>(m_TargetAddress) + s_BackupBytesSize;

    // Copy over the prologue from the original function to our trampoline
    memcpy(s_Trampoline, m_TargetAddress, s_BackupBytesSize);

    // Copy the jump back to midfunction
    memcpy(s_Trampoline + s_BackupBytesSize, s_JumpBuffer, sizeof(s_JumpBuffer));

    // Set our output trampoline and size
    p_OutTrampoline = s_Trampoline;
    p_OutTrampolineSize = s_TrampolineSize;

    return true;
}
uint8_t* Hook::CreateTrampoline(uint32_t* p_OutTrampolineSize)
{
    if (m_TargetAddress == nullptr || m_HookAddress == nullptr)
        return nullptr;
    
    if (m_BackupLength <= 0 || m_BackupData == nullptr)
        return nullptr;

    auto s_TrampolineSize = m_BackupLength + HOOK_LENGTH + sizeof(uint8_t); // NOP;
    auto s_Trampoline = static_cast<uint8_t*>(k_malloc(s_TrampolineSize));//new uint8_t[s_TrampolineSize];
    if (s_Trampoline == nullptr)
        return nullptr;

    WriteLog(LL_Debug, "trampoline: %p (%x)", s_Trampoline, s_TrampolineSize);
    WriteLog(LL_Debug, "backup (%p) backupLen (%x)", m_BackupData, m_BackupLength);
    
    // TODO: Set correct prot on this, unless we always expect rwx (bad assumption)
    memcpy(s_Trampoline, m_BackupData, m_BackupLength);

    // Set up the jmp hook (bless, no registers clobbered bitches)
    uint8_t jumpBuffer[] = {
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// # jmp    QWORD PTR [rip+0x0]
		0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,	// # DQ: AbsoluteAddress
	}; // Shit takes 14 bytes

	uint64_t* jumpBufferAddress = (uint64_t*)(jumpBuffer + 6);

    // Calculate and set the midfunction address
    uint64_t s_Midfunctionoffset = m_BackupLength - 1;
    WriteLog(LL_Debug, "midfunctionOffset: %x, backupLength %x", s_Midfunctionoffset, m_BackupLength);
    uint64_t s_TargetMidFunctionAddress = ((uint64_t)m_TargetAddress) + s_Midfunctionoffset;

    WriteLog(LL_Debug, "jumpBuffer (%p) jumpBufferAddress (%p) midFunctionAddress (%p)", jumpBuffer, jumpBufferAddress, s_TargetMidFunctionAddress);

    *jumpBufferAddress = s_TargetMidFunctionAddress;

    // Copy the jump buffer to the end of the backup bytes in the trampoline
    auto s_TrampolineJumpBufferAddress = (s_Trampoline + s_Midfunctionoffset);
    WriteLog(LL_Debug, "trampolineJump (%p) byteAtLoc %x", s_TrampolineJumpBufferAddress, *s_TrampolineJumpBufferAddress);

    memcpy(s_TrampolineJumpBufferAddress, jumpBuffer, sizeof(jumpBuffer));

    // Set the very last byte of the trampoline to NOP
    auto s_EndOfTrampoline = &s_Trampoline[s_TrampolineSize - 1];
    *s_EndOfTrampoline = 0x90;

    // Update the output size if there was one passed in
    if (p_OutTrampolineSize != nullptr)
        *p_OutTrampolineSize = s_TrampolineSize;
    
    return s_Trampoline;
}

void* Hook::k_malloc(size_t size)
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

void Hook::k_free(void* address)
{
	if (!address)
		return;

	vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_free = (void(*)(void* map, void* addr, size_t size))kdlsym(kmem_free);

	
	uint8_t* data = ((uint8_t*)address) - sizeof(uint64_t);

	uint64_t size = *(uint64_t*)data;

	kmem_free(map, data, size);
}