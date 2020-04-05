#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Boot
    {
        class OptionsBlock
        {
        private:
            /**
             * @brief IsAdvancedMode allows super-users to enable more dangerous features on their consoles
             * 
             * This will allow to configure automatic syscall hooking, rpc validation, and other features
             */
            bool IsAdvancedMode;

            // Advanced Mode Settings
            bool RpcValidationEnabled;
            bool SyscallGuardEnabled;
            bool FakeSelfEnabled;
            bool FakePkgEnabled;
            bool DebuggerBreakOnAttach;
            bool AslrEnabled;
            bool FuseEnabled;

            bool LogServerEnabled;
            uint16_t LogServerPort;

            bool RpcServerEnabled;
            uint16_t RpcServerPort;

        public:
            void SetDefaults();
        };
    }
}