#pragma once
#include <Utils/Types.hpp>
#include <Utils/Kernel.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/proc.h>

    #include <vm/vm.h>
    #include <vm/vm_page.h>
    #include <vm/pmap.h>
    #include <vm/vm_map.h>
    #include <vm/vm_param.h>
    
};

struct proc;

namespace Mira
{
    namespace Utils
    {
        typedef struct _UserProcessVmMap
        {
            uint64_t AllocatedSize;
            uint64_t EntryCount;
            ProcVmMapEntry Entries[];
            // Allocated Data below this
        } UserProcessVmMap;

        class System
        {
        public:
            /**
             * @brief Allocates new memory of a specified size and protection in a process
             * This pointer should not be touched directly, but used with copyout() due to it being userland
             * process memory.
             * 
             * if p_Size is < PAGE_SIZE it will be automatic rounded up to the nearest page (0x4000 on Orbis, 0x1000 on X).
             * 
             * NOTE: This function assumes that the proc is already locked with PROC_LOCK
             * @param p_Process Locked Process
             * @param p_Size Size to allocate
             * @param p_Protection Protection to set, defaults to VM_PROT_ALL (RWX)
             * @return uint8_t* nullptr on error, pointer to allocated memory otherwise
             */
            static uint8_t* AllocateProcessMemory(struct proc* p_Process, uint32_t p_Size, uint32_t p_Protection = VM_PROT_ALL);

            /**
             * @brief Frees previously allocated memory with the specified size
             * 
             * NOTE: This function assumes that the proc is already locked
             * @param p_Process Locked process
             * @param p_Address Address to free
             * @param p_Size Size to free
             */
            static void FreeProcessMemory(struct proc* p_Process, void* p_Address, uint32_t p_Size);
            
            /**
             * @brief Allocates and writes out a process vm map.
             * 
             * 
             * @param p_Process The process to get the map of
             * @param p_RequestingProcess The process who is requesting, if nullptr it assumes p_Process is the calling process
             * @return UserProcessVmMap* Address to UserProcessVmMap structure that's been filled out in p_RequestingProcess memory space
             */
            static UserProcessVmMap* GetUserProcessVmMap(struct proc* p_Process, struct proc* p_RequestingProcess = nullptr);

            static bool ReadProcessMemory(struct proc* p_TargetProcess, void* p_TargetAddress, void* p_Data, uint32_t p_DataLength);
            static bool WriteProcessMemory(struct proc* p_TargetProcess, void* p_TargetAddress, void* p_Data, uint32_t p_DataLength);
            static bool CopyProcessMemory(struct proc* p_SourceProcess, void* p_SourceAddress, struct proc* p_DestProcess, void* p_DestAddress, uint32_t p_Size);

            static bool ProtectMemory(struct proc* p_Process, void* p_Address, uint32_t p_Size, int32_t p_Protection);

        private:
            /**
             * @brief Port of flatz ReadWriteProc
             * 
             * @param p_TargetProcess Target process to read/write memory from
             * @param p_TargetAddres Address in target process to read/write memory from
             * @param p_Data Kernel in/out buffer
             * @param p_DataLength Length of kernel in/out buffer
             * @param p_Write Is this a write or read?
             * @return true Success, false otherwise
             */
            static bool ReadWriteProcessMemory(struct proc* p_TargetProcess, void* p_TargetAddres, void* p_Data, uint32_t p_DataLength, bool p_Write);
        };
    }
}