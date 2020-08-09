#ifndef __DYNLIB_H__
#define __DYNLIB_H__

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/sx.h>

struct dynlib_obj;
struct dynlib_obj_dyn;
struct dynlib;

struct dynlib
{
	// 0x390 = titleId
	// titleId = +0x50 from start of dynlib at 0x340
	struct dynlib_obj *slh_first;
	struct dynlib* self;
	struct dynlib_obj* main_obj;
	uint8_t _unk18[0x38];
	char title_id[0xC];
    uint8_t _unk39C[0x8];
	struct sx bind_lock;
	uint32_t procparam_seg_addr;
	uint32_t procparam_seg_filesz;
	void* unpatched_call_addr;
    char _unk144[104];
};

static_assert(offsetof(struct dynlib, slh_first) == 0x0, "invalid slh_first");
static_assert(offsetof(struct dynlib, self) == 0x8, "invalid self");
static_assert(offsetof(struct dynlib, main_obj) == 0x10, "invalid main_obj");
static_assert(offsetof(struct dynlib, _unk18) == 0x18, "invalid");
static_assert(offsetof(struct dynlib, title_id) == 0x50, "invalid title_id");
static_assert(offsetof(struct dynlib, bind_lock) == 0x68, "invalid bind_lock");

#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_650
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_600
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_550
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
#elif MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
static_assert(sizeof(struct dynlib) == 0x100, "invalid dynlib fw size");
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