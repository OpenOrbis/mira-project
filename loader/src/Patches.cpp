#include <Boot/Patches.hpp>

using namespace Mira::Boot;

void Patches::install_prePatches()
{
	switch (ONI_PLATFORM)
	{
		case ONI_PLATFORM_ORBIS_BSD_405:
			install_prerunPatches_405();
			break;
		case ONI_PLATFORM_ORBIS_BSD_455:
			install_prerunPatches_455();
			break;
		case ONI_PLATFORM_ORBIS_BSD_474:
			install_prerunPatches_474();
			break;
		case ONI_PLATFORM_ORBIS_BSD_501:
			install_prerunPatches_501();
			break;
		case ONI_PLATFORM_ORBIS_BSD_505:
			install_prerunPatches_505();
			break;
#if ONI_PLATFORM == ONI_PLATFORM_RASPI_ZERO
		case ONI_PLATFORM_RASPI_ZERO:
			break;
#elif ONI_PLATFORM == ONI_PLATFORM_STEAM_LINK
		case ONI_PLATFORM_STEAM_LINK:
			install_prerunPatches_SteamLink();
			break;
#elif ONI_PLATFORM == ONI_PLATFORM_STEAM_LINK2
		case ONI_PLATFORM_STEAM_LINK:
			install_prerunPatches_SteamLink2();
			break;
#endif
		default:
			break;
	}
}
