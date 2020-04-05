#include "SyscallGuardPlugin.hpp"
#include <Utils/Kdlsym.hpp>

extern "C"
{
    #include <sys/sysent.h>
};

using namespace Mira::Plugins;

SyscallGuard::SyscallGuard()
{
    // Get the syscall count
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	//struct sysent* sysents = sv->sv_table;
    
    m_SyscallCount = sv->sv_size;
    
    
}

SyscallGuard::~SyscallGuard()
{

}

bool SyscallGuard::OnLoad()
{
    return true;
}

bool SyscallGuard::OnUnload()
{
    return true;
}

bool SyscallGuard::OnSuspend()
{
    return true;
}

bool SyscallGuard::OnResume()
{
    return true;
}
