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
                MaxProcPrivs = 8,
                MaxBitCount = 1024,
                MaskSizeInBytes = MaxBitCount / 8,
            };

            typedef struct _ProcPriv
            {
                // Process Id
                int32_t ProcessId;

                // Bitmask of all of the privs
                // The way this works is each priv_check, will check into this mask.
                // if a bit is set to 1, we force return "success" for that priv
                // if a bit is set to 0, we call the original function and return the value
                uint8_t Mask[MaskSizeInBytes];
            } ProcPriv;

            // The proc's we are overriding
            ProcPriv m_Privs[MaxProcPrivs];

            Utils::Hook* m_PrivCheckHook;
            Utils::Hook* m_PrivCheckCredHook;

        protected:
            void SetBit(int32_t p_ProcessId, uint32_t p_BitIndex, bool p_Value);
            bool GetBit(int32_t p_ProcessId, uint32_t p_BitIndex);

            bool SetMask(int32_t p_ProcessId, uint8_t p_Mask[MaskSizeInBytes]);

            /**
             * @brief This will search the currently tracked privs and see if one matches the pid
             * 
             * @param p_ProcessId ProcessId
             * @return ProcPriv* nullptr if not found, otherwise privs
             */
            ProcPriv* FindPrivByProcessId(int32_t p_ProcessId);

            /**
             * @brief This is a helper function that will get or create the priv
             * 
             * @param p_ProcessId 
             * @return ProcPriv* nullptr on error
             */
            ProcPriv* GetOrCreatePrivByProcessId(int32_t p_ProcessId);
            
        public:
            PrivCheckPlugin();
            ~PrivCheckPlugin();

            // Hooked function for priv_check
            static int PrivCheckHook(struct thread* td, int priv);

            // Hooked function for priv_check_cred
            static int PrivCheckCredHook(struct thread* td, int priv);

            static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

            bool OnLoad() override;
            bool OnUnload() override;
        };
    }
}