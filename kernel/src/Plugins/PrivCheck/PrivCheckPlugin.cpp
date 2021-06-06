#include "PrivCheckPlugin.hpp"
#include <mira/Driver/DriverStructs.hpp>

#include <Utils/Logger.hpp>
#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>


using namespace Mira::Plugins;

PrivCheckPlugin::priv_check_t PrivCheckPlugin::o_priv_check = nullptr;

PrivCheckPlugin::PrivCheckPlugin() :
    m_PrivCheckHook(nullptr)
{
    memset(m_Privs, 0, sizeof(m_Privs));
}

PrivCheckPlugin::~PrivCheckPlugin()
{
}

bool PrivCheckPlugin::OnLoad()
{
    if (m_PrivCheckHook == nullptr)
        m_PrivCheckHook = new Utils::Hook(kdlsym(priv_check), reinterpret_cast<void*>(PrivCheckHook));
    
    o_priv_check = reinterpret_cast<priv_check_t>(m_PrivCheckHook->GetTrampoline());
    if (o_priv_check == nullptr)
    {
        WriteLog(LL_Error, "priv check broke.");
        return false;
    }

    if (!m_PrivCheckHook->Enable())
    {
        WriteLog(LL_Error, "could not enable priv check hook.");
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
    
    return true;
}

bool PrivCheckPlugin::SetMask(int32_t p_ThreadId, uint8_t p_Mask[MaxPrivCount])
{
    auto s_Priv = FindPrivByThreadId(p_ThreadId);
    if (s_Priv == nullptr)
        return false;
    
    // Copy the mask from the input to the tracked copy
    memcpy(s_Priv->Mask, p_Mask, ARRAYSIZE(s_Priv->Mask));

    return true;
}

void PrivCheckPlugin::SetBit(int32_t p_ThreadId, uint32_t p_PrivIndex, bool p_Value)
{
    auto s_Priv = FindPrivByThreadId(p_ThreadId);
    if (s_Priv == nullptr)
        return;
    
    if (p_PrivIndex >= ARRAYSIZE(s_Priv->Mask))
        return;
    
    s_Priv->Mask[p_PrivIndex] = p_Value;
}

bool PrivCheckPlugin::GetBit(int32_t p_ThreadId, uint32_t p_PrivIndex)
{
    auto s_Priv = FindPrivByThreadId(p_ThreadId);
    if (s_Priv == nullptr)
        return false;
    
    if (p_PrivIndex >= ARRAYSIZE(s_Priv->Mask))
        return false;
    
    return s_Priv->Mask[p_PrivIndex] != 0;
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
    // NOTE: This happens if incoming pid is <= 0 or if the specified thread id matches current pid
    if (s_PrivCheck.ThreadId <= 0 ||
        p_Thread->td_tid == s_PrivCheck.ThreadId)
    {
        // Set the output thread id
        s_PrivCheck.ThreadId = p_Thread->td_tid;
    }

    // Handle getting
    if (s_PrivCheck.IsGet)
    {
        // Attempt to find the privs
        auto s_Priv = s_PrivCheckPlugin->FindPrivByThreadId(s_PrivCheck.ThreadId);
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
        auto s_Priv = s_PrivCheckPlugin->GetOrCreatePrivByThreadId(s_PrivCheck.ThreadId);
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

PrivCheckPlugin::ThreadPriv* PrivCheckPlugin::FindPrivByThreadId(int32_t p_ThreadId)
{
    if (p_ThreadId <= 0)
        return nullptr;
    
    for (auto l_Index = 0; l_Index < ARRAYSIZE(m_Privs); ++l_Index)
    {
        PrivCheckPlugin::ThreadPriv* l_Priv = &m_Privs[l_Index];
        if (l_Priv->ThreadId == p_ThreadId)
            return l_Priv;
    }

    return nullptr;
}

PrivCheckPlugin::ThreadPriv* PrivCheckPlugin::GetOrCreatePrivByThreadId(int32_t p_ThreadId)
{
    // Find an existing priv by thread id
    auto l_Priv = FindPrivByThreadId(p_ThreadId);
    if (l_Priv == nullptr)
    {
        // Iterate all of the privs looking for a free slot
        for (auto l_Index = 0; l_Index < ARRAYSIZE(m_Privs); ++l_Index)
        {
            // If the thread id <= 0
            if (m_Privs[l_Index].ThreadId <= 0)
            {
                m_Privs[l_Index].ThreadId = p_ThreadId;
                return &m_Privs[l_Index];
            }
        }

        WriteLog(LL_Error, "could not find open priv slot.");
        return nullptr;
    }

    return l_Priv;
}

int PrivCheckPlugin::PrivCheckHook(struct thread* p_Thread, int p_Priv)
{
    // Call the original
    auto s_Ret = o_priv_check(p_Thread, p_Priv);

    auto s_Framaework = Mira::Framework::GetFramework();
    if (s_Framaework == nullptr)
        return s_Ret;
    
    auto s_PluginManager = s_Framaework->GetPluginManager();
    if (s_PluginManager == nullptr)
        return s_Ret;
    
    auto s_PrivCheckPlugin = reinterpret_cast<PrivCheckPlugin*>(s_PluginManager->GetPrivCheck());
    if (s_PrivCheckPlugin == nullptr)
        return s_Ret;
    
    auto s_Priv = s_PrivCheckPlugin->FindPrivByThreadId(p_Thread->td_tid);
    if (s_Priv == nullptr)
        return s_Ret;


    const uint8_t* s_Mask = s_Priv->Mask;
    if (p_Priv >= ARRAYSIZE(s_Priv->Mask))
    {
        WriteLog(LL_Error, "attempted to priv index out of bounds idx: (%d).", p_Priv);
        return s_Ret;
    }

    // Not sure if this is correct
    uint8_t s_Bit = s_Mask[p_Priv];
    
    // If the bit is set to override we force return success here
    if (s_Bit != 0)
        return 0;

    WriteLog(LL_Error, "o_priv_check ret: (%d).", s_Ret);
    
    // Return the result
    return s_Ret;
}