#ifndef __DYNLIB_H__
#define __DYNLIB_H__

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/sx.h>

typedef STAILQ_HEAD(Struct_Objlist, Struct_Objlist_Entry) Objlist;

struct dynlib_obj;
struct dynlib_obj_dyn;
struct dynlib;

// Thank you flatz for 1.62
// Thank you ChendoChap for fixing newer fw's
struct dynlib
{
	SLIST_HEAD(, dynlib_obj) objs;
	struct dynlib* self;
	struct dynlib_obj* main_obj;
	struct dynlib_obj* libkernel_obj;
	struct dynlib_obj* asan_obj;
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

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_620
	void* __freeze_pointer;
    void* sysc_s00_pointer;
    void* sysc_e00_pointer;
    uint32_t restrict_flags; //flags of some kind, conditionally zeroes out some stuff in the dynlib  info_ex syscall and other places as well.
    uint32_t no_dynamic_segment; //also flags, used to conditionally load the asan? other bit used for sys_mmap_dmem?
    int is_sandboxed; //((proc->p_fd->fd_rdir != rootvnode) ? 1 : 0)   -> used to determine if it should use random path or system/ to load modules
	uint8_t unkFC[0x4];
#endif
} __attribute__((packed));

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
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_620
//static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
static_assert(offsetof(struct dynlib, objs) == 0x0, "invalid slh_first");
static_assert(offsetof(struct dynlib, self) == 0x8, "invalid self");
static_assert(offsetof(struct dynlib, main_obj) == 0x10, "invalid main_obj");
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
static_assert(offsetof(struct dynlib, __freeze_pointer) == 0xE0, "invalid freeze pointer");
static_assert(offsetof(struct dynlib, sysc_s00_pointer) == 0xE8, "invalid sysc_s00_pointer");
static_assert(offsetof(struct dynlib, sysc_e00_pointer) == 0xF0, "invalid sysc_e00_pointer");
static_assert(offsetof(struct dynlib, restrict_flags) == 0xF8, "invalid unpatched_call_addr");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_600
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_550
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
static_assert(offsetof(struct dynlib, bind_lock) == 0x70, "bind_lock invalid");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_455
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_400
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_355
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_162
static_assert(sizeof(struct dynlib) == 0xE0);
#endif

#endif // _KERNEL

#endif // __DYNLIB_H__