// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "TTYRedirector.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

#include <sys/conf.h>

using namespace Mira::Plugins;

TTYRedirector::TTYRedirector() :
m_DeciTTYWrite_orig(nullptr)
{
}

TTYRedirector::~TTYRedirector()
{
}


bool TTYRedirector::OnLoad()
{
    cpu_disable_wp();
    m_DeciTTYWrite_orig = *(void**)(kdlsym(printf_hook));
    *(void**)(kdlsym(printf_hook)) = reinterpret_cast<void*>(OnDeciTTYWrite);
    cpu_enable_wp();

    WriteLog(LL_Info, "TTY Redirection enabled.");
    return true;
}

bool TTYRedirector::OnUnload()
{
    if (m_DeciTTYWrite_orig) {
        cpu_disable_wp();
        *(void**)(kdlsym(printf_hook)) = m_DeciTTYWrite_orig;
        cpu_enable_wp();
    }

    WriteLog(LL_Info, "TTY Redirection disabled.");
    return true;
}

bool TTYRedirector::OnSuspend()
{
    return true;
}

bool TTYRedirector::OnResume()
{
    return true;
}

int TTYRedirector::OnDeciTTYWrite(struct cdev* dev, struct uio* uio, int ioflag) {
   struct uio* cloned_uio = NULL;
   int ret;

   auto cloneuio = (struct uio*(*)(struct uio* uiop))kdlsym(cloneuio);
   auto console_write = (int(*)(struct cdev* dev, struct uio* uio, int ioflag))kdlsym(console_write);
   auto deci_tty_write = (int(*)(struct cdev* dev, struct uio* uio, int ioflag))kdlsym(deci_tty_write);
   auto M_IOV = (struct malloc_type*)kdlsym(M_IOV);
   auto console_cdev = (struct cdev**)kdlsym(console_cdev);
   auto free = (void(*)(void* addr, struct malloc_type* type))kdlsym(free);

   cloned_uio = cloneuio(uio);

   ret = deci_tty_write(dev, uio, ioflag);

   if (cloned_uio) {
       if (*console_cdev)
           console_write(*console_cdev, cloned_uio, ioflag);
       free(cloned_uio, M_IOV);
   }

   return ret;
}