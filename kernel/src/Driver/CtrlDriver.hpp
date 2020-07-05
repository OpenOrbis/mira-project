#pragma once
#include <Utils/Types.hpp>
#include <sys/conf.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>
#include <Utils/_Syscall.hpp>

#include <Plugins/Substitute/Substitute.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>
};

namespace Mira
{
    namespace Driver
    {
        class CtrlDriver
        {
        private:
            eventhandler_entry* m_processStartHandler;
            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

        public:
            CtrlDriver();
            ~CtrlDriver();

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        
        protected:
            static void OnProcessStart(void *arg, struct proc *p);
        };
    }
}