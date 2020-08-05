// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

#define debug_patch 1
/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
	thx: Fire30
*/
void Mira::Boot::Patches::install_prerunPatches_620()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_620
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
	
	//enable UART
	//*(char *)(kernel_base + 0x01570338) = 0;
	kmem = (uint8_t *)&gKernelBase[0x01570338];
	kmem[0] = 0x00;
	
	// Patches: sceSblACMgrHasMmapSelfCapability
	kmem = (uint8_t *)&gKernelBase[0x004594B0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	// Patches: sceSblACMgrIsAllowedToMmapSelf
	kmem = (uint8_t *)&gKernelBase[0x004594C0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;



        // Patches: flatz ddebug_menu_error_patch2 6.20
	kmem = (uint8_t *)&gKernelBase[0x50382c];
	kmem[0] = 0x00;
        kmem[1] = 0x00;
        kmem[2] = 0x00;
        kmem[3] = 0x00;

        // Patches: flatz ddebug_menu_error_patch1 6.20
	kmem = (uint8_t *)&gKernelBase[0x50256e];
	kmem[0] = 0x00;
        kmem[1] = 0x00;
        kmem[2] = 0x00;
        kmem[3] = 0x00;

         /* Huge thanks to Chendo for the corrected offsets */

        // Patches: flatz disable pfs signature check 6.20
	kmem = (uint8_t *)&gKernelBase[0x6a3c10];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;
        kmem[3] = 0x90;


        // Patches: flatz enable debug RIFs pt1 6.20
	kmem = (uint8_t *)&gKernelBase[0x667dc0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
        kmem[3] = 0x90;



        // Patches: flatz enable debug RIFs pt2 6.20
	kmem = (uint8_t *)&gKernelBase[0x667df0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
        kmem[3] = 0x90;
	

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
#endif
}
