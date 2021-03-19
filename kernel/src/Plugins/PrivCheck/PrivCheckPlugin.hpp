#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

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

        protected:
            void SetBit(uint32_t p_BitIndex, bool p_Value);
            bool GetBit(uint32_t p_BitIndex);
            
        public:
            PrivCheckPlugin();
            ~PrivCheckPlugin();

            // Hooked function for priv_check
            static int PrivCheckHook(struct thread* td, int priv);

            // Hooked function for priv_check_cred
            static int PrivCheckCredHook(struct thread* td, int priv);

            
        };
    }
}