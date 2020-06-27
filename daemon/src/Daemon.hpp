#pragma once
#include <Utils/IModule.hpp>

#include <mutex>

// Main entry point
int main(void);

namespace Mira
{
    class Daemon :
        public Utils::IModule
    {
    private:

    public:
        virtual bool OnLoad() override;
    };
};