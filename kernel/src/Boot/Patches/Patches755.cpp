// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

/*
	Please, please, please!
	Keep patches consistent with the used patch style for readability.
*/
void Mira::Boot::Patches::install_prerunPatches_755()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_755
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Enable UART
	kmem = (uint8_t *)&gKernelBase[0x01564910];
	kmem[0] = 0x00;

	// Verbose Panics
	kmem = (uint8_t *)&gKernelBase[0x0046D11E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// sceSblACMgrIsAllowedSystemLevelDebugging
	kmem = (uint8_t *)&gKernelBase[0x003644B0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00364CD0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00364CF0];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	// Enable rwx mapping
	kmem = (uint8_t *)&gKernelBase[0x001754AC];
	kmem[0] = 0x07;

	kmem = (uint8_t *)&gKernelBase[0x001754B4];
	kmem[0] = 0x07;

	// Patch copyin/copyout: Allow userland + kernel addresses in both params
	// copyin
	kmem = (uint8_t *)&gKernelBase[0x0028FA47];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0028FA53];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// copyout
	kmem = (uint8_t *)&gKernelBase[0x0028F952];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0028F95E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Enable MAP_SELF
	kmem = (uint8_t *)&gKernelBase[0x00364D40];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00364D60];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x000DCED1];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x0028FEF3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0028FEFF];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Patch memcpy stack
	kmem = (uint8_t *)&gKernelBase[0x0028F80D];
	kmem[0] = 0xEB;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x00361CF5];
	kmem[0] = 0xEB;

	// second ptrace patch
	kmem = (uint8_t *)&gKernelBase[0x003621CF];
	kmem[0] = 0xE9;
	kmem[1] = 0x7C;
	kmem[2] = 0x02;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// setlogin patch (for autolaunch check)
	kmem = (uint8_t *)&gKernelBase[0x0037CF6C];
	kmem[0] = 0x48;
	kmem[1] = 0x31;
	kmem[2] = 0xC0;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

	// Patch to remove vm_fault: fault on nofault entry, addr %llx
	kmem = (uint8_t *)&gKernelBase[0x003DF2A6];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// Patch mprotect: Allow RWX (mprotect) mapping
	/*kmem = (uint8_t *)&gKernelBase[0x003014C8];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;*/
	kmem = (uint8_t*)&gKernelBase[0x3014C8];
	kmem[0] = 0xEB;
	kmem[1] = 0x04;

	// flatz disable pfs signature check
	kmem = (uint8_t *)&gKernelBase[0x006DD9A0];
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;

	// flatz enable debug RIFs
	kmem = (uint8_t *)&gKernelBase[0x00668140];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;

	kmem = (uint8_t *)&gKernelBase[0x00668170];
	kmem[0] = 0xB0;
	kmem[1] = 0x01;
	kmem[2] = 0xC3;

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo
	kmem = (uint8_t *)&gKernelBase[0x0026F827];
	kmem[0] = 0xEB;
	kmem[1] = 0x3B;

	// flatz allow mangled symbol in dynlib_do_dlsym
	kmem = (uint8_t *)&gKernelBase[0x000271A7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// Enable mount for unprivileged user
	kmem = (uint8_t *)&gKernelBase[0x00076385];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// patch suword_lwpid
	// has a check to see if child_tid/parent_tid is in kernel memory, and it in so patch it
	// Patch by: JOGolden
	kmem = (uint8_t *)&gKernelBase[0x0028FD12];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0028FD21];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	// Patch debug setting errors
	kmem = (uint8_t *)&gKernelBase[0x004FF322];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	kmem = (uint8_t *)&gKernelBase[0x0050059C];
	kmem[0] = 0x00;
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;

	// Send sysveri to hell
	kmem = (uint8_t*)&gKernelBase[0x00636850];
	kmem[0] = 0xC3;

	kmem = (uint8_t*)&gKernelBase[0x00637380];
	// mov rax, 0
	// ret
	kmem[0] = 0x48;
	kmem[1] = 0xC7;
	kmem[2] = 0xC0;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0x00;
	kmem[6] = 0x00;
	kmem[7] = 0xC3;

	// Fuck this function too
	kmem = (uint8_t*)&gKernelBase[0x00636600];
	kmem[0] = 0x48;
	kmem[1] = 0xC7;
	kmem[2] = 0xC0;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0x00;
	kmem[6] = 0x00;
	kmem[7] = 0xC3;

	// sceSblSysVeriInitialize
	kmem = (uint8_t*)&gKernelBase[0x00636DB0];
	kmem[0] = 0x48;
	kmem[1] = 0xC7;
	kmem[2] = 0xC0;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0x00;
	kmem[6] = 0x00;
	kmem[7] = 0xC3;

	// Clear the sceVeri initialized flag
	kmem = (uint8_t*)&gKernelBase[0x2662B00];
	kmem[0] = 0;

#endif
}
