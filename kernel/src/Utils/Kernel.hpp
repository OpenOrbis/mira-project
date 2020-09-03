#pragma once
#include <Utils/Types.hpp>

#ifdef __cplusplus
extern "C"{
#endif

struct task_struct;
struct proc;
struct proc_vm_map_entry;
struct vnode;
struct iovec;

#define  VM_PROT_GPU_READ ((vm_prot_t)0x10)
#define  VM_PROT_GPU_WRITE ((vm_prot_t)0x20)

#define PROT_CPU_READ 0x1
#define PROT_CPU_WRITE 0x2
#define PROT_CPU_EXEC 0x4
#define PROT_GPU_READ 0x10
#define PROT_GPU_WRITE 0x20

typedef struct _ProcVmMapEntry
{
	char name[32];
	vm_offset_t start;
	vm_offset_t end;
	vm_offset_t offset;
	uint16_t prot;
}ProcVmMapEntry;

#ifdef __cplusplus
typedef enum SceAuthenticationId_t : uint64_t
{
   
    SceVdecProxy = 0x3800000000000003ULL, //same as SceSpkService 
    SceVencProxy = 0x3800000000000004ULL,
    Orbis_audiod = 0x3800000000000005ULL,
    Coredump = 0x3800000000000006ULL,// same as orbis-jsc-compiler.self
    SceSysCore = 0x3800000000000007ULL,
    Orbis_setip = 0x3800000000000008ULL,
    GnmCompositor = 0x3800000000000009ULL,
    SceShellUI = 0x380000000000000fULL, // NPXS20001
    SceShellCore = 0x3800000000000010ULL,
    NPXS20103 = 0x3800000000000011ULL,
    SceGameLiveStreaming = 0x3800000000000012ULL, //NPXS21000 samething and SceNKNetworkProcess 
    SCE_SYS_SERVICES = 0x3800000000010003ULL,
    ScePartyDaemon = 0x3800000000000014ULL,
    MaxAccess = 0x3801000000000013ULL,
    SceAvCapture = 0x3800000000000015ULL,
    SceVideoCoreServer = 0x3800000000000016ULL,
    SceRemotePlay = 0x3800000000000019ULL,
    mini_syscore = 0x3800000000000022ULL,
    UNK_ICC = 0x3800800000000024ULL,
    SceCloudClientDaemon = 0x3800000000000028ULL,
    fs_cleaner = 0x380000000000001dULL,
     sceSblACMgrIsAllowedToUsePupUpdate0 = 0x3800100000000001ULL,
    SceNKWebProcess = 0x3800000000010003ULL,
    SecureWebProcess = 0x3800000010000003ULL,
    SecureUIProcess = 0x3800000000000033ULL,
    SceSysAvControl = 0x380000000000001fULL,
    SceSocialScreenMgr = 0x3800000000000037ULL,
    SceSpZeroConf = 0x380000001000000EULL,
    SceMusicCoreServer = 0x380000000000001aULL,
    SceNKUIProcess = 0x380000000000003cULL,
    sceSblACMgrHasUseHp3dPipeCapability = 0x3800000010000009ULL,
    UNK_PFS = 0x380100000000000AULL,
    sceSblACMgrHasUseHp3dPipeCapability2 = 0x380100000000002CULL,
    OS_UPDATE = 0x3801000000000024ULL,
    VTRM_ADMIN = 0x3800800000000002ULL,
    Decid = 0x3800000000010003,
} SceAuthenticationId;

typedef enum SceCapabilities_t : uint64_t
{
	Max = 0xFFFFFFFFFFFFFFFFULL,
} SceCapabilites;
#endif

/*
	kernelRdmsr

	TODO: Description
*/
extern uint64_t kernelRdmsr(int Register);
extern int proc_rw_mem_pid(int pid, void* ptr, size_t size, void* data, size_t* n, int write);
extern int proc_rw_mem(struct proc* p, void* ptr, size_t size, void* data, size_t* n, int write);

extern struct proc* proc_find_by_name(const char* name);
extern int proc_get_vm_map(struct proc* p, struct proc_vm_map_entry** entries, size_t* num_entries);

extern void build_iovec(struct iovec **iov, int *iovlen, const char *name, const char *val, size_t len);
extern int kernel_vmount(int flags, ...);

extern void	*memcpy(void * __restrict, const void * __restrict, size_t);
extern void	*memmove(void *, const void *, size_t);
extern void	*memset(void *, int, size_t);
extern int	 memcmp(const void *, const void *, size_t);

extern size_t strlen(const char *str);
extern int strcmp(const char *str1, const char *str2);

extern void cpu_enable_wp();
extern void cpu_disable_wp();

extern int cpu_sidt(void *buff);
extern int cpu_sldt(void *buff);
extern int cpu_sgdt(void *buff);

//bool orbisThreadEscape(int32_t p_RealGroupId, int32_t p_SavedGroupId, int32_t p_EffectiveUserId, int32_t p_RealUserId, void* p_Prison, struct vnode* p_RootDir, struct vnode* p_JailRootDir, enum SceAuthenticationId_t p_AuthId, enum SceCapabilities_t p_SceCaps);

#ifdef __cplusplus
}
#endif