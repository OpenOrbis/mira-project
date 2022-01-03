#include "PS4GDB.hpp"
#include "kdl.h"
#include "log.h"
#include "gdbstub.hpp"
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

using namespace Mira::Plugins;

PS4GDB::PS4GDB()
{
    
}

PS4GDB::~PS4GDB()
{

}

bool PS4GDB::OnLoad()
{
    return this->SpawnKproc();
}

bool PS4GDB::OnUnload()
{
    return true;
}

bool PS4GDB::OnSuspend()
{
    return OnUnload();
}

bool PS4GDB::OnResume()
{
    return this->SpawnKproc();
}

void PS4GDB::GetRoot()
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

bool PS4GDB::SpawnKproc()
{
    auto kproc_create = (int(*)(void (*func)(void *), void *arg, struct proc **newpp, int flags, int pages, const char *fmt, ...)) kdlsym(kproc_create);
    auto kmalloc = (void*(*)(size_t size, struct	malloc_type *type, int Flags))kdlsym(malloc);
    auto kM_TEMP = (struct	malloc_type *)kdlsym(M_TEMP);

    struct proc *p = (struct proc*)kmalloc(sizeof(struct proc), kM_TEMP, 0x102);
    WriteLog(LL_Info, "PS4GDBMain allocated at: 0x%llx", (uint64_t)p);
    WriteLog(LL_Info, "Spawing KprocMain...");
    kproc_create(PS4GDB::PS4GDBMain, NULL, &p, 0, 0, "PS4GDB");
    return true;
}

void PS4GDB::PS4GDBMain(void *)
{
	WriteLog(LL_Info, "PS4GDB::PS4GDBMain Getting root...");    
    PS4GDB::GetRoot();

    // create new vmspace
    WriteLog(LL_Info, "PS4GDB::PS4GDBMain Creating new vmspace...");
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
		WriteLog(LL_Error, "PS4GDB::PS4GDBMain Could not allocate new vm space\n");
		return;
	}

    // setup vmspace
    WriteLog(LL_Info, "PS4GDB::PS4GDBMain Activate vmspace...");
	p->p_vmspace = vmspace;
	if (p == curthread->td_proc)
		kpmap_activate(curthread);

    WriteLog(LL_Info, "PS4GDB::PS4GDBMain Creating stdin/out/err...\n");
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);

	WriteLog(LL_Info, "PS4GDB::PS4GDBMain Starting server: CMD PORT 8146 - PROTOCOL PORT 8846\n");
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
	LOG_DBG("Starting server...\n");
    ps4gdb_start_cmd_server(8146,8846);
}