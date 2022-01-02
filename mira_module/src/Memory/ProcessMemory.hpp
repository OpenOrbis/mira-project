#pragma once
#include <cstdint>

namespace Mira
{
    namespace Memory
    {
        class ProcessMemory
        {
        public:
            /**
             * @brief This is the user mode wrapper for calling the mira ioctl for allocating process memory
             * 
             * NOTE: This wraps the "raw" call by allocating sizeof(uint64_t) + p_Size to store the "real" allocation size
             * this "real" allocation size is required by the mira ioctl due to FreeBSD restrictions
             * 
             * @param p_ProcessId Process id, values of <= 0 are treated as the calling process id
             * @param p_Size Size of memory to allocate
             * @param p_Protection Protection of the allocation (accepts RWX)
             * @return uint8_t* nullptr on error, otherwise valid allocation with proper protection
             */
            static void* AllocateProcessMemory(int32_t p_ProcessId, uint32_t p_Size, int32_t p_Protection);
            static void* AllocateProcessMemory(int32_t p_Device, int32_t p_ProcessId, uint32_t p_Size, int32_t p_Protection);

            /**
             * @brief This is the user mode wrapper for calling mira ioctl for freeing process memory
             * 
             * NOTE: This will unwrap the uint64_t before the returned pointer and attempt to free it
             * 
             * @param p_ProcessId Process Id, values of <= 0 are treated as the calling process id
             * @param p_Pointer Pointer to free
             */
            static void FreeProcessMemory(int32_t p_ProcessId, void* p_Pointer);
            static void FreeProcessMemory(int32_t p_Device, int32_t p_ProcessId, void* p_Pointer);

            /**
             * @brief Reads process memory
             * 
             * @param p_ProcessId Process id of process to read from, values <= 0 are treated as calling process id
             * @param p_ProcessAddress Address in the process to read from
             * @param p_OutputData Output buffer (must be size of p_Size)
             * @param p_Size Size of the output buffer/bytes to read from process
             * @return true on success, false otherwise
             */
            static bool ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_OutputData, uint32_t p_Size);
            static bool ReadProcessMemory(int32_t p_Device, int32_t p_ProcessId, void* p_ProcessAddress, void* p_OutputData, uint32_t p_Size);
            
            /**
             * @brief Writes process memory
             * 
             * @param p_ProcessId Process id of the process to write to, values <= are treated as calling process
             * @param p_ProcessAddress Address in the process to write to
             * @param p_InputData Data to write
             * @param p_Size Size of input data and size to write to process
             * @return true on success, false otherwise
             */
            static bool WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_InputData, uint32_t p_Size);
            static bool WriteProcessMemory(int32_t p_Device, int32_t p_ProcessId, void* p_ProcessAddress, void* p_InputData, uint32_t p_Size);
        };
    }
}