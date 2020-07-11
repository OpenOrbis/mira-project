// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

void Mira::Boot::Patches::install_prerunPatches_672()
{
#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_672
	// You must assign the kernel base pointer before anything is done
	if (!gKernelBase)
		return;

	// Use "kmem" for all patches
	uint8_t *kmem;

	// Patch dynlib_dlsym
	kmem = (uint8_t*)&gKernelBase[0x1D895A];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

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

	// Enable MAP_SELF

	// Patches: sceSblACMgrHasMmapSelfCapability
	kmem = (uint8_t *)&gKernelBase[0x00233C40];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	// Patches: sceSblACMgrIsAllowedToMmapSelf
	kmem = (uint8_t *)&gKernelBase[0x00233C50];
	kmem[0] = 0xB8;
	kmem[1] = 0x01;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;
	kmem[5] = 0xC3;

	// Patches: call    sceSblAuthMgrIsLoadable in vm_mmap2 (right above the only call to allowed to mmap self)
	kmem = (uint8_t *)&gKernelBase[0x000AD2E4]; // xor eax, eax; nop; nop;
	kmem[0] = 0x31;
	kmem[1] = 0xC0;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;

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

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x0010F879];
	kmem[0] = 0xEB;

	// Enable debug rif's
	kmem = (uint8_t*)&gKernelBase[0x66AEB0];
	kmem[0] = 0xB0; 
	kmem[1] = 0x01; 
	kmem[2] = 0xC3; 
	kmem[3] = 0x90;

	// Enable debug rifs 2
	kmem = (uint8_t*)&gKernelBase[0x66AEE0];
	kmem[0] = 0xB0; 
	kmem[1] = 0x01; 
	kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// Disable pfs checks
	kmem = (uint8_t*)&gKernelBase[0x6A8EB0];
	kmem[0] = 0x31; 
	kmem[1] = 0xC0; 
	kmem[2] = 0xC3; 
	kmem[3] = 0x90;

	kmem = (uint8_t*)&gKernelBase[0x003C1857];
	kmem[0] = 0x41;
	kmem[1] = 0x41;

	// disable delayed panics
	kmem = (uint8_t*)&gKernelBase[0x0063C8CE];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t*)kdlsym(global_settings_base);
	kmem[0x36] |= 0x14;
	kmem[0x59] |= 0x01; // sceSblRcMgrIsAllowULDebugger
	kmem[0x59] |= 0x02; // sceSblRcMgrIsAllowSLDebugger
	kmem[0x5A] |= 0x01;
	kmem[0x78] |= 0x01;

	// Disable this verif bullshit

	// Disable sceSblSysVeriResume
	kmem = (uint8_t*)&gKernelBase[0x0063CDB0];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Disable sceSblSysVeriSuspend
	kmem = (uint8_t*)&gKernelBase[0x0063CC90];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Disable sysvericheckstatus_maybe
	/*kmem = (uint8_t*)&gKernelBase[0x0063BD90];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Disable sysvericheckstatus_maybe callout_ call
	kmem = (uint8_t*)&gKernelBase[0x0063BDE4];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;*/

	// Disable delayedPanicCb
	kmem = (uint8_t*)&gKernelBase[0x0063C8C0];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// Disable panic_fatal
	/*kmem = (uint8_t*)&gKernelBase[0x0043BB0];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;*/

	// Disable sub_FFFFFFFF8283C8D1
	/*kmem = (uint8_t*)&gKernelBase[0x0063C8D1];
	kmem[0] = 0xC3;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

	// NOP sled sceSblSysVeriThread
	kmem = (uint8_t*)&gKernelBase[0x0063C950];
	for (int i = 0; i < 0x327; ++i)
		kmem[i] = 0x90; // NOP sled the entire shit, until the kthread_exit call*/

	// Enable *all* debugging logs (in vprintf)
	// Patch by: SiSTRo
	kmem = (uint8_t*)&gKernelBase[0x00123367];
	kmem[0] = 0xEB; // jmp +0x3D
	kmem[1] = 0x3B;

#endif
}