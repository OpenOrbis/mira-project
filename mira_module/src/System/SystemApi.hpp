#pragma once
#include <cstdint>

class SystemApi
{
public:
    bool ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength);
    bool WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength);
};