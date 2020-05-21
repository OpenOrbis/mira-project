#include <Boot/Patches.hpp>

// Patches done by CrazyVoid
// Thanks to
// WildCard for helping with patches
// Joonie for helping with memcpy patch
// LightingMod for being a tester
// Updated by SiSTRo

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_455()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_455
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x01997BC8];
	kmem[0] = 0x00;

	// Verbose Panics
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x003DBDC7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// sceSblACMgrIsAllowedSystemLevelDebugging
	kmem = (uint8_t *)&gKernelBase[0x00169E00];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0016A530];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0016A550];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	// Enable rwx mapping
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x0016ED8C];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x0016EDA2];
	kmem[0] = 0x07;

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	// Done by CrazyVoid
	kmem = (uint8_t *)&gKernelBase[0x0014A8E7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0014A802];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Enable MAP_SELF
	// Done by IDC
	kmem = (uint8_t *)&gKernelBase[0x0016A5B0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x0016A5C0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00143BE7];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch copyinstr
	// Done by CrazyVoid
	kmem = (uint8_t *)&gKernelBase[0x0014AD53];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0014AD83];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch memcpy stack
	// Done by CrazyVoid
	kmem = (uint8_t *)&gKernelBase[0x0014A6BD];
	kmem[0] = 0xEB;

	// ptrace patches
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x0017D2C1];
	kmem[0] = 0xEB;

	// second ptrace patch
	kmem = (uint8_t *)&gKernelBase[0x0017D636];
	kmem[0] = 0xE9;
	kmem[1] = 0x15;
	kmem[2] = 0x01;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x00116B9C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch to remove vm_fault: fault on nofault entry, addr %llx
	kmem = (uint8_t *)&gKernelBase[0x0029F45E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch mprotect to allow RWX (mprotect) mapping 4.55
	kmem = (uint8_t *)&gKernelBase[0x00396A58];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// flatz disable pfs signature check
	kmem = (uint8_t *)&gKernelBase[0x0069F4E0];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// flatz enable debug RIFs
	kmem = (uint8_t *)&gKernelBase[0x0062D720];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0062D740];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo (ported by kiwidog)
	kmem = (uint8_t *)&gKernelBase[0x0001801A];
	kmem[0] = 0xEB;
	kmem[1] = 0x39;

	// Enable mount for unprivileged user
	kmem = (uint8_t *)&gKernelBase[0x000DA483];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch suword_lwpid
	// has a check to see if child_tid/parent_tid is in kernel memory, and it in so patch it
	// Patch by: JOGolden
	kmem = (uint8_t *)&gKernelBase[0x0014AB92];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0014ABA1];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch debug setting errors
	kmem = (uint8_t *)&gKernelBase[0x004D70F7];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	kmem = (uint8_t *)&gKernelBase[0x004D7F81];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	// prtinf hook patches
	kmem = (uint8_t *)&gKernelBase[0x00018026];
	kmem[0] = 0xEB;
	kmem[1] = 0x2D;

	kmem = (uint8_t *)&gKernelBase[0x00018049];
	kmem[0] = 0xEB;

#endif
}
