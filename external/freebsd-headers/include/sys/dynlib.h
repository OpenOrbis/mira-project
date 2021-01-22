#ifndef __DYNLIB_H__
#define __DYNLIB_H__

#include "../../../../kernel/src/Utils/Kdlsym.hpp"
#ifdef _KERNEL
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/elf64.h>

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
struct SceKdlPerFileInfo;


//variable size? (see pfi alloc)
struct SceKdlPerFileInfo
{
	char _unk0[0x10];
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200
	int32_t _unk10;
	uint64_t _unk18;
	uint64_t _unk20;
#endif

	/* 0x10 | 28 */ caddr_t symtab;
	/* 0x18 | 30 */ size_t symtabsize;
	/* 0x20 | 38 */ caddr_t strtab;
	/* 0x28 | 40 */ size_t strsize;
	/* 0x30 | 48 */ caddr_t pltrela;
	/* 0x38 | 50 */ size_t pltrelasize;
	/* 0x40 | 58 */ caddr_t rela;
	/* 0x48 | 60 */ size_t relasize;
	char _unk50[0x50];
	/* 0xA0 | B8 */ caddr_t buckets;
	char _unkA8[0x8];
	/* 0xB0 | E8  */ int nbuckets;
	/* 0xB8 | D0 */ caddr_t chains;
	char _unkC0[0x8];
	/* 0xC8 | E0 */ int nchains;
	char _unkCC[0x1E];
	//not printed anymore by dump_obj on newer fws.
	/* 0xEA | 102 */ char file_format;
	/* 0xEB | 103 */ char is_prx;
};

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200
//const size_t c = offsetof(struct SceKdlPerFileInfo, nbuckets);

static_assert(offsetof(struct SceKdlPerFileInfo, symtab) == 0x28, "symtab invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, buckets) == 0xB8, "buckets invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, nbuckets) == 0xC8, "nbuckets invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, nchains) == 0xE0, "nchains invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, file_format) == 0x102, "file_format invalid offset.");
#else
static_assert(offsetof(struct SceKdlPerFileInfo, symtab) == 0x10, "symtab invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, buckets) == 0xA0, "buckets invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, nbuckets) == 0xB0, "nbuckets invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, nchains) == 0xC8, "nchains invalid offset.");
static_assert(offsetof(struct SceKdlPerFileInfo, file_format) == 0xEA, "file_format invalid offset.");
#endif

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
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_101 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_204
	Elf64_Size magic;
	Elf64_Size version;
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_250 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_315
	char _unk0[0x10];
#endif
	SLIST_ENTRY(dynlib_obj) link; 	// 0x00
	char* path;						// 0x08
	char _unk10[0x8];				// 0x10
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200
	char _unk18[0x8];				// 0x18
#endif
	int32_t ref_count;				// 0x20
	int32_t dl_ref_count;			// 0x24
	uint64_t handle;				// 0x28
	caddr_t map_base;				// 0x30
	size_t map_size;				// 0x38
	size_t text_size;				// 0x40
	caddr_t database;				// 0x48
	size_t data_size;				// 0x50
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_250
	char _unk58[0x10];				// 0x58
#endif
	size_t vaddr_base;				// 0x68
	caddr_t realloc_base;			// 0x70
	caddr_t entry;					// 0x78

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_101 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_176
	char _unk78[0x70];				// 0x78
#endif

	int32_t tls_index;  //94/80/90/80
	void* tls_init; //98/88/98/88
	size_t tls_init_size; //A0/90/A0/90
	size_t tls_size; //A8/98/A8/98
	size_t tls_offset; //B0/A0/B0/A0
	size_t tls_align; //B8/A8/B8/A8
	caddr_t plt_got; //C0/B0/C0/B0

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_101 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_162
	char _unkC8[0x70];				// 0xC8
#endif

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_170
	char _unkC8[0x38];				// 0xC8
#endif

	caddr_t init; 					//138/100/F0/100/F0
	caddr_t fini; 					//140/108/F8/108/F8
	uint64_t eh_frame_hdr; 			//148/110/100/110/100
	uint64_t eh_frame_hdr_size; 	//150/118/108/118/108
	uint64_t eh_frame; 				//158/120/110/120/110
	uint64_t eh_frame_size; 		//160/128/118/128/118
	int status; 					//168/130/120/130/120
	int flags; 						//bitfield/16C/134/124/134/124
	Objlist unkA; 					//170/138/128/138/128
	Objlist unkB; 					//180/148/138/148/138

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_101 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_176
	char _unk158[0x30];				// 0x158
#endif
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_200
	char _unk148[0x8];				// 0x148
#endif
	struct SceKdlPerFileInfo* pfi;	// 0x150
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_101 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_176
	struct dynlib* parent_dynlib;	//1C8/190 //used to get debug flags for conditional printfs, 2.00 - 2.04 does it with currthread->td_proc->p_dynlib instead
#endif

	char _unk158[0x18]; //1D0/198/158/168/158

#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_200
	char _unk170[0x8];				// 0x170
#endif
#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_500
	char _unk170[0x10];				// 0x170
#endif
};
static_assert(offsetof(struct dynlib_obj, pfi) == 0x150, "invalid pfi");
static_assert(offsetof(struct dynlib_obj, _unk158) == 0x158, "invalid _unk158");

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

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500 && MIRA_PLATFORM <= MIRA_PLATFORM_ORBIS_BSD_700
//static_assert(sizeof(struct dynlib_obj) == 0x188, "5.00-7.00 dynlib object invalid size");
// NOTE: I know the structure size is fucked, but I don't know where :shrug:
// Offsets should be fine tho

static_assert(offsetof(struct dynlib_obj, link) == 0x0, "invalid ntext");
static_assert(offsetof(struct dynlib_obj, path) == 0x8, "invalid path");
static_assert(offsetof(struct dynlib_obj, ref_count) == 0x20, "invalid ref_count");
static_assert(offsetof(struct dynlib_obj, vaddr_base) == 0x68, "invalid vaddr_base");
static_assert(offsetof(struct dynlib_obj, realloc_base) == 0x70, "invalid realloc_base");
static_assert(offsetof(struct dynlib_obj, tls_index) == 0x80, "invalid tls_index");
static_assert(offsetof(struct dynlib_obj, plt_got) == 0xB0, "invalid plt_got");
static_assert(offsetof(struct dynlib_obj, init) == 0xF0, "invalid init");
static_assert(offsetof(struct dynlib_obj, unkB) == 0x138, "invalid unkB");
static_assert(offsetof(struct dynlib_obj, pfi) == 0x150, "invalid pfi");
static_assert(offsetof(struct dynlib_obj, _unk158) == 0x158, "invalid _unk158");

/*const size_t c = offsetof(struct dynlib_obj, _unk158);
const size_t cc = sizeof(struct dynlib_obj);*/
#endif

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