#include "Hook.hpp"

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

extern "C"
{
    #include <hde64/hde64.h>
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

void* Hook::CreateTrampoline(void* p_OriginalFunction, HookType p_HookType)
{
    return nullptr;
}

uint32_t Hook::GetTemplateHookSize(HookType p_HookType)
{
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