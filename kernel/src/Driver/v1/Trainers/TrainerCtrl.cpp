#include "TrainerCtrl.hpp"

#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

#include <Trainers/TrainerManager.hpp>

#include <Utils/Logger.hpp>
#include <Mira.hpp>

using namespace Mira::Driver::v1;

int32_t TrainerCtrl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);

    if (p_Device == nullptr)
    {
        WriteLog(LL_Error, "invalid device.");
        return EINVAL;
    }

    if (p_Thread == nullptr)
    {
        WriteLog(LL_Error, "invalid thread.");
        return EINVAL;
    }

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return EPROCUNAVAIL;
    }

    auto s_TrainerManager = s_Framework->GetTrainerManager();
    if (s_TrainerManager == nullptr)
    {
        WriteLog(LL_Error, "could not get trainer manager.");
        return EPROCUNAVAIL;
    }

    // TODO: Split this shit up later, ain't no reason to leave it messy

    // Handle all of this other garbage
    switch (IOCGROUP(p_Command)) 
    {
        case MIRA_IOCTL_BASE:
        {
            // If we are handling Mira specific ioctl's
            switch (p_Command)
            {
            case MIRA_TRAINER_LOAD:
                {
                    WriteLog(LL_Debug, "tid: (%d) requesting trainer loading...", p_Thread->td_tid);
                    (void)s_TrainerManager->LoadTrainers(p_Thread);
                    
                    return 0;
                }
            case MIRA_TRAINER_GET_ENTRY_POINT:
                {
                    auto s_Proc = p_Thread->td_proc;
                    if (s_Proc == nullptr)
                    {
                        WriteLog(LL_Error, "could not get thread (%d) proc.", p_Thread->td_tid);
                        return EPROCUNAVAIL;
                    }
                    
                    auto s_EntryPoint = s_TrainerManager->GetEntryPoint(s_Proc->p_pid);
                    if (s_EntryPoint == nullptr)
                    {
                        WriteLog(LL_Error, "entry point not found for proc (%d).", s_Proc->p_pid);
                        return ESRCH;
                    }
                    
                    WriteLog(LL_Debug, "Found EntryPoint (%p).", s_EntryPoint);

                    copyout(&s_EntryPoint, p_Data, sizeof(s_EntryPoint));
                    return 0;
                }
            default:
                {
                    WriteLog(LL_Error, "TrainerManager driver cmd ioctl (0x%x) unknown.", p_Command);
                }
            }
        }
    }

    return EINVAL;
}