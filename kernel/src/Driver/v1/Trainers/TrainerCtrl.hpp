#pragma once
#include <Utils/Types.hpp>

struct thread;
struct cdev;

namespace Mira
{
    namespace Driver
    {
        namespace v1
        {
            class TrainerCtrl
            {
            public:
                static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            };
        }
    }
}