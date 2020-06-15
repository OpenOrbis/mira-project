#include "Mira.hpp"

//
// Boot
//
#include <Boot/InitParams.hpp>
#include <Boot/Patches.hpp>

//
// Devices
//
#include <Driver/CtrlDriver.hpp>

//
// Managers
//
#include <Messaging/MessageManager.hpp>
#include <Plugins/PluginManager.hpp>
#include <Messaging/Rpc/Server.hpp>

#include <OrbisOS/ThreadManager.hpp>
#include <OrbisOS/Utilities.hpp>

///
/// Utilities
///
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

//
//	Free-BSD Specifics
//
extern "C"
{
	#include <sys/eventhandler.h>
	#include <sys/sysent.h>					// sysent_t
	#include <sys/proc.h>					// proc
	#include <sys/filedesc.h>				// filedesc
	#include <vm/vm.h>
	#include <vm/pmap.h>
	#include <machine/trap.h>
	#include <machine/pmap.h>
	#include <machine/psl.h>
	#include <machine/segments.h>
	#include <machine/trap.h>
}


const char* gNull = "(null)";
uint8_t* gKernelBase = nullptr;
struct logger_t* gLogger = nullptr;

// ++ LM Patches (eventually move this somewhere better)
typedef int (*myIoctl_t)(struct thread* td, struct ioctl_args* uap);
typedef int (*myWorkaround_t)(struct thread* td, void* uap);

static myIoctl_t gIoctl = nullptr;
static myWorkaround_t gWorkaround = nullptr;
// -- LM Patches

Mira::Framework* Mira::Framework::m_Instance = nullptr;
Mira::Framework* Mira::Framework::GetFramework()
{
	if (m_Instance == nullptr)
		m_Instance = new Mira::Framework();
	
	return m_Instance;
}

Mira::Framework::Framework() :
	m_InitParams(),
	m_EventHandlersInstalled(false),
	m_SuspendTag(nullptr),
	m_ResumeTag(nullptr),
	m_ThreadManager(nullptr),
	m_PluginManager(nullptr),
	m_MessageManager(nullptr),
	m_RpcServer(nullptr)
{

}

Mira::Framework::~Framework()
{
	if (!Terminate())
		WriteLog(LL_Error, "could not terminate successfully");
	else
		WriteLog(LL_Info, "terminated successfully");
}

extern "C" void mira_entry(void* args)
{
	// Fill the kernel base address
	gKernelBase = (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall;

	cpu_disable_wp();

	Mira::Boot::Patches::install_prePatches();

	cpu_enable_wp();

    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
	//auto kproc_exit = (void(*)(int ecode))kdlsym(kproc_exit);
	auto vmspace_alloc = (struct vmspace* (*)(vm_offset_t min, vm_offset_t max))kdlsym(vmspace_alloc);
	auto pmap_activate = (void(*)(struct thread *td))kdlsym(pmap_activate);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	//auto avcontrol_sleep = (void(*)(int milliseconds))kdlsym(avcontrol_sleep);

    // Let'em know we made it
	printf("[+] mira has reached stage 2\n");

    // These are the initialization parameters from the loader
    Mira::Boot::InitParams* initParams = static_cast<Mira::Boot::InitParams*>(args);
    if (initParams == nullptr)
    {
		printf("[-] no init params\n");
		kthread_exit();
		return;
	}    

	printf("[+] starting logging\n");
	auto s_Logger = Mira::Utils::Logger::GetInstance();
	if (!s_Logger)
	{
		printf("[-] could not allocate logger\n");
		kthread_exit();
		return;
	}
	// Create new credentials
	WriteLog(LL_Debug, "Creating new thread credentials");
	(void)ksetuid_t(0, curthread);

	// Root and escape our thread
	if (curthread->td_ucred)
	{
		WriteLog(LL_Info, "escaping thread");

		curthread->td_ucred->cr_rgid = 0;
		curthread->td_ucred->cr_svgid = 0;
		
		curthread->td_ucred->cr_uid = 0;
		curthread->td_ucred->cr_ruid = 0;

		if (curthread->td_ucred->cr_prison)
			curthread->td_ucred->cr_prison = *(struct prison**)kdlsym(prison0);

		if (curthread->td_proc->p_fd)
			curthread->td_proc->p_fd->fd_rdir = curthread->td_proc->p_fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);
		
		// Set our auth id as debugger
		curthread->td_ucred->cr_sceAuthID = SceAuthenticationId::Decid;

		// make system credentials
		curthread->td_ucred->cr_sceCaps[0] = SceCapabilites::Max;
		curthread->td_ucred->cr_sceCaps[1] = SceCapabilites::Max;

		WriteLog(LL_Debug, "credentials rooted for new proc");
	}
	
	WriteLog(LL_Debug, "payloadBase: %p", initParams->payloadBase);
	WriteLog(LL_Debug, "allocatedData: %p", initParams->allocatedBase);
	WriteLog(LL_Debug, "entryPoint: %p", initParams->entrypoint);
	WriteLog(LL_Debug, "kernelBase: %p", gKernelBase);
	WriteLog(LL_Debug, "kthread_exit: %p", kthread_exit);
	WriteLog(LL_Debug, "vmspace_alloc: %p", vmspace_alloc);
	WriteLog(LL_Debug, "pmap_activate: %p", pmap_activate);
	WriteLog(LL_Debug, "printf: %p", printf);

	// Create new vm_space
	WriteLog(LL_Debug, "Creating new vm space");
	struct vmspace* vmspace = vmspace_alloc(0, PAGE_SIZE);
	WriteLog(LL_Debug, "here");
	if (!vmspace)
	{
		WriteLog(LL_Error, "vmspace_alloc failed\n");
		kthread_exit();
		return;
	}

	// Assign our new vmspace to our process
	initParams->process->p_vmspace = vmspace;
	if (initParams->process == curthread->td_proc)
	{
		WriteLog(LL_Debug, "here");
		pmap_activate(curthread);
		WriteLog(LL_Debug, "here");
	}

	// Because we have now forked into a new realm of fuckery
	// We need to reserve the first 3 file descriptors in our process
	int descriptor = kopen_t(const_cast<char*>("/dev/console"), 1, 0, curthread);
	WriteLog(LL_Debug, "/dev/console descriptor: %d", descriptor);
	WriteLog(LL_Info, "dup2(desc, 1) result: %d", kdup2_t(descriptor, 1, curthread));
	WriteLog(LL_Info, "dup2(1, 2) result: %d", kdup2_t(1, 2, curthread));

	// Show over UART that we are running in a new process
	WriteLog(LL_Info, "oni_kernelInitialization in new process!\n");

	// Initialize miraframework
	WriteLog(LL_Debug, "Creating MiraFramework...");
	auto s_Framework = Mira::Framework::GetFramework();

	// Set the initparams so we do not lose track of it
	WriteLog(LL_Debug, "Updating initialization parameters...");
	s_Framework->SetInitParams(initParams);

	// We are successfully running
	s_Framework->GetInitParams()->isRunning = true;

	WriteLog(LL_Debug, "Initializing MiraFramework...");
	if (!s_Framework->Initialize())
	{
		WriteLog(LL_Error, "could not initialize mira framework.");
		WriteLog(LL_Debug, "MiraTerminate: %s", s_Framework->Terminate() ? "Success" : "Failure");
		kthread_exit();
		return;
	}

	// At this point we don't need kernel context anymore
	WriteLog(LL_Info, "Mira initialization complete");
	kthread_exit();
}

bool Mira::Framework::SetInitParams(Mira::Boot::InitParams* p_Params)
{
	if (p_Params == NULL)
		return false;
	
	m_InitParams.elfLoader = p_Params->elfLoader;
	m_InitParams.entrypoint = p_Params->entrypoint;
	m_InitParams.isElf = p_Params->isElf;
	m_InitParams.payloadBase = p_Params->payloadBase;
	m_InitParams.payloadSize = p_Params->payloadSize;
	m_InitParams.process = p_Params->process;
	m_InitParams.isRunning = p_Params->isRunning;

	return true;
}

bool Mira::Framework::Initialize()
{
	// TODO: Load settings
	WriteLog(LL_Warn, "FIXME: loading settings not implemented!!!!");

	// Initialize the thread manager
	// NOTE: WE DO NOT WANT TO KILL THREAD MANAGER ON RELOAD
	WriteLog(LL_Debug, "Initializing the thread manager.");
	if (m_ThreadManager == nullptr)
		m_ThreadManager = new Mira::OrbisOS::ThreadManager();
	if (m_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate thread manager.");
		return false;
	}

	if (!m_ThreadManager->OnLoad())
	{
		WriteLog(LL_Error, "could not load the thread manager.");
		return false;
	}

	// Initialize message manager
	WriteLog(LL_Debug, "Initializing the message manager");
	m_MessageManager = new Mira::Messaging::MessageManager();
	if (m_MessageManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate message manager.");
		return false;
	}

	// Initialize plugin manager
	WriteLog(LL_Debug, "Initializing the plugin manager");
	m_PluginManager = new Mira::Plugins::PluginManager();
	if (m_PluginManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate plugin manager.");
		return false;
	}

	// Load the plugin manager
	WriteLog(LL_Debug, "Loading plugin manager");
	if (!m_PluginManager->OnLoad())
	{
		WriteLog(LL_Error, "could not initialize default plugins");
		return false;
	}

	// Install eventhandler's
	WriteLog(LL_Debug, "Installing event handlers");
	if (!InstallEventHandlers())
		WriteLog(LL_Error, "could not register event handlers");

	// Initialize the rpc server
	WriteLog(LL_Debug, "Initializing rpc server");
	m_RpcServer = new Mira::Messaging::Rpc::Server();
	if (m_RpcServer == nullptr)
	{
		WriteLog(LL_Error, "could not allocate rpc server.");
		return false;
	}

	WriteLog(LL_Debug, "Loading rpc server");
	if (!m_RpcServer->OnLoad())
	{
		WriteLog(LL_Error, "could not load rpc server.");
		return false;
	}

	// TODO: Install needed hooks for Mira
	WriteLog(LL_Warn, "FIXME: Syscall table hooks not implemented!!!!");

	// Install device driver
	WriteLog(LL_Warn, "Initializing the /dev/mira control driver");
	m_CtrlDriver = new Mira::Driver::CtrlDriver();
	if (m_CtrlDriver == nullptr)
	{
		WriteLog(LL_Error, "could not allocate control driver.");
		return false;
	}

	// Set the running flag
	m_InitParams.isRunning = true;

	////////////
	// TEST
	////////////
	auto g_obi_create       = (void* (*) (const char* name, int flags)                           )kdlsym(g_obi_create);
	auto g_obi_destroy      = (int   (*) (void* obj)                                             )kdlsym(g_obi_destroy);
	auto g_obi_read         = (int   (*) (void* obj, void* buffer, uint64_t unk1, uint64_t unk2) )kdlsym(g_obi_read);
	auto g_part_ox_get_bank = (int   (*) (void)                                                  )kdlsym(g_part_ox_get_bank);
	auto hexdump            = (int   (*) (void* buffer, uint64_t size, int unk1, int unk2)       )kdlsym(hexdump);

	void* sflash_obj = g_obi_create("sflash0s1.crypt", 0);
	WriteLog(LL_Warn, "g_obi_create(sflash0s1.crypt): %p", sflash_obj);

	char buffer[0x1000];
	int read_ret = g_obi_read(sflash_obj, buffer, 0x200, 0x1000);
	WriteLog(LL_Warn, "g_obi_read(sflash0s1.crypt): %d", read_ret);

	int destroy_ret = g_obi_destroy(sflash_obj);
	WriteLog(LL_Warn, "g_obi_destroy(sflash0s1.crypt): %d", destroy_ret);

	WriteLog(LL_Warn, "Current bank used: %d", g_part_ox_get_bank());
	WriteLog(LL_Warn, "Hexdump:");
	hexdump((void*)buffer, 0x1000, 0, 0);

	/*
    // Mira is now ready ! Now Killing SceShellUI for relaunching UI Process :D
    struct proc* ui_proc = Mira::OrbisOS::Utilities::FindProcessByName("SceShellUI");
    if (ui_proc) {
        Mira::OrbisOS::Utilities::KillProcess(ui_proc);
    } else {
        WriteLog(LL_Error, "Unable to find SceShellUI Process !");
    }

	auto kthread_suspend = (int (*)(struct thread *td, int timo))kdlsym(kthread_suspend);
	struct proc* proc0 = static_cast<struct proc*>(kdlsym(proc0));
	auto _thread_lock_flags = (void (*)(struct thread *td, int opts, const char *file, int line))kdlsym(_thread_lock_flags);
	auto spinlock_exit = (void(*)(void))kdlsym(spinlock_exit);
	struct thread* td = nullptr;
	FOREACH_THREAD_IN_PROC(proc0, td)
	{
		bool veriThreadFound = false;
		thread_lock(td);
		if (strcmp(td->td_name, "SysVeri") == 0)
			veriThreadFound = true;
		thread_unlock(td);

		if (!veriThreadFound)
			continue;

		WriteLog(LL_Debug, "found verification thread, suspending...");
		kthread_suspend(td, 1000);
		WriteLog(LL_Debug, "verification thread suspended...");
	}

	WriteLog(LL_Info, "mira initialized successfully!");*/

	// Hook SceSblSysVeri (Fire fw only)
	//m_SceSblSysVeriHook = new Mira::Utils::Hook::Hook(kdlsym(SceSblSysVeriThread), OnSceSblSysVeri);

	// DBG: Intentionally fault
	//*((uint64_t*)0x1337) = 0xBADBABE;

	/*
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    struct sysent* sysents = sv->sv_table;

	gIoctl = (myIoctl_t)sysents[SYS_IOCTL].sy_call;
	sysents[SYS_IOCTL].sy_call = (sy_call_t*)OnIoctl;

	gWorkaround = (myWorkaround_t)sysents[SYS_WORKAROUND8849].sy_call;
	sysents[SYS_WORKAROUND8849].sy_call = (sy_call_t*)OnWorkaround8849;
	*/

	return true;
}

// ++ LM Patches
int Mira::Framework::OnIoctl(struct thread* p_Thread, struct ioctl_args* p_Uap)
{
	switch (p_Uap->com)
	{
		case 0xFFFFFFFF40048806:
		case 0x40048806:
		{
			((int*)p_Uap->data)[0] = 1;
			p_Thread->td_retval[0] = 0;
			return 0;
		}
	}

	return ((int(*)(struct thread* td, void* uap))gIoctl)(p_Thread, p_Uap);
}

int Mira::Framework::OnWorkaround8849(struct thread* p_Thread, uint32_t* p_Uap)
{
	if (p_Thread == nullptr || p_Uap == nullptr)
		return ((int(*)(struct thread* td, void* uap))gWorkaround)(p_Thread, p_Uap);
	
	if (p_Uap[0] == 0x78028300)
	{
		p_Thread->td_retval[0] = 1;
		return 0;
	}

	return ((int(*)(struct thread* td, void* uap))gWorkaround)(p_Thread, p_Uap);
}

// -- LM Patches

void Mira::Framework::OnSceSblSysVeri(void* p_Reserved)
{
	auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
	kthread_exit();
}

bool Mira::Framework::Terminate()
{
	// Unload the plugin manager
	if (m_PluginManager && !m_PluginManager->OnUnload())
		WriteLog(LL_Error, "could not unload plugin manager");

	// Free the plugin manager
	delete m_PluginManager;
	m_PluginManager = nullptr;

	// Free the rpc server
	if (m_RpcServer && !m_RpcServer->OnUnload())
		WriteLog(LL_Error, "could not unload rpc server");
	
	delete m_RpcServer;
	m_RpcServer = nullptr;

	// Remove all eventhandlers
	if (!RemoveEventHandlers())
		WriteLog(LL_Error, "could not remove event handlers");

	// Remove the device driver
	if (m_CtrlDriver != nullptr)
	{
		delete m_CtrlDriver;
		m_CtrlDriver = nullptr;
	}

	// Unload the thread manager
	if (!m_ThreadManager->OnUnload())
	{
		WriteLog(LL_Error, "could not unload thread manager.");
		delete m_ThreadManager;
		m_ThreadManager = nullptr;
	}

	// Update our running state, to allow the proc to terminate
	m_InitParams.isRunning = false;
	
	return true;
}

bool Mira::Framework::InstallEventHandlers()
{
	if (m_EventHandlersInstalled)
		return false;

	auto eventhandler_register = (eventhandler_tag
	(*)(struct eventhandler_list *list, const char *name,
		void *func, void *arg, int priority))kdlsym(eventhandler_register);

	// Register our event handlers
	//const int32_t prio = 1337;
	m_SuspendTag = EVENTHANDLER_REGISTER(system_suspend_phase1, reinterpret_cast<void*>(Mira::Framework::OnMiraSuspend), GetFramework(), EVENTHANDLER_PRI_FIRST);
	m_ResumeTag = EVENTHANDLER_REGISTER(system_resume_phase1, reinterpret_cast<void*>(Mira::Framework::OnMiraResume), GetFramework(), EVENTHANDLER_PRI_LAST);
	//m_ShutdownTag = EVENTHANDLER_REGISTER(shutdown_pre_sync, reinterpret_cast<void*>(Mira::Framework::OnMiraShutdown), GetFramework(), EVENTHANDLER_PRI_FIRST);

	// Set our event handlers as installed
	m_EventHandlersInstalled = true;

	return true;
}

bool Mira::Framework::RemoveEventHandlers()
{
	if (!m_EventHandlersInstalled)
		return false;
	
	if (m_SuspendTag == nullptr || m_ResumeTag == nullptr)
		return false;

	auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
	auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

	EVENTHANDLER_DEREGISTER(system_suspend_phase1, m_SuspendTag);
	EVENTHANDLER_DEREGISTER(system_resume_phase1, m_ResumeTag);
	//EVENTHANDLER_DEREGISTER(shutdown_pre_sync, m_ShutdownTag);

	m_SuspendTag = nullptr;
	m_ResumeTag = nullptr;
	m_ShutdownTag = nullptr;

	m_EventHandlersInstalled = false;

	return true;
}

void Mira::Framework::OnMiraSuspend(void* __unused p_Reserved)
{
	if (GetFramework() == nullptr)
		return;
	
	WriteLog(LL_Warn, "SUPSEND SUSPEND SUSPEND");
	
	// Handle suspend events
	auto s_RpcServer = GetFramework()->GetRpcServer();
	if (s_RpcServer)
		s_RpcServer->OnSuspend();
	
	auto s_PluginManager = GetFramework()->m_PluginManager;
	if (s_PluginManager)
		s_PluginManager->OnSuspend();
}

void Mira::Framework::OnMiraResume(void* __unused p_Reserved)
{
	if (GetFramework() == nullptr)
		return;
	
	WriteLog(LL_Warn, "RESUME RESUME RESUME");

	// Handle resume events
	auto s_PluginManager = GetFramework()->GetPluginManager();
	if (s_PluginManager)
		s_PluginManager->OnResume();

	auto s_RpcServer = GetFramework()->GetRpcServer();
	if (s_RpcServer)
		s_RpcServer->OnResume();
}

void Mira::Framework::OnMiraShutdown(void* __unused p_Reserved)
{
	auto kproc_exit = (int(*)(int code))kdlsym(kproc_exit);
	auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

	WriteLog(LL_Warn, "SHUTDOWN SHUTDOWN SHUTDOWN");

	if (GetFramework() == nullptr)
		return;
	
	if (!GetFramework()->Terminate())
	{
		WriteLog(LL_Error, "could not terminate cleanly");
		return;
	}

	kproc_exit(0);
	kthread_exit();
}