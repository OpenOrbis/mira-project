#include <Boot/Patches.hpp>

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_505()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_505
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	kmem = (uint8_t *)&gKernelBase[0x019ECEB0];
	kmem[0] = 0x00;

	// Verbose Panics
	kmem = (uint8_t *)&gKernelBase[0x00171627];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x65;
	kmem[6] = 0x8B;
	kmem[7] = 0x34;

	// sceSblACMgrIsAllowedSystemLevelDebugging
	kmem = (uint8_t *)&gKernelBase[0x00010FC0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00011730];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00011750];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;
	
	// Enable rwx mapping
	kmem = (uint8_t *)&gKernelBase[0x000FCD48];
	kmem[0] = 0x07;
	
	kmem = (uint8_t *)&gKernelBase[0x000FCD56];
	kmem[0] = 0x07;

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	kmem = (uint8_t *)&gKernelBase[0x001EA767];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	
	kmem = (uint8_t *)&gKernelBase[0x001EA682];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Enable MAP_SELF
	kmem = (uint8_t*)&gKernelBase[0x000117b0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	
	kmem = (uint8_t *)&gKernelBase[0x000117c0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	
	kmem = (uint8_t *)&gKernelBase[0x0013F03F];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x001EAB93];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x001EABC3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch memcpy stack
	kmem = (uint8_t *)&gKernelBase[0x001EA53D];
	kmem[0] = 0xEB;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x0030D9C3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x0005775C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// flatz disable pfs signature check
	kmem = (uint8_t *)&gKernelBase[0x006A2700];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// flatz enable debug RIFs
	kmem = (uint8_t *)&gKernelBase[0x0064B2B0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0064B2D0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo
	kmem = (uint8_t*)&gKernelBase[0x043612A];
	kmem[0] = 0xEB; // jmp +0x3A
	kmem[1] = 0x38;

	// Enable mount for unprivileged user
	kmem = (uint8_t *)&gKernelBase[0x001DEBFE];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;
#endif
}
