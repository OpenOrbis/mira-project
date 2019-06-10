#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Hook.hpp>

#include <netinet/in.h>

struct trapframe;

namespace Mira
{
    namespace Plugins
    {
        class Debugger : public Mira::Utils::IModule
        {
        private:
            int32_t m_Socket;
            struct sockaddr_in m_Address;
            Utils::Hook* m_TrapFatalHook;

        protected:
            static void OnTrapFatal(struct trapframe* p_Frame, vm_offset_t p_Eva);
            static bool IsStackSpace(void* p_Address);
            
        public:
            Debugger();
            virtual ~Debugger();

            virtual const char* GetName() override { return "Debugger"; }
            virtual const char* GetDescription() override { return "GDB compatible debugger with ReClass abilties."; }
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;

            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            uint8_t StubGetChar(void);
            bool StubPutChar(uint8_t p_Char);
            int32_t StubReadByte(void* p_Address, char* p_OutValue);
            int32_t StubWriteByte(void* p_Address, char p_Value);
            int32_t StubContinue();
            int32_t StubStep();

        private:
            bool StartStubServer();
            bool TeardownStubServer();
        };
    }
}