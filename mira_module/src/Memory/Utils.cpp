#include "Utils.hpp"
#include <mira/Driver/DriverStructs.hpp>
#include <mira/Driver/DriverCmds.hpp>

#include <Utils/Logger.hpp>

#include <unistd.h>

using namespace Mira::Memory;

void* Utils::FindPattern(int32_t p_ProcessId, const char* p_Pattern, const char* p_Mask)
{
    return nullptr;
}

void Utils::IterateMemoryRanges(std::function<void(MemoryRange&)> p_Callback)
{
    
}