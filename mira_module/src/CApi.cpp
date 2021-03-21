#include "CApi.h"

extern "C" int32_t mira_read_process_memory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength)
{
    return SystemApi::ReadProcessMemory(p_ProcessId, p_ProcessAddress, p_Data, p_DataLength);
}

extern "C" int32_t mira_write_process_memory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength)
{
    return SystemApi::WriteProcessMemory(p_ProcessId, p_ProcessAddress, p_Data, p_DataLength);
}

extern "C" int32_t mira_get_process_list(int32_t* p_OutputProcessList, uint32_t p_OutputProcessCount)
{
    return SystemApi::GetProcessList(p_OutputProcessList, p_OutputProcessCount);
}

extern "C" int32_t mira_get_proc_information(int32_t p_ProcessId, MiraProcessInformation* p_OutputInfo)
{
    return SystemApi::GetProcInformation(p_ProcessId, p_OutputInfo);
}