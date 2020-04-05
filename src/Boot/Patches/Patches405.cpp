#include <Boot/Patches.hpp>

using namespace Mira::Boot;

// Patches done by SiSTRo & Joonie

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Patches::install_prerunPatches_405() 
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_405
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	kmem = (uint8_t *)&gKernelBase[0x0186B0A0];
	kmem[0] = 0x00;

	// Verbose Panics
	kmem = (uint8_t *)&gKernelBase[0x000EC81A];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Enable rwx mapping
	kmem = (uint8_t*)&gKernelBase[0x0036958D];
	kmem[0] = 0x07;

	kmem = (uint8_t*)&gKernelBase[0x003695A5];
	kmem[0] = 0x07;

	// Patch copy(in/out)
	kmem = (uint8_t *)&gKernelBase[0x00286E21];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00286DA1];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Enable MAP_SELF
	kmem = (uint8_t *)&gKernelBase[0x0031EE40];
	kmem[0] = 0x90;
	kmem[1] = 0xE9;

	kmem = (uint8_t *)&gKernelBase[0x0031EF98];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x0028718D];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x002871BD];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x000AC31E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x0008822C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
#endif
}