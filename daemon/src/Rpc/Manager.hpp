#pragma once
#include <vector>
#include <cstddef>
#include <limits>
#include <memory>

namespace Mira
{
    namespace Rpc
    {
        struct RpcHeader;
        class Connection;
        class Listener;

        class Manager
        {
        private:

        public:
            Manager();
            virtual ~Manager();
        };
    }
}