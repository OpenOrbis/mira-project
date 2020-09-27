// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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
	// NOTE: Only apply patches that the loader requires to run, the rest of them should go into Mira's ELF
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	// Done by WildCard
	kmem = (uint8_t *)&gKernelBase[0x01997BC8];
	kmem[0] = 0x00;

	// Patch sys_dynlib_dlsym: Allow from anywhere
	kmem = (uint8_t *)&gKernelBase[0x003CF6FE];
	kmem[0] = 0xE9;
	kmem[1] = 0x52;
	kmem[2] = 0x03;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	kmem = (uint8_t *)&gKernelBase[0x000690C0];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;

	// Patch sys_mmap: Allow RWX (read-write-execute) mapping
	kmem = (uint8_t *)&gKernelBase[0x00141D14];
	kmem[0] = 0x37;
	kmem[3] = 0x37;

	// Patch setuid: Don't run kernel exploit more than once/privilege escalation
	kmem = (uint8_t *)&gKernelBase[0x001144E3];
	kmem[0] = 0xB8;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// Enable RWX (kmem_alloc) mapping
	// Done By WildCard
	kmem = (uint8_t *)&gKernelBase[0x0016ED8C];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x0016EDA2];
	kmem[0] = 0x07;

	// Patch copyin/copyout: Allow userland + kernel addresses in both params
	// Done by CrazyVoid
	kmem = (uint8_t *)&gKernelBase[0x0014A8E7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0014A802];
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

	// Patch mprotect: Allow RWX (mprotect) mapping
	kmem = (uint8_t *)&gKernelBase[0x00396A58];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;
#endif
}
