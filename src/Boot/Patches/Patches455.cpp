#include <Boot/Patches.hpp>

using namespace Mira::Boot;

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
void Patches::install_prerunPatches_455()
{
	// You must assign the kernel base pointer before anything is done
	if(!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x01997BC8];
	kmem[0] = 0x00;

	// Verbose Panics patch
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x003DBDC7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	//kmem[5] = 0x65;
	//kmem[6] = 0x8B;
	//kmem[7] = 0x34;

	// Enable rwx mapping
	// Done By WildCard
	kmem = (uint8_t*)&gKernelBase[0x0016ED8C];
	kmem[0] = 0x07;

	kmem = (uint8_t*)&gKernelBase[0x0016EDA2];
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
	kmem = (uint8_t *)&gKernelBase[0x00143BF2];
	kmem[0] = 0x90;
	kmem[1] = 0xE9;

	kmem = (uint8_t *)&gKernelBase[0x00143E0E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

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
	kmem = (uint8_t *)&gKernelBase[0x0017D2EE];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x00116B9C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

}