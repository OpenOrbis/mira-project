#pragma once
#include <cstdint>
#include <netinet/in.h>

namespace Mira
{
    namespace Debugging
    {
        class Instance
        {
        private:
            /* Number of registers.  */
            enum 
            { 
                NUMREGS = 14,
                NUMREGBYTES = (NUMREGS * 8),
            };
 
            enum regnames 
            {
                RAX,
                RCX,
                RDX,
                RBX,
                RSP,
                RBP,
                RSI,
                RDI,
                RIP,
                EFLAGS,
                CS,
                SS,
                DS,
                ES,
                R8,
                R9,
                R10,
                R11,
                R12,
                R13,
                R14,
                R15,
                FS,
                GS,
                REGS_COUNT
            };
            
            int32_t m_Socket;
            struct sockaddr_in m_Connection;

        protected:
            void putDebugChar();
            int getDebugChar();
            void exceptionHandler();
        };
    }
}