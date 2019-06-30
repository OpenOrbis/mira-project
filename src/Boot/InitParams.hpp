#pragma once
#include <Utils/Types.hpp>

struct proc;
struct _ElfLoader_t;

namespace Mira
{
    namespace Boot
    {
        typedef struct InitParms_t
        {
            // Payload base address, can be allocated dynamically
            uint64_t payloadBase;

            // Payload size
            uint64_t payloadSize;

            // Kernel process handle
            struct proc* process;

            // Entrypoint
            // Userland should set this to NULL
            void(*entrypoint)(void*);

            // ElfLoader
            struct _ElfLoader_t* elfLoader;

            // If this is an elf launch or not
            uint8_t isElf : 1;
        } InitParams;
    }
}