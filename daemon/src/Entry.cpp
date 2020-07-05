#include "Daemon.hpp"

#include <memory>
#include <cstdio>

// Entry point
int main(void)
{
    // Create a new instance of our daemon
    auto s_Daemon = std::make_unique<Mira::Daemon>();
    if (s_Daemon == nullptr)
    {
        printf("err: could not initialize daemon.\n");
        return -1;
    }

    // Try and load the daemon
    if (!s_Daemon->OnLoad())
    {
        printf("err: could not load daemon.\n");
        return -2;
    }

    // We never want to return from main
    for (;;)
        __asm__("nop");
    
    return 0;
}