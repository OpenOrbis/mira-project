#include "Debugger2.hpp"
#include <Messaging/Rpc/rpc.pb-c.h>

using namespace Mira::Plugins;

Debugger2::Debugger2()
{
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__DEBUG, FileManager_Echo, OnEcho);
}

Debugger2::~Debugger2()
{
    //Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(RPC_CATEGORY__FILE, FileManager_Echo, OnEcho);
}