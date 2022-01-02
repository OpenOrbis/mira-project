#include "v1.hpp"

using namespace Mira::Driver::v1;

int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    switch (IOCGROUP(p_Command)) 
    {
        case MIRA_IOCTL_BASE:
        {
                // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
                case MIRA_PROCESS_READ_MEMORY:
                case MIRA_PROCESS_WRITE_MEMORY:
                //case MIRA_GET_PROC_INFORMATION:
                case MIRA_PROCESS_LIST:
                case MIRA_PROCESS_ALLOCATE_MEMORY:
                case MIRA_PROCESS_FREE_MEMORY:
                case MIRA_MOUNT_IN_SANDBOX:
                    return OnMiraMountInSandbox(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                // Get/set the thread credentials
                case MIRA_PROCESS_THREAD_READ_CREDENTIALS:
                case MIRA_PROCESS_THREAD_WRITE_CREDENTIALS:
                    return OnMiraThreadCredentials(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                
                /*case MIRA_TRAINER_CREATE_SHM:
                case MIRA_TRAINER_GET_SHM:*/
                case MIRA_TRAINER_LOAD:
                case MIRA_TRAINER_GET_ENTRY_POINT:
                    return Mira::Trainers::TrainerManager::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);

                case MIRA_READ_CONFIG:
                    return OnMiraGetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_WRITE_CONFIG:
                    return OnMiraSetConfig(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_PROCESS_THREAD_READ_PRIVILEGE_MASK:
                case MIRA_PROCESS_THREAD_WRITE_PRIVILEGE_MASK:
                    return Plugins::PrivCheckPlugin::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                case MIRA_PROCESS_FIND_IMPORT_ADDRESS:
                    return Plugins::Debugger::OnIoctl(p_Device, p_Command, p_Data, p_FFlag, p_Thread);
                default:
                    WriteLog(LL_Debug, "mira base unknown command: (0x%llx).", p_Command);
                    break;
            }
        }
    }
}