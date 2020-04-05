#include "OptionsBlock.hpp"

using namespace Mira::Boot;

void OptionsBlock::SetDefaults()
{
    IsAdvancedMode = false;

    RpcValidationEnabled = true;
    SyscallGuardEnabled = true;
    FakeSelfEnabled = true;
    FakePkgEnabled = true;

    DebuggerBreakOnAttach = false;
    AslrEnabled = true;
    FuseEnabled = false;

    LogServerEnabled = false;
    LogServerPort = 9998;

    RpcServerEnabled = false;
    RpcServerPort = 9999;
}