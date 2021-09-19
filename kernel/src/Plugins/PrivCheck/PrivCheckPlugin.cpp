#include "PrivCheckPlugin.hpp"
#include <mira/Driver/DriverStructs.hpp>

#include <Utils/Logger.hpp>
#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

extern "C"
{
    #include <sys/priv.h>
};

using namespace Mira::Plugins;

PrivCheckPlugin::priv_check_cred_t PrivCheckPlugin::o_priv_check_cred = nullptr;

PrivCheckPlugin::PrivCheckPlugin()
{
    
    m_PrivCheckCredHook = subhook_new((void*)kdlsym(priv_check_cred), (void*)OnPrivCheckCred, subhook_flags_t::SUBHOOK_64BIT_OFFSET);
    o_priv_check_cred = (priv_check_cred_t)subhook_get_trampoline(m_PrivCheckCredHook);
    WriteLog(LL_Warn, "priv_check_cred hook created!");
}

PrivCheckPlugin::~PrivCheckPlugin()
{
}

bool PrivCheckPlugin::OnLoad()
{
    subhook_install(m_PrivCheckCredHook);
    WriteLog(LL_Warn, "priv_check_hook installed.");

    return true;
}

bool PrivCheckPlugin::OnUnload()
{
    subhook_remove(m_PrivCheckCredHook);
    WriteLog(LL_Warn, "priv_check_hook removed.");

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

int PrivCheckPlugin::OnPrivCheckCred(struct ucred* p_Cred, int p_Priv)
{
    // Unrestrict all SCE calls
    if (p_Priv >= PRIV_SCE_0)
        return 0;
    
    // Call the original
    auto s_Ret = o_priv_check_cred(p_Cred, p_Priv);

    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
    {
        WriteLog(LL_Error, "could not get framework.");
        return s_Ret;
    }
    
    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin manager.");
        return s_Ret;
    }
    
    auto s_PrivCheckPlugin = reinterpret_cast<PrivCheckPlugin*>(s_PluginManager->GetPrivCheck());
    if (s_PrivCheckPlugin == nullptr)
    {
        WriteLog(LL_Error, "could not get plugin.");
        return s_Ret;
    }

    if (s_Ret != 0 && (p_Priv != PRIV_VFS_BLOCKRESERVE && p_Priv != PRIV_VFS_CHROOT))
        WriteLog(LL_Info, "pcc: ret: (%d) priv: (%d).", s_Ret, p_Priv);
    
    return s_Ret;
}