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
            
        };
    }
}