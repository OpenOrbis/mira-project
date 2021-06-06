#include <cstdio>
#include <cstdint>
#include <thread>
#include <mutex>
#include <orbis/libkernel.h>

void PrintThread()
{
    for (;;)
    {
        printf("[=] PrintThread\n");
        sceKernelSleep(1000);
    }
}

extern "C" void trainer_load()
{
    printf("[+] trainer_load has been reached\n");

    std::thread main_thread(PrintThread);

    main_thread.detach();

    printf("[+] detached thread\n");
}