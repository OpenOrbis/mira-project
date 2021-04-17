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
            
            
        public:
            Debugger();
            virtual ~Debugger();

            virtual bool OnLoad() override;
        };
    }
}