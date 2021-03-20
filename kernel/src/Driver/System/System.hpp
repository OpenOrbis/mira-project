#pragma once
#include <Utils/Types.hpp>

struct cdev;
struct thread;
struct proc;

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