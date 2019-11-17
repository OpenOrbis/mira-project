#include <Boot/Patches.hpp>

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
	thx: Fire30
*/
void Mira::Boot::Patches::install_prerunPatches_620()
{
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable rwx mapping
	kmem = (uint8_t *)&gKernelBase[0x002704E8];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x002704F6];
	kmem[0] = 0x07;

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	// copyin
	kmem = (uint8_t *)&gKernelBase[0x00114947];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00114953];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// copyout
	kmem = (uint8_t *)&gKernelBase[0x00114852];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0011485E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Enable MAP_SELF
	// TODO: Find MAP_SELF patches

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x00114DF3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00114DFF];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Patch memcpy stack
	kmem = (uint8_t *)&gKernelBase[0x0011470D];
	kmem[0] = 0xEB;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x0013F234];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	kmem = (uint8_t*)&gKernelBase[0x001149A7];
	kmem[0] = 0x41;
	kmem[1] = 0x41;
}
