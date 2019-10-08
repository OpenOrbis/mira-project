#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Plugins
    {
        namespace Debugging
        {
            enum { Debugger_MaxPrint = 1024 };

            // This is created by CRC32(IoctlName)
            typedef enum _DebuggerIoctl
            {
                DebuggerEvent = 0x9B6C9F7B
                
            } DebuggerIoctl;

            typedef enum _DebuggerEvent
            {
                Malloc = 0xBBA498A8,
                Free = 0xED1C6EF6,
                Syscall = 0xF0C41A60,
                Print = 0xD4175362
            } DebuggerEvent;

            typedef struct _EventMalloc
            {
                uint64_t Address;
                uint64_t Size;
            } EventMalloc;

            typedef struct _EventFree
            {
                uint64_t Address;
            } EventFree;

            typedef struct _EventSyscall
            {
                int32_t Number;
                uint64_t Uap;
            } EventSyscall;

            typedef struct _EventPrint
            {
                uint32_t Length;
                char Message[Debugger_MaxPrint];
            } EventPrint;
        }
    }
}