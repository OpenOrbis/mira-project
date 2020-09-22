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
            static void install_prerunPatches_176();
            static void install_prerunPatches_405();
            static void install_prerunPatches_455();
            static void install_prerunPatches_474();
            static void install_prerunPatches_501();
            static void install_prerunPatches_503();
            static void install_prerunPatches_505();
            static void install_prerunPatches_555();
            static void install_prerunPatches_620();
            static void install_prerunPatches_650();
            static void install_prerunPatches_672();
            // static void install_prerunPatches_SteamLink(); // got both versions booting off the same code
            static void install_prerunPatches_SteamLink2();
        };
    }
}
