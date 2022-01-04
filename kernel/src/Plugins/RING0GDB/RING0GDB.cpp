#include "RING0GDB.hpp"
#include "kdl.h"
#include "log.h"
#include "syscalls.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Hook.hpp>

#include <Utils/SysWrappers.hpp>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/stdint.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <vm/vm.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/sx.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/sysent.h>

#include <Mira.hpp>
#include <Utils/Kdlsym.hpp>

void gdbstub_set_debug_traps (void);

using namespace Mira::Plugins;

RING0GDB::RING0GDB()
{
    
}

RING0GDB::~RING0GDB()
{

}

bool RING0GDB::OnLoad()
{
    return this->SpawnKproc();
}

bool RING0GDB::OnUnload()
{
    return true;
}

bool RING0GDB::OnSuspend()
{
    return OnUnload();
}

bool RING0GDB::OnResume()
{
    return this->SpawnKproc();
}

void RING0GDB::GetRoot()
{
    struct prison **prison0 = NULL;
    struct vnode **rootvnode = NULL;
    struct thread *td = curthread;

	/* Resolve credentials */
	struct ucred *cred;
	struct filedesc *fd;

	fd = td->td_proc->p_fd;
	cred = td->td_proc->p_ucred;

	/* Escalate process to uid0 */
	cred->cr_uid = 0;
	cred->cr_ruid = 0;
	cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;

	/* Break out of FreeBSD jail */
	prison0 = (struct prison **)kdlsym(prison0);
	cred->cr_prison = prison0[0];

	/* Set sony auth ID flag */
	cred->cr_sceAuthID = 0x3800000000000007ULL;

	/* Obtain system credentials for Sony stuff */
	cred->cr_sceCaps[0] = 0xffffffffffffffff;
	cred->cr_sceCaps[1] = 0xffffffffffffffff;

	/* Set vnode to real root "/" to defeat sandbox */
	rootvnode = (struct vnode **)kdlsym(rootvnode);
	fd->fd_rdir = rootvnode[0];
	fd->fd_jdir = rootvnode[0];
}

bool RING0GDB::SpawnKproc()
{
	auto kproc_create = (int(*)(void (*func)(void *), void *arg, struct proc **newpp, int flags, int pages, const char *fmt, ...)) kdlsym(kproc_create);
    auto kmalloc = (void*(*)(size_t size, struct	malloc_type *type, int Flags))kdlsym(malloc);
    auto kM_TEMP = (struct	malloc_type *)kdlsym(M_TEMP);

    struct proc *p = (struct proc*)kmalloc(sizeof(struct proc), kM_TEMP, 0x102);
    WriteLog(LL_Info, "RING0GDBMain allocated at: 0x%llx", (uint64_t)p);
    WriteLog(LL_Info, "Spawing KprocMain...");
    kproc_create(RING0GDB::RING0GDBMain, NULL, &p, 0, 0, "RING0GDB");
    return true;
}

struct thread *RING0GDBMainThread;

void RING0GDB::RING0GDBMain(void *)
{
	WriteLog(LL_Info, "RING0GDB::RING0GDBMain Getting root...");    
    RING0GDB::GetRoot();

    // create new vmspace
    WriteLog(LL_Info, "RING0GDB::RING0GDBMain Creating new vmspace...");
	vm_offset_t sv_minuser;
	struct vmspace *vmspace;
	struct proc *p = curproc;
	struct sysentvec *sv = (struct sysentvec *)kdlsym(self_orbis_sysvec);
    
    // allocate user mode virtual space
    auto kvmspace_alloc = (struct vmspace*(*)(vm_offset_t min, vm_offset_t max)) kdlsym(vmspace_alloc);
    auto kpmap_activate = (void(*)(struct thread *td)) kdlsym(pmap_activate);
	

	sv_minuser = MAX(sv->sv_minuser, PAGE_SIZE);
	vmspace = kvmspace_alloc(sv_minuser, sv->sv_maxuser);
	if (!vmspace)
	{
		WriteLog(LL_Error, "RING0GDB::RING0GDBMain Could not allocate new vm space\n");
		return;
	}

    // setup vmspace
    WriteLog(LL_Info, "RING0GDB::RING0GDBMain Activate vmspace...");
	p->p_vmspace = vmspace;
	if (p == curthread->td_proc)
		kpmap_activate(curthread);

    WriteLog(LL_Info, "RING0GDB::RING0GDBMain Creating stdin/out/err...\n");
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);

	WriteLog(LL_Info, "RING0GDB::RING0GDBMain Starting server: PROTOCOL PORT 9946\n");
	//Print thread info
    struct thread *td = curthread;
    /* Resolve credentials */
	struct ucred *cred;
    cred = td->td_proc->p_ucred;
    LOG_DBG("sceAuthID: 0x%llx\n",cred->cr_sceAuthID);
    LOG_DBG("cr_sceCaps[0]: 0x%llx\n",cred->cr_sceCaps[0]);
    LOG_DBG("cr_sceCaps[1]: 0x%llx\n",cred->cr_sceCaps[1]);
    // Start server
	LOG_DBG("PID: %d\n", ps4gdb_sys_getpid());	

	uint8_t idtr[10];
	cpu_sidt((struct idt*)&idtr);

	uint64_t *base = (uint64_t*)&idtr[2];
	uint16_t *limit = (uint16_t*)&idtr[0];

	LOG_DBG("kernelbase: 0x%llx\n", (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall);
	LOG_DBG("idtr->base: 0x%llx\n", *base);
	LOG_DBG("idtr->limit: 0x%llx\n", *limit);

	// Setup debug traps
	RING0GDBMainThread = curthread;
	gdbstub_set_debug_traps();

	//Endlessloop here to keep the process alive
	PS4GDB_kavcontrol_sleep();
	LOG_DBG("Entering main process sleep\n");
	int kill_this_process = 0;
	while(1){ 
		kavcontrol_sleep(100000); 
		if(kill_this_process == 1)
			break;
	}	

	return;
}