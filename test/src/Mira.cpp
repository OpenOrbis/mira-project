#include "Mira.hpp"
#include <Messaging/MessageManager.hpp>
#include <Rpc/Server.hpp>

Mira::Framework* Mira::Framework::m_Instance = nullptr;
typedef enum _Commands
{
    DbgCmd_None = 0x7AB56E31,
    DbgCmd_ReadMem = 0xF25FEE19,
    DbgCmd_WriteMem = 0x78B3A60C,
    DbgCmd_ProtectMem = 0x73FA541B,
    DbgCmd_ScanMem = 0xEDCCE6D4,
    DbgCmd_GetProcInfo = 0xF3B7D3F1,
    DbgCmd_AllocateProcMem = 0x16FE60FC,
    DbgCmd_FreeProcMem = 0x93E0CC76,
    DbgCmd_GetProcMap = 0x758DC819,
    DbgCmd_Attach = 0xFEFCF9C8,
    DbgCmd_Detach = 0xF3B1D649,
    DbgCmd_Breakpoint = 0xD60E69E4,
    DbgCmd_Watchpoint = 0x23DE0FCE,
    DbgCmd_GetProcThreads = 0x1F5290F2,
    DbgCmd_SignalProc = 0xA2E2610F,
    DbgCmd_GetRegs = 0x449EAA46,
    DbgCmd_SetRegs = 0xD70F129B,
    DbgCmd_GetThreadInfo = 0x51B931C2,
    DbgCmd_ThreadSinglestep = 0x080B3752,
    DbgCmd_ReadKernelMem = 0x844AE491,
    DbgCmd_WriteKernelMem = 0xCFD904E7,
    DbgCmd_GetProcList = 0x7CF61ABE
} Commands;

Mira::Framework* Mira::Framework::GetFramework()
{
    if (m_Instance == nullptr)
        m_Instance = new Mira::Framework();
    
    return m_Instance;
}

Mira::Framework::Framework()
{
    printf("asdadsa");
    
    WriteLog(LL_Error, "sadada");
}

static void MyCallback(Mira::Messaging::Rpc::Connection* p_Connection, const RpcTransport* p_Message)
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        printf("dsadsad\n");
        return;
    }

    Mira::Framework::GetFramework()->GetMessageManager()->SendErrorResponse(p_Connection, RPC_CATEGORY__DEBUG, -12);
}

bool Mira::Framework::Initialize()
{
    m_MessageManager = new Mira::Messaging::MessageManager();

    m_Server = new Mira::Messaging::Rpc::Server();
    if (m_Server)
        m_Server->OnLoad();

    m_MessageManager->RegisterCallback(RPC_CATEGORY__DEBUG, DbgCmd_GetProcList, MyCallback);
    return true;
}
