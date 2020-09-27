// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

// Patches done by SiSTRo & Joonie

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_405()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_405
	// NOTE: Only apply patches that the loader requires to run, the rest of them should go into Mira's ELF
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	kmem = (uint8_t *)&gKernelBase[0x0186B0A0];
	kmem[0] = 0x00;

	// Patch sys_dynlib_dlsym: Allow from anywhere
	//kmem = (uint8_t *)&gKernelBase[0x003CF6FE];
	kmem[0] = 0xE9;
	kmem[1] = 0x52;
	kmem[2] = 0x03;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	//kmem = (uint8_t *)&gKernelBase[0x000690C0];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;

	// Patch sys_mmap: Allow RWX (read-write-execute) mapping
	//kmem = (uint8_t *)&gKernelBase[0x00141D14];
	kmem[0] = 0x37;
	kmem[3] = 0x37;

	// Patch setuid: Don't run kernel exploit more than once/privilege escalation
	//kmem = (uint8_t *)&gKernelBase[0x001144E3];
	kmem[0] = 0xB8;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// Enable RWX (kmem_alloc) mapping
	kmem = (uint8_t *)&gKernelBase[0x0036958D];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x003695A5];
	kmem[0] = 0x07;

	// Patch copyin/copyout: Allow userland + kernel addresses in both params
	kmem = (uint8_t *)&gKernelBase[0x00286E21];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x00286DA1];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x0028718D];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x002871BD];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch mprotect: Allow RWX (mprotect) mapping
	kmem = (uint8_t *)&gKernelBase[0x004423E9];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;
#endif
}
