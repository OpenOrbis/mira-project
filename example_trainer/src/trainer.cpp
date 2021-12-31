#include <cstdio>
#include <cstdint>
#include <thread>
#include <mutex>
#include <orbis/libkernel.h>

#include "_syscalls.hpp"

int64_t debug_log(const char* debug_message) {
    
    return (int64_t)syscall3(601, (void*)0x7, reinterpret_cast<void*>(const_cast<char*>(debug_message)), (void*)0x0);
}

void PrintThread()
{
    for (;;)
    {
        debug_log("[=] PrintThread\n");
        sceKernelSleep(1000);
    }
}

extern "C" void trainer_load()
{
    debug_log("[+] trainer_load has been reached\n");

    std::thread main_thread(PrintThread);
    main_thread.detach();

    debug_log("[+] detached thread\n");
}

extern "C" void _start()
{
	// Empty start
	return;
}