#include "Hook.hpp"
#include <Utils/Logger.hpp>

#include <cstring>

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

extern "C"
{
    #include <hde64/hde64.h>
    #include <sys/mman.h>
};

using namespace Mira::Hooks;

/**
 * @brief This is the template for jump absolute
 * This method does not clobber *any* register state and should be
 * used in most cases if applicable. It also allows far jumps to anywhere.
 * 
 * Length = 14
 */
const uint8_t c_JumpRipTemplate[] = 
{
    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// # jmp    QWORD PTR [rip+0x0]
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,	// # DQ: AbsoluteAddress
};

/**
 * @brief This is for a relative jump
 * The RelativeAddress must be within int32_t.MaxValue range forward or backward
 * The start is calculated by the end of this jump instruction.
 * 
 * Length = 5
 */
const uint8_t c_JumpRelativeTemplate[] = 
{
    0xE9,                                           // # jmp    0x5 (instruction after jump + RelativeAddress)
    0x41, 0x41, 0x41, 0x41                          // # DW: RelativeAddress
};

/**
 * @brief This is for an absolute far-jump
 * NOTE: This function clobbers RAX, if you need this either try a near-jmp trampoline
 * or a jump-rip hook.
 * 
 * Length = 12
 */
const uint8_t c_JumpRaxTemplate[] =
{ 
    0x48, 0xB8,                                     // # movabs rax, AbsoluteAddress
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, // # AbsoluteAddress
    0xFF, 0xE0                                      // # jmp rax
};

/**
 * @brief This is a template for a call <rel-32> instruction
 * 
 * Length = 5
 */
const uint8_t c_Call32Relative[] =
{
    0xE8,                                           // # call RelativeAddress
    0x41, 0x41, 0x41, 0x41                          // # DW: RelativeAddress
};

void* Hook::AllocateNearAddress(void* p_NearAddress, uint32_t p_Size)
{
    // TODO: Get the VM maps from the Mira Driver
    // Iterate through all of the vm_map's
    // Find a free space close to the address we provided
    // Allocate in that free space
    // Check to see if it's > int32_t.MaxValue
    // Return the allocation
    return nullptr;
}

uint32_t Hook::GetTemplateHookSize(HookType p_HookType)
{
    // These 3 lines below are just to shut up the warning
    memcmp(c_JumpRelativeTemplate, c_JumpRelativeTemplate, ARRAYSIZE(c_JumpRelativeTemplate));
    memcmp(c_JumpRaxTemplate, c_JumpRaxTemplate, ARRAYSIZE(c_JumpRaxTemplate));
    memcmp(c_Call32Relative, c_Call32Relative, ARRAYSIZE(c_Call32Relative));

    switch (p_HookType)
    {
    case HookType::JumpRip:
        return ARRAYSIZE(c_JumpRipTemplate);
    case HookType::JumpRelative:
        return ARRAYSIZE(c_JumpRelativeTemplate);
    case HookType::JumpRax:
        return ARRAYSIZE(c_JumpRaxTemplate);
    case HookType::CallRelative:
        return ARRAYSIZE(c_Call32Relative);
    default:
        return 0;
    }
}

bool Hook::ProtectMemory(void* p_Address, uint32_t p_Size, uint32_t p_Protection)
{
    // Validate arguments
    if (p_Address == nullptr || p_Size == 0)
        return false;

    // TODO: Get the old protection

    
    // Change the protection of the memory
    auto s_Ret = mprotect(p_Address, p_Size, p_Protection);
    if (s_Ret == -1)
    {
        WriteLog(LL_Error, "could not mprotect (%p) with perms (%d), err: (%d).", p_Address, p_Protection, errno);
        return false;
    }

    return true;
}

void* Hook::CreateTrampoline(void* p_OriginalFunction, HookType p_Type)
{
    // Validate original function
    if (p_OriginalFunction == nullptr)
        return nullptr;
    
    // Validate the hook type
    if (p_Type <= HookType::None || 
        p_Type >= HookType::COUNT ||
        // We don't allow creating trampolines for calls
        p_Type == HookType::CallRelative)
    {
        WriteLog(LL_Error, "invalid hook type.");
        return nullptr;
    }

    // Calculate the needed backup size for hook + extra "correct" instructions
    uint32_t s_BackupSize = GetMinimumHookSize(p_OriginalFunction, p_Type);
    if (s_BackupSize == 0)
    {
        WriteLog(LL_Error, "there was an error getting min hook size.");
        return nullptr;
    }

    // Validate that we have enough space to backup this function in hook
    if (s_BackupSize > MaxBackupSize)
    {
        WriteLog(LL_Error, "(%d) > max backup size (%d).", s_BackupSize, MaxBackupSize);
        return nullptr;
    }

    // Zero out the previous backup bytes
    (void)memset(m_BackupBytes, 0, ARRAYSIZE(m_BackupBytes));

    // Create a backup of the existing bytes
    (void)memcpy(m_BackupBytes, p_OriginalFunction, s_BackupSize);

    // Create a new trampoline in "memory space"
    uint32_t s_TemplateSize = GetTemplateHookSize(HookType::JumpRip);
    if (s_TemplateSize == 0)
    {
        WriteLog(LL_Error, "could not get the template size.");
        return nullptr;
    }

    // Calculate the total trampoline size which are backup bytes + absolute jump to midfunction
    uint32_t s_TrampolineSize = s_BackupSize + s_TemplateSize;

    // Allocate new trampoline data
    uint8_t* s_Trampoline = new uint8_t[s_TrampolineSize];
    if (s_Trampoline == nullptr)
    {
        WriteLog(LL_Error, "could not allocate trampoline.");
        return nullptr;
    }
    // Zero out trampoline
    (void)memset(s_Trampoline, 0, s_BackupSize);

    // Copy all bytes overwritten by the hook
    (void)memcpy(s_Trampoline, m_BackupBytes, s_BackupSize);

    // Create a new Jump-Rip template
    uint8_t s_JumpRip[s_TemplateSize];
    (void)memcpy(s_JumpRip, c_JumpRipTemplate, s_TemplateSize);
    
    // Set the absolute jump address
    *(uint64_t*)(s_JumpRip + 6) = reinterpret_cast<uint64_t>(p_OriginalFunction) + s_BackupSize;

    // Copy the absolute jump to the trampoline
    (void)memcpy(s_Trampoline + s_BackupSize, s_JumpRip, ARRAYSIZE(s_JumpRip));

    // Finally we need to change the protection on this memory
    if (!ProtectMemory(s_Trampoline, s_TemplateSize, PROT_READ | PROT_EXEC))
    {
        WriteLog(LL_Error, "could not protect memory.");
        delete [] s_Trampoline;
        return nullptr;
    }

    // Return the completed trampoline
    return s_Trampoline;
}

uint32_t Hook::GetMinimumHookSize(void* p_Target, HookType p_HookType)
{
    hde64s s_Dism;
    uint32_t s_HookMinSize = GetTemplateHookSize(p_HookType);
    uint32_t s_TotalLength = 0;

    while (s_TotalLength < s_HookMinSize)
    {
        uint32_t s_InstructionLength = hde64_disasm(p_Target, &s_Dism);
        if (s_Dism.flags & F_ERROR)
            return 0;
        
        s_TotalLength += s_InstructionLength;
    }

    return s_TotalLength;
}