// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_503()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_503
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

	// sceSblACMgrIsAllowedSystemLevelDebugging
	kmem = (uint8_t *)&gKernelBase[0x00010FC0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00011730];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00011750];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

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
	kmem = (uint8_t *)&gKernelBase[0x000117B0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x000117C0];
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
	kmem = (uint8_t *)&gKernelBase[0x0030D9AA];
	kmem[0] = 0xEB;

	// second ptrace patch
	kmem = (uint8_t *)&gKernelBase[0x0030DE01];
	kmem[0] = 0xE9;
	kmem[1] = 0xD0;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x0005775C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch to remove vm_fault: fault on nofault entry, addr %llx
	kmem = (uint8_t *)&gKernelBase[0x002A4EB3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch mprotect to allow RWX (mprotect) mapping 5.03
	kmem = (uint8_t *)&gKernelBase[0x001A3C08];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// flatz disable pfs signature check
	kmem = (uint8_t *)&gKernelBase[0x006A26C0];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;

	// flatz enable debug RIFs
	kmem = (uint8_t *)&gKernelBase[0x0064B270];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x0064B290];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo
	kmem = (uint8_t *)&gKernelBase[0x004360EA];
	kmem[0] = 0xEB;
	kmem[1] = 0x38;

	// flatz allow mangled symbol in dynlib_do_dlsym
	kmem = (uint8_t *)&gKernelBase[0x002AF877];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// Enable mount for unprivileged user
	kmem = (uint8_t *)&gKernelBase[0x001DEAEE];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch suword_lwpid
	// has a check to see if child_tid/parent_tid is in kernel memory, and it in so patch it
	// Patch by: JOGolden
	kmem = (uint8_t *)&gKernelBase[0x001EA9D2];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x001EA9E1];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch debug setting errors
	kmem = (uint8_t *)&gKernelBase[0x004F9008];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	kmem = (uint8_t *)&gKernelBase[0x004FA11C];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

#endif
}
