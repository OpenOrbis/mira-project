#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace OrbisOS
    {
        class Utilities
        {
        private:
            static Utilities* m_Instance;

            Utilities();

        public:
            static Utilities* GetInstance();


        };
    }
}