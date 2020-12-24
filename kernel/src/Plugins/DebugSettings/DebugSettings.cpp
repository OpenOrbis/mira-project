#include "DebugSettings.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/sysent.h>

using namespace Mira::Plugins;

struct sysctl_uap
{
    int* name;
    int name_len;
    void* old_buf;
    size_t* old_len;
    void* new_buf;
    size_t new_len;
};

static int (*real_sysctl)(thread*, sysctl_uap*) = 0;
static int enabled = 0;

static int sysctl_hook(thread* td, sysctl_uap* uap)
{
    int fake_ans = 1;
    auto copyin = (int(*)(const void*, void*, size_t))kdlsym(copyin);
    auto copyout = (int(*)(const void*, void*, size_t))kdlsym(copyout);
    int ans;
    if(!enabled)
        goto passthrough;
    if(memcmp(td->td_proc->p_comm, "SceShellUI", sizeof("SceShellUI")))
        goto passthrough;
    if(uap->name_len != 2 || !uap->name || !uap->old_buf || !uap->old_len || uap->new_buf || uap->new_len)
        goto passthrough;
    int name[2];
    if(copyin(uap->name, &name, sizeof(name)))
        goto passthrough;
    if(name[0] != 7 || (name[1] != 276 && name[1] != 286))
        goto passthrough;
    size_t old_len;
    if(copyin(uap->old_len, &old_len, sizeof(old_len)) || old_len != sizeof(fake_ans))
        goto passthrough;
    ans = real_sysctl(td, uap);
    if(ans)
        return ans;
    copyout(&fake_ans, uap->old_buf, sizeof(fake_ans));
    return 0;
passthrough:
    return real_sysctl(td, uap);
}

DebugSettingsActivator::DebugSettingsActivator(){}

DebugSettingsActivator::~DebugSettingsActivator(){}

bool DebugSettingsActivator::OnLoad()
{
    if(!real_sysctl)
    {
        auto sysents = (struct sysent*)kdlsym(kern_sysents);
        real_sysctl = (int(*)(thread*, sysctl_uap*))sysents[SYS___sysctl].sy_call;
        sysents[SYS___sysctl].sy_call = (int(*)(thread*, void*))&sysctl_hook;
        WriteLog(LL_Info, "DebugSettingsActivator: sysctl hooked");
    }
    enabled = 1;
    return true;
}

bool DebugSettingsActivator::OnUnload()
{
    enabled = 0;
    return false; // we did not really terminate
}

bool DebugSettingsActivator::OnSuspend()
{
    return false;
}

bool DebugSettingsActivator::OnResume()
{
    return false;
}
