// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#pragma once
#include <Utils/Types.hpp>
#include <mira/Driver/DriverStructs.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/proc.h>
    #include <sys/mutex.h>
    
    #include <vm/vm.h>
    #include <vm/vm_page.h>
    #include <vm/pmap.h>
    #include <vm/vm_map.h>
    #include <vm/vm_param.h>
    
    #include <sys/conf.h>
};

namespace Mira
{
    namespace Driver
    {
        class SystemDriverCtl
        {
            // Driver Callbacks
        protected:
            // Memory
            static int32_t OnSystemAllocateProcessMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnSystemFreeProcessMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            static int32_t OnSystemReadProcessMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnSystemWriteProcessMemory(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            // Proc
            static int32_t OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetProcList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

        private:
            // Helper functions
            static bool GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result);
            static bool GetProcessList(MiraProcessList*& p_List);

        public:
            static int OnSystemDriverCtlIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        };
    }
}