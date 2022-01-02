#pragma once
#include <Utils/Types.hpp>

extern "C"
{
    #include <sys/proc.h>
};

namespace Mira
{
    namespace Driver
    {
        namespace Processes
        {
            class ProcessCtrl
            {
            public:
                static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

                static bool GetProcessList();
                static bool GetProcessMainThread();
            };
        }
    }
}