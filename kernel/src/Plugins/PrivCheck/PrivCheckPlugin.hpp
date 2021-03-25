#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

extern "C"
{
    #include <vm/vm.h>
    #include <sys/conf.h>
    #include <sys/param.h>
    #include <sys/proc.h>
}

namespace Mira
{
    namespace Plugins
    {
        class PrivCheckPlugin :
            public Utils::IModule
        {
        private:
            enum
            {
                MaxProcThreads = 20,
                MaxBitCount = 1024,
                MaskSizeInBytes = MaxBitCount / 8,
            };

            typedef struct _ThreadPriv
            {
                // Process Id
                int32_t ThreadId;

                // Bitmask of all of the privs
                // The way this works is each priv_check, will check into this mask.
                // if a bit is set to 1, we force return "success" for that priv
                // if a bit is set to 0, we call the original function and return the value
                uint8_t Mask[MaskSizeInBytes];
            } ThreadPriv;

            // The proc's we are overriding
            ThreadPriv m_Privs[MaxProcThreads];

            Utils::Hook* m_PrivCheckHook;

        protected:
            MIRA_DECLARE_HOOK(int, priv_check, struct thread* td, int priv);

        protected:
            void SetBit(int32_t p_ThreadId, uint32_t p_BitIndex, bool p_Value);
            bool GetBit(int32_t p_ThreadId, uint32_t p_BitIndex);

            bool SetMask(int32_t p_ThreadId, uint8_t p_Mask[MaskSizeInBytes]);

            /**
             * @brief This will search the currently tracked privs and see if one matches the pid
             * 
             * @param p_ThreadId Thread id
             * @return ProcPriv* nullptr if not found, otherwise privs
             */
            ThreadPriv* FindPrivByThreadId(int32_t p_ThreadId);

            /**
             * @brief This is a helper function that will get or create the priv
             * 
             * @param p_ThreadId Thread id 
             * @return ProcPriv* nullptr on error
             */
            ThreadPriv* GetOrCreatePrivByThreadId(int32_t p_ThreadId);
            
        public:
            PrivCheckPlugin();
            ~PrivCheckPlugin();

            /**
             * @brief Hooked function for priv_check
             * 
             * @param td Thread of privs to check
             * @param priv Priv to check
             * @return int 0 on success, error otherwise (see: priv_check documentation)
             */
            static int PrivCheckHook(struct thread* td, int priv);

            /**
             * @brief Priv check ioctl handler
             * 
             * This handles requests from user mode to get/set priv overriding
             * @param p_Device 
             * @param p_Command 
             * @param p_Data 
             * @param p_FFlag 
             * @param p_Thread 
             * @return int 
             */
            static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            bool OnLoad() override;
            bool OnUnload() override;
        };
    }
}