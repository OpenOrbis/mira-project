#pragma once
#include <Utils/Types.hpp>

struct proc;
struct _ElfLoader_t;

namespace MiraLoader
{
    class Loader;
}
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

            uint64_t allocatedBase;

            // Kernel process handle
            struct proc* process;

            // Entrypoint
            // Userland should set this to NULL
            void(*entrypoint)(void*);

            // ElfLoader
            MiraLoader::Loader* elfLoader;

            // If this is an elf launch or not
            uint8_t isElf : 1;
            
            // If the kproc is currently running
            uint8_t isRunning : 1;
        } InitParams;
    }
}