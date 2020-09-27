// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Boot/Patches.hpp>

void Mira::Boot::Patches::install_prerunPatches_555()
{
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_555
  // You must assign the kernel base pointer before anything is done
  if (!gKernelBase)
    return;

  // Use "kmem" for all patches
  uint8_t *kmem;

  // Enable UART
  kmem = (uint8_t *)&gKernelBase[0x01A63DD0];
  kmem[0] = 0x00;

  // Verbose Panics
  kmem = (uint8_t *)&gKernelBase[0x000A1826];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;
  kmem[3] = 0x90;
  kmem[4] = 0x90;

  // sceSblACMgrIsAllowedSystemLevelDebugging
  kmem = (uint8_t *)&gKernelBase[0x001B4410];
  kmem[0] = 0xB8;
  kmem[1] = 0x01;
  kmem[2] = 0x00;
  kmem[3] = 0x00;
  kmem[4] = 0x00;
  kmem[5] = 0xC3;

  kmem = (uint8_t *)&gKernelBase[0x001B4B90];
  kmem[0] = 0xB8;
  kmem[1] = 0x01;
  kmem[2] = 0x00;
  kmem[3] = 0x00;
  kmem[4] = 0x00;
  kmem[5] = 0xC3;

  kmem = (uint8_t *)&gKernelBase[0x001B4BB0];
  kmem[0] = 0xB8;
  kmem[1] = 0x01;
  kmem[2] = 0x00;
  kmem[3] = 0x00;
  kmem[4] = 0x00;
  kmem[5] = 0xC3;

  // Enable rwx mapping in kmem_alloc
	kmem = (uint8_t *)&gKernelBase[0x001423A8];
	kmem[0] = 0x07; // set maxprot to RWX

	kmem = (uint8_t *)&gKernelBase[0x001423B6];
	kmem[0] = 0x07; // set maxprot to RWX

  // Patch copyin/copyout to allow userland + kernel addresses in both params
  // copyin
  kmem = (uint8_t *)&gKernelBase[0x00405DD2];
  kmem[0] = 0x90;
  kmem[1] = 0x90;

  kmem = (uint8_t *)&gKernelBase[0x00405DDE];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;

  // copyout
  kmem = (uint8_t *)&gKernelBase[0x00405EC7];
  kmem[0] = 0x90;
  kmem[1] = 0x90;

  kmem = (uint8_t *)&gKernelBase[0x00405ED3];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;

  // Enable MAP_SELF

	// Patches: sceSblACMgrHasMmapSelfCapability
  kmem = (uint8_t *)&gKernelBase[0x001B4C00];
  kmem[0] = 0xB8;
  kmem[1] = 0x01;
  kmem[2] = 0x00;
  kmem[3] = 0x00;
  kmem[4] = 0x00;
  kmem[5] = 0xC3;

	// Patches: sceSblACMgrIsAllowedToMmapSelf
  kmem = (uint8_t *)&gKernelBase[0x001B4C10];
  kmem[0] = 0xB8;
  kmem[1] = 0x01;
  kmem[2] = 0x00;
  kmem[3] = 0x00;
  kmem[4] = 0x00;
  kmem[5] = 0xC3;

	// Patches: call    sceSblAuthMgrIsLoadable in vm_mmap2 (right above the only call to allowed to mmap self)
  kmem = (uint8_t *)&gKernelBase[0x003C427D]; // xor eax, eax; nop; nop;
  kmem[0] = 0x31;
  kmem[1] = 0xC0;
  kmem[2] = 0x90;
  kmem[3] = 0x90;
  kmem[4] = 0x90;

  // Patch copyinstr
	kmem = (uint8_t *)&gKernelBase[0x00406373];
	kmem[0] = 0x90;
	kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x0040637F];
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;

  // Patch memcpy stack
  kmem = (uint8_t *)&gKernelBase[0x00405C8D];
  kmem[0] = 0xEB;

	// ptrace patches
	kmem = (uint8_t *)&gKernelBase[0x00393EE8];
	kmem[0] = 0xEB;

	// second ptrace patch
	kmem = (uint8_t *)&gKernelBase[0x00394364];
	kmem[0] = 0xE9;
	kmem[1] = 0x8F;
	kmem[2] = 0x00;
	kmem[3] = 0x00;
	kmem[4] = 0x00;

  // setlogin patch (for autolaunch check)
  kmem = (uint8_t *)&gKernelBase[0x0001352C];
  kmem[0] = 0x48;
  kmem[1] = 0x31;
  kmem[2] = 0xC0;
  kmem[3] = 0x90;
  kmem[4] = 0x90;

  // Patch to remove vm_fault: fault on nofault entry, addr %llx
  kmem = (uint8_t *)&gKernelBase[0x00214486];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;
  kmem[3] = 0x90;
  kmem[4] = 0x90;
  kmem[5] = 0x90;

  // patch mprotect to allow RWX (mprotect) mapping 5.55
	kmem = (uint8_t*)&gKernelBase[0x0002EE98]; // jnz     loc_FFFFFFFF8CB974C3; nop it out
	kmem[0] = 0x90;
	kmem[1] = 0x90;
	kmem[2] = 0x90;
	kmem[3] = 0x90;
	kmem[4] = 0x90;
	kmem[5] = 0x90;

	// Disable pfs checks
  kmem = (uint8_t *)&gKernelBase[0x0069B730];
  kmem[0] = 0x31;
  kmem[1] = 0xC0;
  kmem[2] = 0xC3;
	kmem[3] = 0x90;

	// Enable debug rif's
  kmem = (uint8_t *)&gKernelBase[0x00665AA0];
  kmem[0] = 0xB0;
  kmem[1] = 0x01;
  kmem[2] = 0xC3;
  kmem[3] = 0x90;

	// Enable debug rifs 2
  kmem = (uint8_t *)&gKernelBase[0x00665AD0];
  kmem[0] = 0xB0;
  kmem[1] = 0x01;
  kmem[2] = 0xC3;
  kmem[3] = 0x90;

  // Enable *all* debugging logs (in vprintf)
  // Patch by: SiSTRo
  kmem = (uint8_t *)&gKernelBase[0x0011B237];
	kmem[0] = 0xEB; // jmp +0x3D
	kmem[1] = 0x3B;

  // flatz allow mangled symbol in dynlib_do_dlsym
  kmem = (uint8_t *)&gKernelBase[0x003FD907];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;
  kmem[3] = 0x90;
  kmem[4] = 0x90;
  kmem[5] = 0x90;

  // Enable mount for unprivileged user
  kmem = (uint8_t *)&gKernelBase[0x0019C67C];
  kmem[0] = 0x90;
  kmem[1] = 0x90;
  kmem[2] = 0x90;
  kmem[3] = 0x90;
  kmem[4] = 0x90;
  kmem[5] = 0x90;

  // patch suword_lwpid
  // has a check to see if child_tid/parent_tid is in kernel memory, and it in so patch it
  // Patch by: JOGolden
  kmem = (uint8_t *)&gKernelBase[0x00406192];
  kmem[0] = 0x90;
  kmem[1] = 0x90;

	kmem = (uint8_t *)&gKernelBase[0x004061A1];
  kmem[0] = 0x90;
  kmem[1] = 0x90;

  // Patch debug setting errors
  kmem = (uint8_t *)&gKernelBase[0x0050985E];
  kmem[0] = 0x00;
  kmem[1] = 0x00;
  kmem[2] = 0x00;
  kmem[3] = 0x00;

  kmem = (uint8_t *)&gKernelBase[0x0050AB2C];
  kmem[0] = 0x00;
  kmem[1] = 0x00;
  kmem[2] = 0x00;
  kmem[3] = 0x00;

  // printf hook patches
  kmem = (uint8_t *)&gKernelBase[0x0011B243];
  kmem[0] = 0xEB;
  kmem[1] = 0x2F;
#endif
}
