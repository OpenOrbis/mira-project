#include "Utilities.hpp"
#include <Utils/Kernel.hpp>

using namespace Mira::OrbisOS;

// Credits: m0rph
void Utilities::HookFunctionCall(uint8_t* p_HookTrampoline, void* p_Function, void* p_Address)
{
    uint8_t* s_HookPayload = p_HookTrampoline;
    uint16_t* s_TempAddress = reinterpret_cast<uint16_t*>(p_HookTrampoline);
    s_TempAddress++;

    uint64_t* s_FunctionAddress = reinterpret_cast<uint64_t*>(s_TempAddress);

    cpu_disable_wp();

    // mov rax
    s_HookPayload[0] = 0x48;
    s_HookPayload[1] = 0xB8;

    *s_FunctionAddress = reinterpret_cast<uint64_t>(p_Function);

    s_HookPayload[0x0A] = 0xFF;
    s_HookPayload[0x0B] = 0xE0;

    int32_t s_CallAddress = (int32_t)(p_HookTrampoline - (uint8_t*)p_Address) - 5;
    s_HookPayload = reinterpret_cast<uint8_t*>(p_Address);
    s_HookPayload++;
    int32_t* s_Pointer = reinterpret_cast<int32_t*>(s_HookPayload);
    *s_Pointer = s_CallAddress;

    cpu_enable_wp();
}