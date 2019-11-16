#include <Boot/Patches.hpp>

using namespace Mira::Boot;

void Patches::install_prePatches()
{
	switch (ONI_PLATFORM)
	{
		case ONI_PLATFORM_ORBIS_BSD_176:
			install_prerunPatches_176();
			break;
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
		case ONI_PLATFORM_RASPI_ZERO:
			// no patches needed for the raspi-z
			break;
		case ONI_PLATFORM_ORBIS_BSD_620:
			install_prerunPatches_620();
			break;
		case ONI_PLATFORM_ORBIS_BSD_650:
			install_prerunPatches_650();
			break;
		case ONI_PLATFORM_STEAM_LINK:
		case ONI_PLATFORM_STEAM_LINK2:
			install_prerunPatches_SteamLink2();
			break;
		default:
			break;
	}
}
