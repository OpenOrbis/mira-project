#include "v1.hpp"
#include "Mira/MiraCtrl.hpp"
#include "Processes/ProcessCtrl.hpp"
#include "Trainers/TrainerCtrl.hpp"

using namespace Mira::Driver::v1;

int v1Ctrl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto s_Ret = ProcessCtrl::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    if (s_Ret == 0)
        return 0;
    
    s_Ret = MiraCtrl::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    if (s_Ret == 0)
        return 0;
    
    s_Ret = TrainerCtrl::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
    if (s_Ret == 0)
        return 0;
    

    switch (IOCGROUP(p_Command)) 
    {
        case MIRA_IOCTL_BASE:
        {
            // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
                /*case MIRA_PROCESS_THREAD_READ_PRIVILEGE_MASK:
                case MIRA_PROCESS_THREAD_WRITE_PRIVILEGE_MASK:
                    return Plugins::PrivCheckPlugin::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_PROCESS_FIND_IMPORT_ADDRESS:
                    return Plugins::Debugger::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                default:
                    WriteLog(LL_Debug, "mira base unknown command: (0x%llx).", p_Command);
                    break;*/
            }
        }
    }

    return EINVAL;
}