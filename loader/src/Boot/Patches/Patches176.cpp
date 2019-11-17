#include <Boot/Patches.hpp>

using namespace Mira::Boot;

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Patches::install_prerunPatches_176() {

	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

    // Enable UART
	kmem = (uint8_t *)&gKernelBase[0x011242F6]; // 5.00 0x019ECEB0
	kmem[0] = 0x00;

    // Verbose Panics
	kmem = (uint8_t *)&gKernelBase[0x00415AD3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x65;
	kmem[6] = 0x8B;
	kmem[7] = 0x34;

	//Allow system level debugging
	kmem = (uint8_t *)&gKernelBase[0x003F5180];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;

	//Allow coredump
	kmem = (uint8_t *)&gKernelBase[0x003F51A0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	kmem[6] = 0x90;
	kmem[7] = 0x90;
	
	// Enable rwx mapping
	kmem = (uint8_t *)&gKernelBase[0x003AB305];
	kmem[0] = 0x07;

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	kmem = (uint8_t *)&gKernelBase[0x00413CEE];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00413C6E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Enable MAP_SELF
	kmem = (uint8_t *)&gKernelBase[0x003F5200];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	
	kmem = (uint8_t *)&gKernelBase[0x003F5210];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;
	
	kmem = (uint8_t *)&gKernelBase[0x003B6873];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x0041403A];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0041406A];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x002300A7];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
}