#include "ModuleLoader.hpp"
#include <Messaging/Rpc/Connection.hpp>
#include <Messaging/MessageManager.hpp>

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Hook.hpp>

#include <Utils/SysWrappers.hpp>

#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/filedesc.h>
#include <sys/syslimits.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/sysent.h>
#include <sys/uio.h>
#include <vm/vm.h>
#include <fcntl.h>
#include <Mira.hpp>
#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;

ModuleLoader::ModuleLoader()
{
    
}

ModuleLoader::~ModuleLoader()
{

}

void ModuleLoader::GetRoot()
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

bool ModuleLoader::SpawnKproc()
{
    auto kproc_create = (int(*)(void (*func)(void *), void *arg, struct proc **newpp, int flags, int pages, const char *fmt, ...)) kdlsym(kproc_create);
    auto kmalloc = (void*(*)(size_t size, struct	malloc_type *type, int Flags))kdlsym(malloc);
    auto kM_TEMP = (struct	malloc_type *)kdlsym(M_TEMP);

    struct proc *p = (struct proc*)kmalloc(sizeof(struct proc), kM_TEMP, 0x102);
    WriteLog(LL_Info, "KprocMain allocated at: 0x%llx", (uint64_t)p);
    WriteLog(LL_Info, "Spawing KprocMain...");
    kproc_create(ModuleLoader::KprocMain, NULL, &p, 0, 0, "MiraModuleLoader");
    return true;
}

void ModuleLoader::KprocMain(void *)
{
    WriteLog(LL_Info, "ModuleLoader::KprocMain Getting root...");    
    ModuleLoader::GetRoot();

    // create new vmspace
    WriteLog(LL_Info, "ModuleLoader::KprocMain Creating new vmspace...");
	vm_offset_t sv_minuser;
	struct vmspace *vmspace;
	struct proc *p = curproc;
	struct sysentvec *sv = (struct sysentvec *)kdlsym(self_orbis_sysvec);
    
    // allocate user mode virtual space
    auto kvmspace_alloc = (struct vmspace*(*)(vm_offset_t min, vm_offset_t max)) kdlsym(vmspace_alloc);
    auto kpmap_activate = (void(*)(struct thread *td)) kdlsym(pmap_activate);
    auto kmem_alloc = (vm_offset_t(*)(vm_map_t, vm_size_t))kdlsym(kmem_alloc);

	sv_minuser = MAX(sv->sv_minuser, PAGE_SIZE);
	vmspace = kvmspace_alloc(sv_minuser, sv->sv_maxuser);
	if (!vmspace)
	{
		WriteLog(LL_Error, "ModuleLoader::KprocMain Could not allocate new vm space\n");
		return;
	}

    // setup vmspace
    WriteLog(LL_Info, "ModuleLoader::KprocMain Activate vmspace...");
	p->p_vmspace = vmspace;
	if (p == curthread->td_proc)
		kpmap_activate(curthread);

    WriteLog(LL_Info, "ModuleLoader::KprocMain Creating stdin/out/err...\n");
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    kopen_t("/dev/console", O_RDONLY,0, curthread);
    

    WriteLog(LL_Info, "ModuleLoader::KprocMain Starting server...\n");
    // Create a socket
    WriteLog(LL_Info, "ModuleLoader::KprocMain Creating socket...\n");
    int ms  = ksocket_t(AF_INET, SOCK_STREAM, 0, curthread);
    if (ms < 0)
    {
        WriteLog(LL_Error, "could not initialize socket (%d).", ms);
        ms = 0;
        return;
    }
    WriteLog(LL_Info, "ModuleLoader::KprocMain socket created: (%d).", s);

    struct sockaddr_in m_Address;
    uint16_t m_Port = 9025;

    // Set our server to listen on 0.0.0.0:<port>
    auto memset = (void*(*)(void *s, int c, size_t n))kdlsym(memset);
    memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
    m_Address.sin_addr.s_addr = htonl(INADDR_ANY);
    m_Address.sin_port = htons(m_Port);
    m_Address.sin_len = sizeof(m_Address);

    // Bind to port
    WriteLog(LL_Info, "ModuleLoader::KprocMain Binding to port %d", m_Port);
    int s_Ret = kbind_t(ms, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address), curthread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not bind socket (%d).", s_Ret);
        kshutdown_t(ms, SHUT_RDWR, curthread);
        kclose_t(ms, curthread);
        return;
    }
    WriteLog(LL_Info, "ModuleLoader::KprocMain socket (%d) bound to port (%d).", ms, m_Port);

    // Listen on the port for new connections
    s_Ret = klisten_t(ms, 1, curthread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "ModuleLoader::KprocMain could not listen on socket (%d).", s_Ret);
        kshutdown_t(ms, SHUT_RDWR, curthread);
        kclose_t(ms, curthread);
        return;
    }


    for(;;)
    {
        WriteLog(LL_Info, "ModuleLoader::KprocMain accepting new payload on 9025 (%d)", s);
        int s = kaccept_t(ms, (sockaddr *) NULL, 0, curthread);

        if(s < 0)
        {
            WriteLog(LL_Error, "Accept failed, maybe we got a suspend signal");
            WriteLog(LL_Error, "Killing process\n");
            kshutdown_t(ms, SHUT_RDWR, curthread);
            kclose_t(ms, curthread);
            return;
        }

        WriteLog(LL_Info, "Got connection %d", s);

        // Alloc RWX memory
        WriteLog(LL_Info, "Allocating memory for module");
        unsigned char *mem = NULL;
        vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
        mem = (unsigned char *)kmem_alloc(map, 0x80000);
        if (mem == NULL)
        {
            WriteLog(LL_Error, "Memory allocation failed");
        }
        else
        {
            WriteLog(LL_Info, "Memory allocated at 0x%llx", mem);

            // Read payload from socket
            int32_t currentSize = 0;
            int32_t recvSize = 0;

            // Recv one byte at a time until we get our buffer            
            while ((recvSize = krecv_t(s, mem + currentSize, 0x80000 - currentSize, 0, curthread)) > 0)
                currentSize += recvSize;

            // Close socket
            WriteLog(LL_Info, "Received %d bytes", currentSize);            
            WriteLog(LL_Info, "Closing socket %d", s);
            kclose_t(s, curthread);

            auto kproc_create = (int(*)(void (*func)(void *), void *arg, struct proc **newpp, int flags, int pages, const char *fmt, ...)) kdlsym(kproc_create);
            auto kmalloc = (void*(*)(size_t size, struct	malloc_type *type, int Flags))kdlsym(malloc);
            auto kM_TEMP = (struct	malloc_type *)kdlsym(M_TEMP);

            struct proc *p = (struct proc*)kmalloc(sizeof(struct proc), kM_TEMP, 0x102);
            WriteLog(LL_Info, "ModuleProc allocated at: 0x%llx", (uint64_t)p);
            WriteLog(LL_Info, "Spawing ModuleProc...");
            kproc_create((void(*)(void *))mem, NULL, &p, 0, 0, "MiraModuleProc");
        }
    }
}

bool ModuleLoader::OnLoad()
{
    WriteLog(LL_Info, "Module Loader OnLoad");
    return this->SpawnKproc();
}

bool ModuleLoader::OnUnload()
{
    return true;
}

bool ModuleLoader::OnSuspend()
{
    WriteLog(LL_Info, "Module Loader OnSuspend");
    return true;
}

bool ModuleLoader::OnResume()
{
    WriteLog(LL_Info, "Module Loader OnResume");
    return this->SpawnKproc();
}