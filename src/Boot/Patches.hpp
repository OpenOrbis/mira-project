#pragma once
#include <Utils/Types.hpp>
#include <Utils/Kdlsym.hpp>

namespace Mira
{
    namespace Boot
    {
        class Patches
        {
        public:
            static void install_prePatches();
            static void install_prerunPatches_405();
            static void install_prerunPatches_455();
            static void install_prerunPatches_474();
            static void install_prerunPatches_501();
            static void install_prerunPatches_505();
            static void install_prerunPatches_RaspiZero();
            static void install_prerunPatches_SteamLink();
            static void install_prerunPatches_SteamLink2();
        };
    }
}