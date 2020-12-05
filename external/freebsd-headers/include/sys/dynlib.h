#ifndef __DYNLIB_H__
#define __DYNLIB_H__

#include "../../../../kernel/src/Utils/Kdlsym.hpp"
#ifdef _KERNEL
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/sx.h>

#if !defined(__cplusplus)
#include <sys/stddef.h>
#ifndef static_assert
#define static_asssert
#endif
#endif

typedef STAILQ_HEAD(Struct_Objlist, Struct_Objlist_Entry) Objlist;

struct dynlib_obj;
struct dynlib_obj_dyn;
struct dynlib;
struct dynlib_load_prx_args;
struct dynlib_dlsym_args;
struct dynlib_get_obj_member;

// Syscall 594 : sys_dynlib_load_prx
// struct dynlib_load_prx_args {
//     char*    path;  // const char *
//     uint64_t args;   // size_t
//     uint64_t argp;   // const void *
//     uint32_t flags;  // uint32_t
//     uint64_t pOpt;   // const SceKernelLoadModuleOpt *
//     uint64_t pRes;   // int *
// };
struct dynlib_load_prx_args
{
  const char *prx_path;
  int flags;
  int *handle_out;
  uint64_t unk; //never used in (kernel 5.05) and always 0 (libkernel 7.00);
};

struct dynlib_dlsym_args {
    int32_t handle;
    const char* symbol;
    void** address_out;
};

struct dynlib_get_obj_member {
    uint32_t handle;
    uint32_t index;
    uint64_t value;
};

// Thank you flatz for 1.62
// Thank you ChendoChap for fixing newer fw's
struct dynlib
{
	SLIST_HEAD(, dynlib_obj) objs;
	struct dynlib* self;
	struct dynlib_obj* main_obj;
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400
	struct dynlib_obj* libkernel_obj;
#endif
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_350
	struct dynlib_obj* asan_obj;
#endif
	uint32_t nmodules; // module count
	char unk2C[0x4];
	Objlist obj_list_0;
	Objlist obj_list_1;
	Objlist obj_list_2;
	Objlist obj_list_3;
	struct sx bind_lock;
	char unk90[0x18];
	uint8_t unkA8[0x8];
    uint8_t unkB0[0x8];
    uint8_t unkB8[0x8];
    uint8_t unkC0[0x4]; // set to 1
    uint8_t unkC4[0x4]; // set to 1, this and above optimized to one 0x100000001 (on creation)
	void* procparam_seg_addr;
	uint64_t procparam_seg_filesz;
	void* unpatched_call_addr;

	// [ includes
	// ( up to

	// [1.xx - 2.00)
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_100 && MIRA_PLATFORM < MIRA_PLATFORM_ORBIS_BSD_200
	char unkD0[0x4];
	char unkD4[0x8];
	char unkDC;
	char unkDD;
	char unkDE;
	char unkDF;
#endif

// [2.00 - 3.15]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_315
	int rtld_dbg_msg_flag; 			//D0
	int is_sandboxed;				//D4
#endif

// [2.50 - 3.15]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_250 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_315
	char unkD8[0x8];
#endif

// [3.50 - 3.70]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_350 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_370
    uint32_t restrict_flags; //D8
    uint32_t no_dynamic_segment; //DC
    int is_sandboxed; //E0
    char unkE4[0x4];
#endif

// [4.00 - 6.20]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_620
    void* sysc_s00_pointer;
    void* sysc_e00_pointer;
    uint32_t restrict_flags;
    uint32_t no_dynamic_segment;
    int is_sandboxed;
    char unkFC[0x4];
#endif

// [6.50 - *]
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_650
	void* __freeze_pointer;
    void* sysc_s00_pointer;
    void* sysc_e00_pointer;
    uint32_t restrict_flags; //flags of some kind, conditionally zeroes out some stuff in the dynlib  info_ex syscall and other places as well.
    uint32_t no_dynamic_segment; //also flags, used to conditionally load the asan? other bit used for sys_mmap_dmem?
    int is_sandboxed; //((proc->p_fd->fd_rdir != rootvnode) ? 1 : 0)   -> used to determine if it should use random path or system/ to load modules
	uint8_t unk104[0x4];
#endif

};

// Credits: flatz
struct dynlib_obj_dyn
{
	void* symtab_addr;
	uint64_t symtab_size;

	void* strtab_addr;
	uint64_t strtab_size;
};

// Credits: flatz
struct dynlib_obj
{
	SLIST_ENTRY(dynlib_obj) link; 	// 0x00
	char* path;						// 0x08
	// TODO: Finish the rest of the structure
};

const int s = sizeof(struct dynlib);
// The maximum size is found by looking up function
// dynlib_proc_initialize_step1
// look for the first malloc call for M_DYNLIB
// then get the size

/*
	6.72 = 0x104
	5.05 = 0x100
	1.00 = 0xE0
*/

#if defined(MIRA_CHECKS)
static_assert(offsetof(struct dynlib, objs) == 0x0, "invalid slh_first");
static_assert(offsetof(struct dynlib, self) == 0x8, "invalid self");
static_assert(offsetof(struct dynlib, main_obj) == 0x10, "invalid main_obj");

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_650
static_assert(sizeof(struct dynlib) == 0x108, "invalid dynlib fw size");
static_assert(offsetof(struct dynlib, libkernel_obj) == 0x18, "invalid libkernel_obj");
static_assert(offsetof(struct dynlib, asan_obj) == 0x20, "invalid libkernel_obj");
static_assert(offsetof(struct dynlib, unk2C) == 0x2C, "invalid");
static_assert(offsetof(struct dynlib, obj_list_0) == 0x30, "invalid obj_list_0");
static_assert(offsetof(struct dynlib, obj_list_1) == 0x40, "invalid obj_list_1");
static_assert(offsetof(struct dynlib, obj_list_2) == 0x50, "invalid obj_list_2");
static_assert(offsetof(struct dynlib, obj_list_3) == 0x60, "invalid obj_list_3");
static_assert(offsetof(struct dynlib, bind_lock) == 0x70, "invalid bind_lock");
static_assert(offsetof(struct dynlib, unk90) == 0x90, "invalid bind_lock");
static_assert(offsetof(struct dynlib, procparam_seg_addr) == 0xC8, "invalid procparam_seg_addr");
static_assert(offsetof(struct dynlib, procparam_seg_filesz) == 0xD0, "invalid procparam_seg_filesz");
static_assert(offsetof(struct dynlib, unpatched_call_addr) == 0xD8, "invalid unpatched_call_addr");
static_assert(offsetof(struct dynlib, restrict_flags) == 0xF8, "6.00-6.50 restrict_flags invalid offset");
static_assert(offsetof(struct dynlib, no_dynamic_segment) == 0xFC, "6.00-6.50 no_dynamic_segment invalid offset");
static_assert(offsetof(struct dynlib, is_sandboxed) == 0x100, "6.00-6.50 is_sandboxed invalid offset");
static_assert(offsetof(struct dynlib, unk104) == 0x104, "6.00-6.50 unkFC invalid offset");
#endif

// Check 6.00-6.50
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_600 && MIRA_PLATFORM < MIRA_PLATFORM_ORBIS_BSD_650
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
static_assert(offsetof(struct dynlib, libkernel_obj) == 0x18, "invalid libkernel_obj");
static_assert(offsetof(struct dynlib, asan_obj) == 0x20, "invalid libkernel_obj");
static_assert(offsetof(struct dynlib, unk2C) == 0x2C, "invalid");
static_assert(offsetof(struct dynlib, obj_list_0) == 0x30, "invalid obj_list_0");
static_assert(offsetof(struct dynlib, obj_list_1) == 0x40, "invalid obj_list_1");
static_assert(offsetof(struct dynlib, obj_list_2) == 0x50, "invalid obj_list_2");
static_assert(offsetof(struct dynlib, obj_list_3) == 0x60, "invalid obj_list_3");
static_assert(offsetof(struct dynlib, bind_lock) == 0x70, "invalid bind_lock");
static_assert(offsetof(struct dynlib, unk90) == 0x90, "invalid bind_lock");
static_assert(offsetof(struct dynlib, procparam_seg_addr) == 0xC8, "invalid procparam_seg_addr");
static_assert(offsetof(struct dynlib, procparam_seg_filesz) == 0xD0, "invalid procparam_seg_filesz");
static_assert(offsetof(struct dynlib, unpatched_call_addr) == 0xD8, "invalid unpatched_call_addr");
static_assert(offsetof(struct dynlib, restrict_flags) == 0xF0, "6.00-6.50 restrict_flags invalid offset");
static_assert(offsetof(struct dynlib, no_dynamic_segment) == 0xF4, "6.00-6.50 no_dynamic_segment invalid offset");
static_assert(offsetof(struct dynlib, is_sandboxed) == 0xF8, "6.00-6.50 is_sandboxed invalid offset");
static_assert(offsetof(struct dynlib, unkFC) == 0xFC, "6.00-6.50 unkFC invalid offset");
#endif

// Check 4.00-6.20
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400 && MIRA_PLATFORM < MIRA_PLATFORM_ORBIS_BSD_620
static_assert(sizeof(struct dynlib) == 0x100, "4.00-2.00 dynlib invalid size");
static_assert(offsetof(struct dynlib, nmodules) == 0x28, "4.00-6.20 nmodules invalid offset");
static_assert(offsetof(struct dynlib, bind_lock) == 0x70, "4.00-6.20 bind_lock invalid offset");
static_assert(offsetof(struct dynlib, procparam_seg_addr) == 0xC8, "4.00-6.20 procparam_seg_addr invalid offset");
static_assert(offsetof(struct dynlib, unkFC) == 0xFC, "4.00-6.20 unkFC invalid offset");
#endif

// Check 1.00
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_100 && MIRA_PLATFORM < MIRA_PLATFORM_ORBIS_BSD_200
static_assert(sizeof(struct dynlib) == 0xE0, "1.xx dynlib invalid size");
static_assert(offsetof(struct dynlib, nmodules) == 0x18, "1.xx nmodules invalid offset");
static_assert(offsetof(struct dynlib, bind_lock) == 0x60, "1.xx bind_lock invalid offset");
static_assert(offsetof(struct dynlib, procparam_seg_addr) == 0xB8, "1.xx procparam_seg_addr invalid offset");
static_assert(offsetof(struct dynlib, unkD4) == 0xD4, "1.xx unkD4 invalid offset");
#endif

#endif // MIRA_CHECKS

#endif // _KERNEL

#endif // __DYNLIB_H__