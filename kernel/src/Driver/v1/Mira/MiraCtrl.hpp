#pragma once
#include <Utils/Types.hpp>

struct cdev;
struct thread;

namespace Mira
{
    namespace Driver
    {
        namespace v1
        {
            class MiraCtrl
            {
            protected:
                static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
                
            public:
                static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            };
        }
    }
}