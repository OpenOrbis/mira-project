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

#include <Plugins/Substitute/Substitute.hpp>
#include <mira/MiraConfig.hpp>
#include <mira/Driver/DriverCmds.hpp>

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
            static int32_t OnMiraGetProcInformation(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetProcList(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraThreadCredentials(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraSetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            // Helper functions
            static bool GetProcessInfo(int32_t p_ProcessId, MiraProcessInformation*& p_Result);
            static bool GetProcessList(MiraProcessList*& p_List);

            static bool GetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials*& p_Output);
            static bool SetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, MiraThreadCredentials& p_Input);
        };
    }
}