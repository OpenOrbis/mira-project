#pragma once
#include <cstdint>

namespace Mira
{
    namespace Hooks
    {
        class Hook
        {
        private:
            /**
             * @brief The different kinds of hook types that this hooking
             * library offers
             * 
             */
            typedef enum _HookType
            {
                None = 0,
                JumpRelative,
                JumpRax,
                JumpRip,
                CallRelative,
                COUNT
            } HookType;

            
            enum _Defaults
            {
                // Maximum bytes of x86_64 instruction (15) + 1
                MaxBackupSize = 16
            };

            // Type of hook that this is for
            HookType m_Type;

            // Backup data bytes that get overwritten with the hook
            uint8_t m_BackupBytes[MaxBackupSize];

        protected:
            /**
             * @brief This function will attempt to allocate memory "near" a specified address
             * It will iterate all of the mapped memory looking for a "free" space to slot into
             * 
             * This is useful for when you have a call <32-bit relative>, or jmp <32-bit relative>
             * and cannot do a far jump directly into trainer code
             * 
             * @param p_NearAddress Address to try and get close to allocating near
             * @param p_Size Size to allocate
             * @return void* nullptr if no available address is found, or value on success
             */
            void* AllocateNearAddress(void* p_NearAddress, uint32_t p_Size);

            /**
             * @brief Changes the protection on a part of memory
             * 
             * @param p_Address Address to change the protections on
             * @param p_Size Size of the memory to change protections on
             * @param p_Protection New protection to apply
             * @param p_OldProtection Old protection before changing
             * @return bool true on success, failure otherwise
             */
            bool ProtectMemory(void* p_Address, uint32_t p_Size, uint32_t p_Protection);

            /**
             * @brief Create a Trampoline
             * 
             * @param p_OriginalFunction The original function to create a trampoline for
             * @param p_Type The type of hook that is being applied (they vary in size)
             * 
             * This function will automatically set up the permissions to be ready to use
             * 
             * @return void* nullptr on error, otherwise valid trampoline to call
             */
            void* CreateTrampoline(void* p_OriginalFunction, HookType p_Type);

            /**
             * @brief Gets the template hook size
             * 
             * @param p_Type Type of hook
             * @return uint32_t Value on success, otherwise 0 for error
             */
            uint32_t GetTemplateHookSize(HookType p_Type);

            /**
             * @brief Gets the minimum amount of bytes to backup for the specified hook type
             * This will take in account not only the disassembly of the function, but also the type
             * of hook that will be applied
             * 
             * @param p_Target Target function to hook
             * @param p_Type The type of hook to use
             * @return uint32_t Minimum hook size, or 0 on error
             */
            uint32_t GetMinimumHookSize(void* p_Target, HookType p_Type);

        public:
            /**
             * @brief This function takes the start of a target function makes magic happen
             * to get the code detoured, this hides all of the nasty impl details of the hook
             * and allows us to hide behind that for future changes.
             * 
             * @param p_OriginFunc Original function to hook
             * @param p_HookFunction The detour function to run instead
             * @return void* Trampoline to call the original function, or nullptr on error
             */
            void* DetourFunction(void* p_OriginFunc, void* p_HookFunction);

            /**
             * @brief This one's a bit complicated
             * TODO: This will have to disassemble the call instruction
             * Get the relative offset, calculate the address to that
             * 
             * Check the target function to make sure that it can reach, otherwise
             * will need to create allocation nearby.
             * 
             * @param p_CallInstruction The call instruction to disassemble
             * @param p_TargetFunction The absolute address of the target function
             * @return void* Absolute address of the original function
             */
            void* DetourCall(void* p_CallInstruction, void* p_TargetFunction);
        };
    }
}