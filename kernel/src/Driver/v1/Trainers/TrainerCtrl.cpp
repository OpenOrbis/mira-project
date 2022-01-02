#include "TrainerCtrl.hpp"

#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

using namespace Mira::Driver::v1;

int32_t TrainerCtrl::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    return EINVAL;
}