// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

void Mira::Boot::Patches::install_prerunPatches_672()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_672
	// NOTE: Only apply patches that the loader requires to run, the rest of them should go into Mira's ELF
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Patch dynlib_dlsym
	kmem = (uint8_t*)&gKernelBase[0x1D895A];
	kmem[0] = 0xE9;
	kmem[1] = 0xC7;
	kmem[2] = 0x01;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// Patch a function called by dynlib_dlsym
	kmem = (uint8_t*)&gKernelBase[0x0041A2D0];
	kmem[0] = 0x31; // xor eax, eax
	kmem[1] = 0xC0;
	kmem[2] = 0xC3;	// ret

	// Patch sys_mmap
	kmem = (uint8_t*)&gKernelBase[0x000AB57A];
	kmem[0] = 0x37; // mov     [rbp+var_61], 33h ; '3'
	kmem[3] = 0x37; // mov     sil, 33h ; '3'

	// patch sys_setuid
	kmem = (uint8_t*)&gKernelBase[0x0010BED0]; // call    priv_check_cred; overwrite with mov eax, 0
	kmem[0] = 0xB8; // mov eax, 0
	kmem[1] = 0x00;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

	// patch sys_mprotect
	kmem = (uint8_t*)&gKernelBase[0x00451DB8]; // jnz     loc_FFFFFFFF82652426; nop it out
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// Enable rwx mapping in kmem_alloc
	kmem = (uint8_t *)&gKernelBase[0x002507F5];
	kmem[0] = 0x07; // set maxprot to RWX

	kmem = (uint8_t *)&gKernelBase[0x00250803];
	kmem[0] = 0x07; // set maxprot to RWX

	// Patch copyin/copyout to allow userland + kernel addresses in both params
	// copyin
	kmem = (uint8_t *)&gKernelBase[0x003C17F7];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x003C1803];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// copyout
	kmem = (uint8_t *)&gKernelBase[0x003C1702];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x003C170E];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x003C1CA3];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x003C1CAF];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Patch memcpy stack
	kmem = (uint8_t *)&gKernelBase[0x003C15BD];
	kmem[0] = 0xEB;
#endif
}
