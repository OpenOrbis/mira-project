#pragma once
#include <Utils/IModule.hpp>
#include <vector>

namespace Mira
{
    namespace Debugging
    {
        class Debugger :
            public Utils::IModule
        {
        private:
            uint64_t m_Registers[REGS_COUNT];
            
        public:
            Debugger();
            virtual ~Debugger();

            virtual bool OnLoad() override;
        };
    }
}