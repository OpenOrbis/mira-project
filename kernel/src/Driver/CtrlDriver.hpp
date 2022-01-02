// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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
#include <mira/Driver/DriverStructs.hpp>

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
 * All ioctls that require an output parameter must provide an address in local process space that will hold the returned pointer data
 * 
 * Example.
 * 
 * {
 *      MyInputStruct s_MyStruct = { 0 };
 *      void* s_IoctlData = &s_MyStruct;
 *      if (ioctl(s_DeviceFd, MY_IOCTL_CODE, &s_IoctlData) == 0)
 *      {
 *          // s_IoctlData will contain a new pointer or nullptr
 *          // use s_IoctlData will now be
 *          // MyOutputStruct* s_IoctlData = someallocatedaddr;
 *          // After finished using you must call the ioctl for FreeAllocatedData(s_IoctlData)
 *      }
 * }
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
            enum
            {
                MaxApiInfoCount = 16,
                InvalidId = -1,
            };

            // The open flags determine which API this is going to run from
            typedef enum _CtrlDriverFlags
            {
                // Run the default latest API provided by Mira
                Latest = 0,

                // Initial v1 API
                v1 = 1,

                // Total API flag Count
                COUNT
            } CtrlDriverFlags;

            typedef struct _ApiInfo
            {
                // Thread id
                int32_t ThreadId;

                // Api version
                CtrlDriverFlags Version;
            } ApiInfo;

            // Upper limit of available api infos
            uint32_t m_MaxApiInfoCount;
            ApiInfo* m_ApiInfos;

            struct cdevsw m_DeviceSw;
            struct cdev* m_Device;

            struct mtx m_Mutex;

            uint32_t m_Versions;

        public:
            CtrlDriver();
            ~CtrlDriver();

            static int32_t OnOpen(struct cdev* p_Device, int32_t p_OFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnClose(struct cdev* p_Device, int32_t p_FFlags, int32_t p_DeviceType, struct thread* p_Thread);
            static int32_t OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
        
            static void OnProcessExec(void*, struct proc *p);

        private:
            /**
             * @brief Determines if the CtrlDriver is ready to start accepting new clients
             * 
             * NOTE: This checks if the m_ApiInfos has been allocated
             * @return true on success, false otherwise 
             */
            bool IsInitialized() const { return m_ApiInfos != nullptr && m_MaxApiInfoCount > 0; }

            /**
             * @brief Finds an available slot index for ApiInfo array
             * 
             * @return int32_t -1 on error, success otherwise
             */
            int32_t FindFreeApiInfoIndex() const;

            /**
             * @brief Finds an ApiInfo index by thread id
             * 
             * @param p_ThreadId Caller thread id
             * @return int32_t -1 on error, success otherwise
             */
            int32_t FindApiInfoByThreadId(int32_t p_ThreadId);
            void ClearApiInfo(uint32_t p_Index);
            void DestroyAllApiInfos();


        protected:
            // Callback functions
            static int32_t OnMiraMountInSandbox(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraThreadCredentials(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraGetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);
            static int32_t OnMiraSetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            static bool GetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, ProcessThreadReadCredentials*& p_Output);
            static bool SetThreadCredentials(int32_t p_ProcessId, int32_t p_ThreadId, ProcessThreadWriteCredentials& p_Input);
        };
    }
}