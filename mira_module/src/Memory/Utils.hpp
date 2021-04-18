#pragma once
#include <functional>

extern "C"
{
    #include <sys/types.h>
    #include <sys/mman.h>
};

#define DECLARE_VTABLE_HOOK_OFFSET(name, offset)
#define DECLARE_CALL_HOOK(name, offset)
#define DECLARE_IAT_HOOK(module, name)
#define DECLARE_PATTERN_HOOK()

namespace Mira
{
    namespace Memory
    {
        class MemoryRange
        {
        public:
            void* Start;
            void* End;
            int32_t Protection;
        };

        class Utils
        {
            /**
             * @brief Find a pattern in process by process id. This will search *all* allocated memory regions of that process
             * 
             * Should be used with caution.
             * 
             * @param p_ProcessId Process Id
             * @param p_Pattern Pattern to match
             * @param p_Mask Mask to match
             * @return void* nullptr if pattern not found, non-nullptr otherwise
             */
            void* FindPattern(int32_t p_ProcessId, const char* p_Pattern, const char* p_Mask);

            /**
             * @brief Find a pattern in the CURRENT process. This will search *all* allocated memory regions.
             * 
             * Should be used with caution
             * 
             * @param p_Pattern Pattern to match
             * @param p_Mask Mask to match
             * @return void* nullptr if pattern not found, non-nullptr otherwise
             */
            void* FindPattern(const char* p_Pattern, const char* p_Mask);

            /**
             * @brief Finds a pattern in the current process starting from memory range p_Start going to p_End
             * 
             * @param p_Start Memory address start
             * @param p_End Memory address end
             * @param p_Pattern Pattern to find
             * @param p_Mask Mask of the pattern
             * @return void* nullptr if pattern not found, non-nullptr otherwise
             */
            void* FindPattern(u_int8_t* p_Start, u_int8_t* p_End, const char* p_Pattern, const char* p_Mask);

            /**
             * @brief Finds a pattern in process by id, starting from memory range p_Start, going to p_End
             * 
             * @param p_ProcessId Process id of the process to find pattern
             * @param p_Start Starting memory address
             * @param p_End Ending memory address
             * @param p_Pattern Pattern to find
             * @param p_Mask Mask of the pattern
             * @return void* nullptr if pattern not found, non-nullptr otherwise
             */
            void* FindPattern(int32_t p_ProcessId, u_int8_t* p_Start, u_int8_t* p_End, const char* p_Pattern, const char* p_Mask);

            /**
             * @brief Iterates all allocated memory ranges in provided process
             * 
             * @param p_ProcessId ProcessId to iterate memory ranges
             * @param p_Callback Callback function
             */
            void IterateMemoryRegions(int32_t p_ProcessId, std::function<void(MemoryRange&)> p_Callback);

            /**
             * @brief Iterate all allocated memory ranges in current process
             * 
             * @param p_Callback Callback function
             */
            void IterateMemoryRanges(std::function<void(MemoryRange&)> p_Callback);


            // Process Memory
            
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
            void* AllocateProcessMemory(int32_t p_ProcessId, uint32_t p_Size, int32_t p_Protection);

            /**
             * @brief This is the user mode wrapper for calling mira ioctl for freeing process memory
             * 
             * NOTE: This will unwrap the uint64_t before the returned pointer and attempt to free it
             * 
             * @param p_ProcessId Process Id, values of <= 0 are treated as the calling process id
             * @param p_Pointer Pointer to free
             */
            void FreeProcessMemory(int32_t p_ProcessId, void* p_Pointer);

            /**
             * @brief Reads process memory
             * 
             * @param p_ProcessId Process id of process to read from, values <= 0 are treated as calling process id
             * @param p_ProcessAddress Address in the process to read from
             * @param p_OutputData Output buffer (must be size of p_Size)
             * @param p_Size Size of the output buffer/bytes to read from process
             * @return true on success, false otherwise
             */
            bool ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_OutputData, uint32_t p_Size);
            
            /**
             * @brief Writes process memory
             * 
             * @param p_ProcessId Process id of the process to write to, values <= are treated as calling process
             * @param p_ProcessAddress Address in the process to write to
             * @param p_InputData Data to write
             * @param p_Size Size of input data and size to write to process
             * @return true on success, false otherwise
             */
            bool WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_InputData, uint32_t p_Size);
        };
    }
}