// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//-V::668

#include "Mira.hpp"
#if __has_include("GitHash.hpp")
#include "GitHash.hpp"
#endif

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
#include <Trainers/TrainerManager.hpp>

#include <OrbisOS/Utilities.hpp>

///
/// Utilities
///
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Types.hpp>
#include <OrbisOS/Utilities.hpp>

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
	#include <sys/systm.h>
	#include <sys/sx.h>
}

#include <stdarg.h>

const char* gNull = "(null)";
uint8_t* gKernelBase = nullptr;
struct logger_t* gLogger = nullptr;

Mira::Framework::vprintf_t Mira::Framework::o_vprintf = nullptr;

Mira::Framework* Mira::Framework::m_Instance = nullptr;
Mira::Framework* Mira::Framework::GetFramework()
{
	if (m_Instance == nullptr)
	{
		// Debug logging to check for new framework
		// TODO: Remove debug logging
		WriteLog(LL_Warn, "creating new framework.");
		
		m_Instance = new Mira::Framework();
	}

	return m_Instance;
}


int OnVPrintf(const char* fmt, va_list list)
{
	// Fix sony's dumbass bullshit
	auto __sx_xlock = (int (*)(struct sx *sx, int opts, const char* file, int line))kdlsym(_sx_xlock);
	auto __sx_xunlock = (int (*)(struct sx *sx, const char* file, int line))kdlsym(_sx_xunlock);

	// Get instance of the framework
	auto s_Framework = Mira::Framework::GetFramework();
	if (s_Framework == nullptr)
		return 0;
	
	// Get the hook
	auto s_Hook = s_Framework->m_PrintfHook;
	if (s_Hook == nullptr)
		return 0;

	auto s_Ret = 0;
	
	__sx_xlock(&s_Framework->m_PrintfLock, 0, __FILE__, __LINE__);
	do
	{
		// Call the original function
		s_Ret = Mira::Framework::o_vprintf(fmt, list);

	} while (false);
	__sx_xunlock(&s_Framework->m_PrintfLock, __FILE__, __LINE__);

	return s_Ret;
}

Mira::Framework::Framework() :
	m_InitParams(),
	m_Configuration { 0 },
	m_State(State::None),
	m_SuspendTag(nullptr),
	m_ResumeTag(nullptr),
	m_ProcessExec(nullptr),
	m_ProcessExecEnd(nullptr),
	m_ProcessExit(nullptr),
	m_PluginManager(nullptr),
	m_MessageManager(nullptr),
	m_TrainerManager(nullptr),
	m_CtrlDriver(nullptr)
{
	InitializeMiraConfig(&m_Configuration);

	// Initialize a mutex to prevent overlapping spam
	auto sx_init_flags = (void(*)(struct sx* sx, const char* description, int opts))kdlsym(_sx_init_flags);
	sx_init_flags(&m_PrintfLock, "fcksony", 0);

	m_PrintfHook = subhook_new((void*)kdlsym(vprintf), (void*)OnVPrintf, subhook_flags_t::SUBHOOK_64BIT_OFFSET);
	o_vprintf = (vprintf_t)subhook_get_trampoline(m_PrintfHook);
	//subhook_install(m_PrintfHook);
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
#ifdef GIT_HASH
	WriteLog(LL_Debug, "gitHash: %s", GIT_HASH);
#endif
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
	struct vmspace* vmspace = vmspace_alloc(0, PAGE_SIZE * 2048); // Allocate 8MiB
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
		WriteLog(LL_Debug, "Activating vmspace physical map");
		pmap_activate(curthread);
	}

	// Because we have now forked into a new realm of fuckery
	// We need to reserve the first 3 file descriptors in our process
	WriteLog(LL_Debug, "Creating initial 3 file descriptors (0, 1, 2).");
	int s_Descriptor = kopen_t(const_cast<char*>("/dev/console"), 1, 0, curthread);
	WriteLog(LL_Debug, "/dev/console descriptor: %d", s_Descriptor);

	int s_Ret = kdup2_t(s_Descriptor, 1, curthread);
	WriteLog(LL_Info, "dup2(desc, 1) result: %d", s_Ret);
	
	s_Ret = kdup2_t(1, 2, curthread);
	WriteLog(LL_Info, "dup2(1, 2) result: %d", s_Ret);

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
	WriteLog(LL_Info, "Mira initialization complete tid: (%d).", curthread->td_tid);

	// This never returns until shutdown
	s_Framework->Update();

	WriteLog(LL_Info, "Shutting down.");

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
	m_InitParams.allocatedBase = p_Params->allocatedBase;

	return true;
}

bool Mira::Framework::SetConfiguration(MiraConfig* p_SourceConfig)
{
	if (p_SourceConfig == nullptr)
		return false;
	
	memcpy(&m_Configuration, p_SourceConfig, sizeof(*p_SourceConfig));

	return true;
}

bool Mira::Framework::Initialize()
{
	// TODO: Load settings
	WriteLog(LL_Warn, "FIXME: loading settings not implemented!!!!");

	// Initialize everything that will survive past reboots and what not, and will not be torn down until Mira ends execution (on shutdown)

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

	// TODO: Install needed hooks for Mira
	WriteLog(LL_Warn, "FIXME: Syscall table hooks not implemented!!!!");

	// Initialize the trainer manager
	m_TrainerManager = new Trainers::TrainerManager();
	if (m_TrainerManager == nullptr)
	{
		WriteLog(LL_Error, "could not initalize the trainer manager.");
		return false;
	}

	// Load the trainer manager
	if (!m_TrainerManager->OnLoad())
	{
		WriteLog(LL_Error, "could not load the trainer manager.");
		return false;
	}

	// Install device driver
	WriteLog(LL_Warn, "Initializing the /dev/mira control driver");
	m_CtrlDriver = new Mira::Driver::CtrlDriver();
	if (m_CtrlDriver == nullptr)
	{
		WriteLog(LL_Error, "could not allocate control driver.");
		return false;
	}

	// Install eventhandler's
	WriteLog(LL_Debug, "Installing event handlers");
	if (!InstallEventHandlers())
		WriteLog(LL_Error, "could not register event handlers");

	// Set the running flag
	m_InitParams.isRunning = true;

	WriteLog(LL_Debug, "initalize thread (%d).", curthread->td_tid);
  
	return true;
}

bool Mira::Framework::Terminate()
{
	// Mira is shutting down for good, kill everything.
	// This should be INVERTED of how it was initialized

	// Check if we already have torn down
	if (!m_InitParams.isRunning)
		return true;
	
	// Update our state
	m_InitParams.isRunning = false;

	if (!RemoveEventHandlers())
		WriteLog(LL_Error, "could not remove event handlers.");

	// Remove the device driver
	if (m_CtrlDriver != nullptr)
	{
		WriteLog(LL_Debug, "deleting device driver.");
		delete m_CtrlDriver;
		m_CtrlDriver = nullptr;
	}

	// Unload the trainer manager
	if (m_TrainerManager)
	{
		WriteLog(LL_Debug, "unloading trainer manager.");
		if (m_TrainerManager->OnUnload())
		{
			delete m_TrainerManager;
			m_TrainerManager = nullptr;
		}
		else
			WriteLog(LL_Error, "could not unload trainer manager.");
	}
	
	// Unload the plugin manager
	if (m_PluginManager)
	{
		WriteLog(LL_Debug, "unloading plugin manager.");
		if (m_PluginManager->OnUnload())
		{
			delete m_PluginManager;
			m_PluginManager = nullptr;
		}
		else
			WriteLog(LL_Error, "could not unload plugin manager");
	}

	// Unload the message manager
	if (m_MessageManager)
	{
		WriteLog(LL_Debug, "Deleting message manager.");
		delete m_MessageManager;
		m_MessageManager = nullptr;
	}

	// Update our running state, to allow the proc to terminate
	m_InitParams.isRunning = false;

	return true;
}

void Mira::Framework::Update()
{
	//tsleep()
	auto _sleep = (int(*)(void *ident, struct lock_object *lock, int priority, const char *wmesg, int timo))kdlsym(_sleep);

	// As long as the main service is running this loop will continue
	while (m_InitParams.isRunning)
	{
		// Check the current state of mira, it should be None if there is nothing to do
		switch (m_State)
		{
			// The case is when the eventhandler for suspension is fired from any thread
		case State::Suspend:
		{
			WriteLog(LL_Debug, "got suspend flag.");

			if (m_PluginManager)
			{
				WriteLog(LL_Debug, "suspending plugin manager.");
				if (!m_PluginManager->OnSuspend())
					WriteLog(LL_Error, "could not suspend plugin manager.");
			}

			// Clear the current state flag, because we have finished suspending
			ClearFlag();

			// We wait for a eventhandler for the resume state change

			// Put this thread to sleep
			tsleep(this, PPAUSE, "mira:update", 0);
			break;
		}
		case State::Resume:
		{
			WriteLog(LL_Debug, "got resume flag.");

			// Clear the resume flag so we can continue running
			ClearFlag();

			// Handle resume events
			auto s_PluginManager = GetFramework()->GetPluginManager();
			if (s_PluginManager)
			{
				WriteLog(LL_Debug, "resuming plugin manager.");
				if (!s_PluginManager->OnResume())
					WriteLog(LL_Error, "could not resume plugin manager.");
			}

			break;
		}
		
		// Do nothing in the case we don't have a change state
		case State::None:
		default:
			break;
		}
	}
}

struct thread* Mira::Framework::GetMainThread()
{
	auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

	auto s_Process = m_InitParams.process;
	if (s_Process == nullptr)
		return nullptr;

	_mtx_lock_flags(&s_Process->p_mtx, 0);
	struct thread* s_Thread = s_Process->p_singlethread;
	if (s_Thread == nullptr)
		s_Thread = FIRST_THREAD_IN_PROC(s_Process);
	_mtx_unlock_flags(&s_Process->p_mtx, 0);

	return s_Thread;
}

struct thread* Mira::Framework::GetSyscoreThread()
{
	auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
	auto s_Process = OrbisOS::Utilities::FindProcessByName("SceSysCore");
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not get SceSysCore process.");
		return nullptr;
	}

	struct thread* s_MainThread = nullptr;

	_mtx_lock_flags(&s_Process->p_mtx, 0);
	s_MainThread = FIRST_THREAD_IN_PROC(s_Process);
	_mtx_unlock_flags(&s_Process->p_mtx, 0);

	return s_MainThread;
}

struct thread* Mira::Framework::GetShellcoreThread()
{
	auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
	struct ::proc* s_Process = OrbisOS::Utilities::FindProcessByName("SceShellCore");
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not get SceShellCore process.");
		return nullptr;
	}

	struct thread* s_MainThread = nullptr;

	_mtx_lock_flags(&s_Process->p_mtx, 0);
	s_MainThread = FIRST_THREAD_IN_PROC(s_Process);
	_mtx_unlock_flags(&s_Process->p_mtx, 0);

	return s_MainThread;
}

bool Mira::Framework::InstallEventHandlers()
{
	#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_550
	auto eventhandler_register = (eventhandler_tag
	(*)(struct eventhandler_list *list, const char *name,
		void *func, void* unk, void *arg, int priority))kdlsym(eventhandler_register);
	#else
		auto eventhandler_register = (eventhandler_tag
	(*)(struct eventhandler_list *list, const char *name,
		void *func, void *arg, int priority))kdlsym(eventhandler_register);
	#endif

	// Register our event handlers
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_550
	m_SuspendTag = eventhandler_register(nullptr, "system_suspend_phase1", reinterpret_cast<void*>(Mira::Framework::OnMiraSuspend), nullptr, nullptr, EVENTHANDLER_PRI_FIRST);
	m_ResumeTag = eventhandler_register(nullptr, "system_resume_phase3", reinterpret_cast<void*>(Mira::Framework::OnMiraResume), nullptr, nullptr, EVENTHANDLER_PRI_LAST);

	m_ProcessExec = eventhandler_register(nullptr, "process_exec", reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExec), nullptr, nullptr, EVENTHANDLER_PRI_ANY);
	m_ProcessExecEnd = eventhandler_register(nullptr, "process_exec_end", reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExecEnd), nullptr, nullptr, EVENTHANDLER_PRI_LAST);
	m_ProcessExit = eventhandler_register(nullptr, "process_exit", reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExit), nullptr, nullptr, EVENTHANDLER_PRI_FIRST);
#else
	m_SuspendTag = EVENTHANDLER_REGISTER(system_suspend_phase1, reinterpret_cast<void*>(Mira::Framework::OnMiraSuspend), nullptr, EVENTHANDLER_PRI_FIRST);
	m_ResumeTag = EVENTHANDLER_REGISTER(system_resume_phase3, reinterpret_cast<void*>(Mira::Framework::OnMiraResume), nullptr, EVENTHANDLER_PRI_LAST);

	m_ProcessExec = EVENTHANDLER_REGISTER(process_exec, reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExec), nullptr, EVENTHANDLER_PRI_ANY);
	m_ProcessExecEnd = EVENTHANDLER_REGISTER(process_exec_end, reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExecEnd), nullptr, EVENTHANDLER_PRI_LAST);
	m_ProcessExit = EVENTHANDLER_REGISTER(process_exit, reinterpret_cast<void*>(Mira::Framework::OnMiraProcessExit), nullptr, EVENTHANDLER_PRI_FIRST);
#endif

	return true;
}

bool Mira::Framework::RemoveEventHandlers()
{
	if (m_SuspendTag == nullptr || m_ResumeTag == nullptr || m_ProcessExec == nullptr || m_ProcessExecEnd == nullptr || m_ProcessExit == nullptr)
		return false;

	auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
	auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

	EVENTHANDLER_DEREGISTER(system_suspend_phase1, m_SuspendTag);
	EVENTHANDLER_DEREGISTER(system_resume_phase3, m_ResumeTag);
	
	EVENTHANDLER_DEREGISTER(process_exec, m_ProcessExec);
	EVENTHANDLER_DEREGISTER(process_exec_end, m_ProcessExecEnd);
	EVENTHANDLER_DEREGISTER(process_exit, m_ProcessExit);

	m_SuspendTag = nullptr;
	m_ResumeTag = nullptr;

	m_ProcessExec = nullptr;
	m_ProcessExecEnd = nullptr;
	m_ProcessExit = nullptr;

	return true;
}

void Mira::Framework::OnMiraSuspend(void* __unused p_Reserved)
{
	// NOTE: Do not assume that this thread is the same thread that Mira is running on
	if (GetFramework() == nullptr)
		return;

	WriteLog(LL_Warn, "SUPSEND SUSPEND SUSPEND (%d).", curthread->td_tid);

	// The Update thread should still be checking for a state change (it should be None currently which means spin)
	// Set the suspend flag and let that thread take over suspending
	GetFramework()->SetSuspendFlag();
}

void Mira::Framework::OnMiraResume(void* __unused p_Reserved)
{
	// NOTE: Do not assume that this thread is the same thread that Mira is running on
	auto wakeup = (void(*)(void* chan))kdlsym(wakeup);

	auto s_Framework = GetFramework();
	if (s_Framework == nullptr)
		return;

	WriteLog(LL_Warn, "RESUME RESUME RESUME (%d).", curthread->td_tid);

	// Change the flag of the framework to resume
	s_Framework->SetResumeFlag();

	// and wake up the Update thread
	WriteLog(LL_Debug, "waking (%p).", s_Framework);
	wakeup(s_Framework);
}

void Mira::Framework::OnMiraShutdown(void* __unused p_Reserved)
{
	// NOTE: Do not assume that this thread is the same thread Mira is running on
	WriteLog(LL_Warn, "SHUTDOWN SHUTDOWN SHUTDOWN on thread (%d).", curthread->td_tid);

	if (GetFramework() == nullptr)
		return;

	if (!GetFramework()->Terminate())
		WriteLog(LL_Error, "could not terminate cleanly");
}

void Mira::Framework::OnMiraProcessExec(void* _unused, struct proc* p_Process)
{
	if (p_Process == nullptr)
		return;
	
	//WriteLog(LL_Warn, "Process Executing: (%d) (%s).", p_Process->p_pid, p_Process->p_comm);	
	
	auto s_Framework = GetFramework();
	if (s_Framework == nullptr)
		return;
	
	auto s_PluginManager = s_Framework->GetPluginManager();
	if (s_PluginManager)
	{
		if (!s_PluginManager->OnProcessExec(p_Process))
			WriteLog(LL_Error, "could not call exit process on plugin manager.");
	}

	if (s_Framework->m_CtrlDriver)
		s_Framework->m_CtrlDriver->OnProcessExec(s_Framework, p_Process);
}

void Mira::Framework::OnMiraProcessExecEnd(void* _unused, struct proc* p_Process)
{
	// Get the framework
	auto s_Framework = GetFramework();
	if (s_Framework == nullptr)
		return;
	
	// Call exec end to the plugin manager
	auto s_PluginManager = s_Framework->GetPluginManager();
	if (s_PluginManager)
	{
		if (!s_PluginManager->OnProcessExecEnd(p_Process))
			WriteLog(LL_Error, "could not call end process on plugin manager.");
	}

	// Call exec end to the trainer manager
	auto s_TrainerManager = s_Framework->m_TrainerManager;
	if (s_TrainerManager)
	{
		if (!s_TrainerManager->OnProcessExecEnd(p_Process))
			WriteLog(LL_Error, "trainer manager on process exec end failed.");
	}
}

void Mira::Framework::OnMiraProcessExit(void* _unused, struct proc* p_Process)
{
	// Get the framework
	auto s_Framework = Mira::Framework::GetFramework();
	if (s_Framework == nullptr)
		return;
	
	// Call plugin manager callback
	auto s_PluginManager = s_Framework->GetPluginManager();
	if (s_PluginManager)
	{
		if (!s_PluginManager->OnProcessExit(p_Process))
			WriteLog(LL_Error, "could not call exit process on plugin manager.");
	}

	// Call trainer manager callback
	auto s_TrainerManager = s_Framework->m_TrainerManager;
	if (s_TrainerManager)
	{
		if (!s_TrainerManager->OnProcessExit(p_Process))
			WriteLog(LL_Error, "trainer manager process exit error");
	}
}