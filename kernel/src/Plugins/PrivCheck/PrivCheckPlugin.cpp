#include "PrivCheckPlugin.hpp"
#include <mira/Driver/DriverStructs.hpp>

#include <Utils/Logger.hpp>
#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>


using namespace Mira::Plugins;

PrivCheckPlugin::PrivCheckPlugin() :
    m_Privs { 0 },
    m_PrivCheckHook(nullptr),
    m_PrivCheckCredHook(nullptr)
{

}

PrivCheckPlugin::~PrivCheckPlugin()
{
    
}

bool PrivCheckPlugin::OnLoad()
{
    if (m_PrivCheckHook == nullptr)
        m_PrivCheckHook = new Utils::Hook(kdlsym(priv_check), reinterpret_cast<void*>(PrivCheckHook));
    
    if (m_PrivCheckCredHook == nullptr)
        m_PrivCheckCredHook = new Utils::Hook(kdlsym(priv_check_cred), reinterpret_cast<void*>(PrivCheckCredHook));
    
    if (!m_PrivCheckHook->Enable())
    {
        WriteLog(LL_Error, "could not enable priv check hook.");
        return false;
    }

    if (!m_PrivCheckCredHook->Enable())
    {
        WriteLog(LL_Error, "could not enable priv check cred hook.");
        return false;
    }
    return true;
}

bool PrivCheckPlugin::OnUnload()
{
    if (m_PrivCheckHook)
    {
        m_PrivCheckHook->Disable();
        delete m_PrivCheckHook;
        m_PrivCheckHook = nullptr;
    }

    if (m_PrivCheckCredHook)
    {
        m_PrivCheckCredHook->Disable();
        delete m_PrivCheckCredHook;
        m_PrivCheckCredHook = nullptr;
    }
    
    return true;
}

bool PrivCheckPlugin::SetMask(int32_t p_ProcessId, uint8_t p_Mask[MaskSizeInBytes])
{
    auto s_Priv = FindPrivByProcessId(p_ProcessId);
    if (s_Priv == nullptr)
        return false;
    
    // Copy the mask from the input to the tracked copy
    memcpy(s_Priv->Mask, p_Mask, ARRAYSIZE(s_Priv->Mask));

    return true;
}

int PrivCheckPlugin::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);

    if (p_Device == nullptr || p_Data == 0)
        return EINVAL;

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return ENOMEM;
    
    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
        return ENOMEM;
    
    PrivCheckPlugin* s_PrivCheckPlugin = reinterpret_cast<PrivCheckPlugin*>(s_PluginManager->GetPrivCheck());
    if (s_PrivCheckPlugin == nullptr)
        return ENOMEM;
    
    MiraPrivCheck s_PrivCheck = { 0 };
    auto s_Ret = copyin(p_Data, &s_PrivCheck, sizeof(s_PrivCheck));
    if (s_Ret != 0)
        return s_Ret;
    
    // Check to see if we are using the "current proc"
    // NOTE: This happens if incoming pid is <= 0 or if the specified process id matches current pid
    if (s_PrivCheck.ProcessId <= 0 ||
        p_Thread->td_proc->p_pid == s_PrivCheck.ProcessId)
    {
        // Set the output process id
        s_PrivCheck.ProcessId = p_Thread->td_proc->p_pid;
    }

    // Handle getting
    if (s_PrivCheck.IsGet)
    {
        // Attempt to find the privs
        auto s_Priv = s_PrivCheckPlugin->FindPrivByProcessId(s_PrivCheck.ProcessId);
        if (s_Priv == nullptr)
        {
            // If no privs are being tracked/found
            // Zero all of the permissions
            memset(s_PrivCheck.Mask, 0, ARRAYSIZE(s_PrivCheck.Mask));
        }
        else
        {
            // Copy the bitmask
            memcpy(s_PrivCheck.Mask, s_Priv->Mask, ARRAYSIZE(s_PrivCheck.Mask));
        }
    }
    else
    {
        // Handle setting the privs
        auto s_Priv = s_PrivCheckPlugin->GetOrCreatePrivByProcessId(s_PrivCheck.ProcessId);
        if (s_Priv == nullptr)
        {
            WriteLog(LL_Error, "could not get or create priv by pid.");
            return ENOMEM;
        }

        memcpy(s_Priv->Mask, s_PrivCheck.Mask, ARRAYSIZE(s_Priv->Mask));
    }

    // Copy the data out to userland
    s_Ret = copyout(&s_PrivCheck, p_Data, sizeof(s_PrivCheck));
    if (s_Ret != 0)
        WriteLog(LL_Error, "could not copy out the priv check structure.");
    
    return s_Ret;
}

PrivCheckPlugin::ProcPriv* PrivCheckPlugin::FindPrivByProcessId(int32_t p_ProcessId)
{
    if (p_ProcessId <= 0)
        return nullptr;
    
    for (auto l_Index = 0; l_Index < ARRAYSIZE(m_Privs); ++l_Index)
    {
        PrivCheckPlugin::ProcPriv* l_Priv = &m_Privs[l_Index];
        if (l_Priv->ProcessId == p_ProcessId)
            return l_Priv;
    }

    return nullptr;
}

PrivCheckPlugin::ProcPriv* PrivCheckPlugin::GetOrCreatePrivByProcessId(int32_t p_ProcessId)
{
    // Find an existing priv by process id
    auto l_Priv = FindPrivByProcessId(p_ProcessId);
    if (l_Priv == nullptr)
    {
        // Iterate all of the privs looking for a free slot
        for (auto l_Index = 0; l_Index < ARRAYSIZE(m_Privs); ++l_Index)
        {
            // If the process id <= 0
            if (m_Privs[l_Index].ProcessId <= 0)
            {
                m_Privs[l_Index].ProcessId = p_ProcessId;
                return &m_Privs[l_Index];
            }
        }

        WriteLog(LL_Error, "could not find open priv slot.");
        return nullptr;
    }

    return l_Priv;
}

int PrivCheckPlugin::PrivCheckHook(struct thread* td, int priv)
{
    PrivCheckPlugin* s_PrivCheckPlugin = nullptr;
    do
    {
        auto s_Framaework = Mira::Framework::GetFramework();
        if (s_Framaework == nullptr)
            break;
        
        auto s_PluginManager = s_Framaework->GetPluginManager();
        if (s_PluginManager == nullptr)
            break;
        
        s_PrivCheckPlugin = reinterpret_cast<PrivCheckPlugin*>(s_PluginManager->GetPrivCheck());
        if (s_PrivCheckPlugin == nullptr)
            break;
        
        auto s_Priv = s_PrivCheckPlugin->FindPrivByProcessId(td->td_proc->p_pid);
        if (s_Priv == nullptr)
            break;


        const uint8_t* s_Mask = s_Priv->Mask;
        // Calculate where in the mask we need to check
        uint32_t s_Index = priv / 8;
        uint32_t s_BitShift = priv % 8;
        if (s_Index >= ARRAYSIZE(s_Mask))
        {
            WriteLog(LL_Error, "attempted to index out of bounds idx: (%d).", s_Index);
            return 0;
        }

        // Not sure if this is correct
        uint8_t s_Bit = (s_Mask[s_Index] >> s_BitShift) & 1;
        
        // If the bit is set to override we force return true here
        if (s_Bit == 1)
            return 0;
 
    } while (false);
    
    // If there was no override found then call the original
    if (s_PrivCheckPlugin == nullptr)
        return 0;
    
    // Call original
    auto s_Address = s_PrivCheckPlugin->m_PrivCheckHook->GetOriginalFunctionAddress();
    auto s_Function = (int(*)(struct thread*, int))s_Address;
    if (s_Function == nullptr)
        return 0;
    
    s_PrivCheckPlugin->m_PrivCheckHook->Disable();
    auto s_Ret = s_Function(td, priv);
    s_PrivCheckPlugin->m_PrivCheckHook->Enable();

    return s_Ret;
}

int PrivCheckPlugin::PrivCheckCredHook(struct thread* td, int priv)
{
    PrivCheckPlugin* s_PrivCheckPlugin = nullptr;
    do
    {
        auto s_Framaework = Mira::Framework::GetFramework();
        if (s_Framaework == nullptr)
            break;
        
        auto s_PluginManager = s_Framaework->GetPluginManager();
        if (s_PluginManager == nullptr)
            break;
        
        s_PrivCheckPlugin = reinterpret_cast<PrivCheckPlugin*>(s_PluginManager->GetPrivCheck());
        if (s_PrivCheckPlugin == nullptr)
            break;
        
        auto s_Priv = s_PrivCheckPlugin->FindPrivByProcessId(td->td_proc->p_pid);
        if (s_Priv == nullptr)
            break;


        const uint8_t* s_Mask = s_Priv->Mask;
        // Calculate where in the mask we need to check
        uint32_t s_Index = priv / 8;
        uint32_t s_BitShift = priv % 8;
        if (s_Index >= ARRAYSIZE(s_Mask))
        {
            WriteLog(LL_Error, "attempted to index out of bounds idx: (%d).", s_Index);
            break;;
        }

        // Not sure if this is correct
        uint8_t s_Bit = (s_Mask[s_Index] >> s_BitShift) & 1;
        
        // If the bit is set to override we force return true here
        if (s_Bit == 1)
            return 0;
 
    } while (false);
    
    // If there was no override found then call the original
    if (s_PrivCheckPlugin == nullptr)
        return 0;
    
    auto s_Hook = s_PrivCheckPlugin->m_PrivCheckCredHook;
    if (s_Hook == nullptr)
        return 0;
    
    // Get original function address
    auto s_Address = s_Hook->GetOriginalFunctionAddress();
    auto s_Function = (int(*)(struct thread*, int))s_Address;
    if (s_Function == nullptr)
        return 0;
    
    // Disable the hook
    if (!s_Hook->Disable())
    {
        WriteLog(LL_Error, "failed to disable priv check cred hook.");
        return 0;
    }
    
    // Call original
    auto s_Ret = s_Function(td, priv);

    // Re-enable the hook
    if (!s_Hook->Enable())
    {
        WriteLog(LL_Error, "failed to enable priv check cred hook.");
        return 0;
    }
    
    // Return the result
    return s_Ret;
}