#include <Boot/Patches.hpp>

using namespace Mira::Boot;

void Patches::install_prePatches()
{
	switch (MIRA_PLATFORM)
	{
		case MIRA_PLATFORM_ORBIS_BSD_176:
			install_prerunPatches_176();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_405:
			install_prerunPatches_405();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_455:
			install_prerunPatches_455();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_474:
			install_prerunPatches_474();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_501:
			install_prerunPatches_501();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_503:
			install_prerunPatches_503();
		case MIRA_PLATFORM_ORBIS_BSD_505:
			install_prerunPatches_505();
			break;
		case MIRA_PLATFORM_RASPI_ZERO:
			// no patches needed for the raspi-z
			break;
		case MIRA_PLATFORM_ORBIS_BSD_620:
			install_prerunPatches_620();
			break;
		case MIRA_PLATFORM_ORBIS_BSD_650:
			install_prerunPatches_650();
			break;
		case MIRA_PLATFORM_STEAM_LINK:
		case MIRA_PLATFORM_STEAM_LINK2:
			install_prerunPatches_SteamLink2();
			break;
		default:
			break;
	}
}
