#include "Hook.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

#include <External/hde64/hde64.h>

using namespace Mira::Utils;

#define HOOK_LENGTH	14

Hook::Hook() :
    m_TargetAddress(nullptr),
    m_HookAddress(nullptr),
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