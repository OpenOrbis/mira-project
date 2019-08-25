#pragma once
#include <Utils/Types.hpp>
#include <sys/conf.h>

namespace Mira
{
    namespace Driver
    {
        class CtrlDriver
        {
        private:
            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

        public:
            CtrlDriver();
            ~CtrlDriver();

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        };
    }
}