#pragma once
#include <Utils/Types.hpp>

struct task_struct;
struct proc;
struct proc_vm_map_entry;
struct vnode;

struct proc_vm_map_entry
{
	char name[32];
	vm_offset_t start;
	vm_offset_t end;
	vm_offset_t offset;
	uint16_t prot;
};

typedef enum SceAuthenticationId_t : uint64_t
{
	SceVdecProxy = 0x3800000000000003,
	SceVencProxy = 0x3800000000000004,
	Orbis_audiod = 0x3800000000000005,
	Coredump = 0x3800000000000006,
	SceSysCore = 0x3800000000000007,
	Orbis_setip = 0x3800000000000008,
	GnmCompositor = 0x3800000000000009,
	SceShellUI = 0x380000000000000f, // NPXS20001
	SceShellCore = 0x3800000000000010,
	NPXS20103 = 0x3800000000000011,
	NPXS21000 = 0x3800000000000012,
	// TODO: Fill in the rest
	Decid = 0x3800000000010003,
} SceAuthenticationId;

typedef enum SceCapabilities_t : uint64_t
{
	Max = 0xFFFFFFFFFFFFFFFFULL,
} SceCapabilites;

/*
	kernelRdmsr

	TODO: Description
*/
extern "C" uint64_t kernelRdmsr(int Register);
int proc_rw_mem_pid(int pid, void* ptr, size_t size, void* data, size_t* n, int write);
int proc_rw_mem(struct proc* p, void* ptr, size_t size, void* data, size_t* n, int write);

struct proc* proc_find_by_name(const char* name);
int proc_get_vm_map(struct proc* p, struct proc_vm_map_entry** entries, size_t* num_entries);


extern "C" void	*memcpy(void * __restrict, const void * __restrict, size_t);
extern "C" void	*memmove(void *, const void *, size_t);
extern "C" void	*memset(void *, int, size_t);
extern "C" int	 memcmp(const void *, const void *, size_t);

extern "C" size_t strlen(const char *str);
extern "C" int strcmp(const char *str1, const char *str2);

bool orbisThreadEscape(int32_t p_RealGroupId, int32_t p_SavedGroupId, int32_t p_EffectiveUserId, int32_t p_RealUserId, void* p_Prison, struct vnode* p_RootDir, struct vnode* p_JailRootDir, SceAuthenticationId p_AuthId, SceCapabilites p_SceCaps);