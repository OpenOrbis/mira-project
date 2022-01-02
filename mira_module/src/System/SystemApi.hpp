#pragma once
#include <cstdint>

// Driver includes
#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

class SystemApi
{
public:
    /**
     * @brief Reads another processes memory
     * 
     * @param p_ProcessId Process Id of the target process to read
     * @param p_ProcessAddress Address in the target process to read
     * @param p_Data The output buffer where the read data is going to be placed
     * @param p_DataLength The size of the output buffer (as well as how much data to read)
     * @return true on success, false otherwise
     */
    static bool ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength);

    /**
     * @brief Writes another processes memory
     * 
     * @param p_ProcessId Process id of the target process to write to
     * @param p_ProcessAddress Address in the target process to write
     * @param p_Data Data to be written
     * @param p_DataLength Size of the input buffer (as well as how much data to read)
     * @return true on success, false otherwise
     */
    static bool WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength);

    /**
     * @brief Gets a list of active processes
     * 
     * NOTE: In order to use this properly, you will need to declare at least (N = Current Pid Count, or N = MaxPids (200))
     * Process id's of <= 0 are invalid and should be skipped (or parsing stopped)
     * 
     * @param p_OutputProcessList Pointer to the int32_t array to receive pids
     * @param p_ProcessCount Number of entries in p_OutputProcessList
     * @return true on success, false otherwise
     */
    static bool GetProcessList(int32_t* p_OutputProcessList, uint32_t p_ProcessCount);

    /**
     * @brief Get basic process information
     * 
     * This includes:
     *  Id, OpId, DebugChild, SigParent, Signal, Code, Stops, SType, Name, ElfPath, RandomizedPath, Threads
     *  Each Thread Includes:
     *      Id, ErrNo, RetVal, Name
     * 
     * @param p_ProcessId Process Id to get information for
     * @param p_ProcessInfo Output buffer with enough space for sizeof(MiraProcessInformation) + sizeof(ThreadResult) * N (where N = Maximum number of output threads, 2048 is default)
     * @return true on success, false otherwise
     */
    static bool GetProcInformation(int32_t p_ProcessId, ProcessInfo* p_ProcessInfo);

    // TODO: Need implementing
    static bool GetProcessPriv(uint8_t* p_Privs);
    static bool SetProcessPrivs(uint8_t* p_Privs);
};