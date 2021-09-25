// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CtrlDriver.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <Mira.hpp>

using namespace Mira::Driver;

int32_t CtrlDriver::OnMiraGetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);

    // Validate the device driver
    if (p_Device == nullptr)
        return ENXIO;
    
    // Validate framework
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return ENOMEM;
    
    auto s_Configuration = s_Framework->GetConfiguration();
    if (s_Configuration == nullptr)
        return ENOMEM;
    
    auto s_Result = copyout(s_Configuration, p_Data, sizeof(*s_Configuration));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyout the configuration block (%d).", s_Result);
        return EINVAL;
    }

    return 0;
}

int32_t CtrlDriver::OnMiraSetConfig(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    // Validate device
    if (p_Device == nullptr)
        return ENXIO;
    
    // Get the framework
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return ENOMEM;
    
    // Copyin the configuration into kernel space
    MiraConfig s_Configuration = { 0 };
    auto s_Result = copyin(p_Data, &s_Configuration, sizeof(s_Configuration));
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not copyin configuration (%d).", s_Result);
        return EINVAL;
    }

    // Set the framework configuration
    if (!s_Framework->SetConfiguration(&s_Configuration))
    {
        WriteLog(LL_Error, "could not set configuration.");
        return EINVAL;
    }
    
    return 0;
}