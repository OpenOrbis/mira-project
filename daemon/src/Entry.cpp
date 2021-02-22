#include "Daemon.hpp"

#include <memory>
#include <cstdio>

#if defined(PS4)
#include <orbis/libkernel.h>
#endif

// Entry point
int main()
{
    // Create a new instance of our daemon
    auto s_Daemon = Mira::Daemon::GetInstance();
    if (s_Daemon == nullptr)
    {
        fprintf(stderr, "err: could not create daemon.\n");
        return -1;
    }

    // Try and load the daemon
    if (!s_Daemon->OnLoad())
    {
        printf("err: could not load daemon.\n");
        return -2;
    }
    
    for (;;)
        __asm__("nop");
    
    return 0;
}