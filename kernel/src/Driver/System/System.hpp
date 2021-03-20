#pragma once
#include <Utils/Types.hpp>

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

namespace Mira
{
    namespace Driver
    {
        class SystemDriverCtl
        {
        protected:
            static void* ProcessAllocate(struct proc* p_Process, uint64_t p_Size);
            
        public:
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        };
    }
}