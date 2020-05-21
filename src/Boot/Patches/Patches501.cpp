#include <Boot/Patches.hpp>

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_501()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_501
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	kmem = (uint8_t *)&gKernelBase[0x019ECEB0];
	kmem[0] = 0x00;

	// Verbose Panics
	kmem = (uint8_t *)&gKernelBase[0x00171517];
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
	kmem = (uint8_t *)&gKernelBase[0x000FCC38];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x000FCC46];
	kmem[0] = 0x07;

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	kmem = (uint8_t *)&gKernelBase[0x001EA657];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x001EA572];
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

	kmem = (uint8_t *)&gKernelBase[0x0013EF2F];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x001EAA83];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x001EAAB3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch memcpy stack
	kmem = (uint8_t *)&gKernelBase[0x001EA42D];
	kmem[0] = 0xEB;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x0030D61A];
	kmem[0] = 0xEB;

	// second ptrace patch
	kmem = (uint8_t *)&gKernelBase[0x0030DA71];
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
	kmem = (uint8_t *)&gKernelBase[0x002A4BE3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch mprotect to allow RWX (mprotect) mapping 5.01
	kmem = (uint8_t *)&gKernelBase[0x001A3AF8];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// flatz disable pfs signature check
	kmem = (uint8_t *)&gKernelBase[0x006A2320];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// flatz enable debug RIFs
	kmem = (uint8_t *)&gKernelBase[0x0064AED0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0064AEF0];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo (ported by kiwidog)
	kmem = (uint8_t *)&gKernelBase[0x00435D5A];
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
	kmem = (uint8_t *)&gKernelBase[0x001EA8C2];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x001EA8D1];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch debug setting errors
	kmem = (uint8_t *)&gKernelBase[0x004F8C78];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	kmem = (uint8_t *)&gKernelBase[0x004F9D8C];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	// prtinf hook patches
	kmem = (uint8_t *)&gKernelBase[0x00435D66];
	kmem[0] = 0xEB;
	kmem[1] = 0x1E;

	kmem = (uint8_t *)&gKernelBase[0x00435D84];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

#endif
}
