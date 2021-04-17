#pragma once

extern "C"
{
    #include <sys/types.h>
    #include <sys/eventhandler.h>
    #include <sys/module.h>
    #include <sys/proc.h>
    #include <sys/ioccom.h>
    #include <vm/vm.h>
    #include <sys/conf.h>
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
};

#include <Utils/Types.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Kernel.hpp>

#include <mira/MiraConfig.hpp>
#include <mira/Driver/DriverCmds.hpp>

#include <Driver/System/SystemDriverCtl.hpp>

/**
 * @brief Mira Control Driver
 * 
 * This driver is used for safely accessing kernel features while in usermode.
 * The idea behind this is to get rid of the kexec syscall all together once Mira is installed and allow only safe and tested access to kernel
 * 
 * Control Driver Design Decisions:
 * 
 * All inputs must be wrapped in a struct found in external/mira/Driver/DriverStructs.hpp
 * All outputs must be wrapped in a struct found in external/mira/Driver/DriverStructs.hpp
 * 
 * Each input if allowing get and set, must have a boolean/some kind of flag in the input structure for get or set
 * 
 * Each output if it's dynamic sized, will allocate process memory in the driver, and return a pointer back to the user to use
 * 
 * This can be freed with munmap after use
 */
namespace Mira
{
    namespace Driver
    {
        class CtrlDriver
        {
        private:
            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

            struct mtx m_Mutex;

        public:
            CtrlDriver();
            ~CtrlDriver();

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        
            static void OnProcessExec(void*, struct proc *p);
        protected:
            // Callback functions
            static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraThreadCredentials(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraSetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            static bool GetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials*& p_Output);
            static bool SetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials& p_Input);
        };
    }
}