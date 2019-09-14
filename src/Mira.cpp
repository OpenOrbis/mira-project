#include "Mira.hpp"

//
// Boot
//
#include <Boot/InitParams.hpp>

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

///
/// Utilities
///
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Types.hpp>

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

#include <machine/frame.h>
#include <Utils/Hook.hpp>

struct amd64_frame {
	struct amd64_frame      *f_frame;
	long                    f_retaddr;
	long                    f_arg0;
};

static Mira::Utils::Hook* m_Hook = nullptr;

/*static uint8_t debugger_isStackSpace(uint64_t address)
{
	return ((address & 0xFFFFFFFF00000000) == 0xFFFFFF8000000000);
}*/

void sdtossd(struct user_segment_descriptor *sd, struct soft_segment_descriptor *ssd)
{

	ssd->ssd_base  = (sd->sd_hibase << 24) | sd->sd_lobase;
	ssd->ssd_limit = (sd->sd_hilimit << 16) | sd->sd_lolimit;
	ssd->ssd_type  = sd->sd_type;
	ssd->ssd_dpl   = sd->sd_dpl;
	ssd->ssd_p     = sd->sd_p;
	ssd->ssd_long  = sd->sd_long;
	ssd->ssd_def32 = sd->sd_def32;
	ssd->ssd_gran  = sd->sd_gran;
}

static void debugger_onTrapFatal(struct trapframe* frame, vm_offset_t eva)
{
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto gdt = (struct user_segment_descriptor*)kdlsym(gdt);/* global descriptor tables */

#define MAX_TRAP_MSG		33
static const char *trap_msg[] = {
	"",					/*  0 unused */
	"privileged instruction fault",		/*  1 T_PRIVINFLT */
	"",					/*  2 unused */
	"breakpoint instruction fault",		/*  3 T_BPTFLT */
	"",					/*  4 unused */
	"",					/*  5 unused */
	"arithmetic trap",			/*  6 T_ARITHTRAP */
	"",					/*  7 unused */
	"",					/*  8 unused */
	"general protection fault",		/*  9 T_PROTFLT */
	"trace trap",				/* 10 T_TRCTRAP */
	"",					/* 11 unused */
	"page fault",				/* 12 T_PAGEFLT */
	"",					/* 13 unused */
	"alignment fault",			/* 14 T_ALIGNFLT */
	"",					/* 15 unused */
	"",					/* 16 unused */
	"",					/* 17 unused */
	"integer divide fault",			/* 18 T_DIVIDE */
	"non-maskable interrupt trap",		/* 19 T_NMI */
	"overflow trap",			/* 20 T_OFLOW */
	"FPU bounds check fault",		/* 21 T_BOUND */
	"FPU device not available",		/* 22 T_DNA */
	"double fault",				/* 23 T_DOUBLEFLT */
	"FPU operand fetch fault",		/* 24 T_FPOPFLT */
	"invalid TSS fault",			/* 25 T_TSSFLT */
	"segment not present fault",		/* 26 T_SEGNPFLT */
	"stack fault",				/* 27 T_STKFLT */
	"machine check trap",			/* 28 T_MCHK */
	"SIMD floating-point exception",	/* 29 T_XMMFLT */
	"reserved (unknown) fault",		/* 30 T_RESERVED */
	"",					/* 31 unused (reserved) */
	"DTrace pid return trap",		/* 32 T_DTRACE_RET */
	"DTrace fasttrap probe trap",		/* 33 T_DTRACE_PROBE */
};

	int code, ss;
	u_int type;
	long esp;
	struct soft_segment_descriptor softseg;
	const char *msg;

	code = frame->tf_err;
	type = frame->tf_trapno;
	sdtossd(&gdt[NGDT * PCPU_GET(cpuid) + IDXSEL(frame->tf_cs & 0xffff)],
	    &softseg);

	if (type <= MAX_TRAP_MSG)
		msg = trap_msg[type];
	else
		msg = "UNKNOWN";
	printf("\n\nFatal trap %d: %s while in %s mode\n", type, msg,
	    ISPL(frame->tf_cs) == SEL_UPL ? "user" : "kernel");
#ifdef SMP
	/* two separate prints in case of a trap on an unmapped page */
	printf("cpuid = %d; ", PCPU_GET(cpuid));
	printf("apic id = %02x\n", PCPU_GET(apic_id));
#endif
	if (type == T_PAGEFLT) {
		printf("fault virtual address	= 0x%lx\n", eva);
		printf("fault code		= %s %s %s, %s\n",
			code & PGEX_U ? "user" : "supervisor",
			code & PGEX_W ? "write" : "read",
			code & PGEX_I ? "instruction" : "data",
			code & PGEX_P ? "protection violation" : "page not present");
	}
	printf("instruction pointer	= 0x%lx:0x%lx\n",
	       frame->tf_cs & 0xffff, frame->tf_rip);
        if (ISPL(frame->tf_cs) == SEL_UPL) {
		ss = frame->tf_ss & 0xffff;
		esp = frame->tf_rsp;
	} else {
		ss = GSEL(GDATA_SEL, SEL_KPL);
		esp = (long)&frame->tf_rsp;
	}
	printf("stack pointer	        = 0x%x:0x%lx\n", ss, esp);
	printf("frame pointer	        = 0x%x:0x%lx\n", ss, frame->tf_rbp);
	printf("code segment		= base 0x%lx, limit 0x%lx, type 0x%x\n",
	       softseg.ssd_base, softseg.ssd_limit, softseg.ssd_type);
	printf("			= DPL %d, pres %d, long %d, def32 %d, gran %d\n",
	       softseg.ssd_dpl, softseg.ssd_p, softseg.ssd_long, softseg.ssd_def32,
	       softseg.ssd_gran);
	printf("processor eflags	= ");
	if (frame->tf_rflags & PSL_T)
		printf("trace trap, ");
	if (frame->tf_rflags & PSL_I)
		printf("interrupt enabled, ");
	if (frame->tf_rflags & PSL_NT)
		printf("nested task, ");
	if (frame->tf_rflags & PSL_RF)
		printf("resume, ");
	printf("IOPL = %ld\n", (frame->tf_rflags & PSL_IOPL) >> 12);
	printf("current process		= ");
	if (curproc) {
		printf("%lu (%s)\n",
		    (u_long)curproc->p_pid, curthread->td_name/* ?
		    curthread->td_name : ""*/);
	} else {
		printf("Idle\n");
	}

#ifdef KDB
	if (debugger_on_panic || kdb_active)
		if (kdb_trap(type, 0, frame))
			return;
#endif
	printf("trap number		= %d\n", type);
	if (type <= MAX_TRAP_MSG)
		printf("%s\n", trap_msg[type]);
	else
		printf("unknown/reserved trap");
	
	// Intentionally hang the thread
	for (;;)
		__asm__("nop");

	// Allow the debugger to be placed here manually and continue exceution
	__asm__("pop %rbp;leave;ret;");
}

extern "C" void mira_entry(void* args)
{
	// Fill the kernel base address
	gKernelBase = (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall;

    //auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
	auto kproc_exit = (void(*)(int ecode))kdlsym(kproc_exit);
	auto vmspace_alloc = (struct vmspace* (*)(vm_offset_t min, vm_offset_t max))kdlsym(vmspace_alloc);
	auto pmap_activate = (void(*)(struct thread *td))kdlsym(pmap_activate);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto avcontrol_sleep = (void(*)(int milliseconds))kdlsym(avcontrol_sleep);

    // Let'em know we made it
	printf("[+] mira has reached stage 2\n");

    // These are the initialization parameters from the loader
    Mira::Boot::InitParams* initParams = static_cast<Mira::Boot::InitParams*>(args);
    if (initParams == nullptr)
    {
		printf("[-] no init params\n");
		kproc_exit(0);
		return;
	}    

	printf("[+] starting logging\n");
	m_Hook = new Mira::Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(debugger_onTrapFatal));
	m_Hook->Enable();

	printf("[+] %s %d: here\n", __PRETTY_FUNCTION__, __LINE__);

	auto s_Logger = Mira::Utils::Logger::GetInstance();
	printf("[+] %s %d: here\n", __PRETTY_FUNCTION__, __LINE__);
	if (!s_Logger)
	{
		printf("[-] could not allocate logger\n");
		kproc_exit(0);
		return;
	}

	printf("[+] %s %d: here\n", __PRETTY_FUNCTION__, __LINE__);

	// Create new credentials
	(void)ksetuid_t(0, curthread);

	WriteLog(LL_Debug, "here");

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
	}
	WriteLog(LL_Debug, "here");

	// Create new vm_space
	WriteLog(LL_Debug, "Creating new vm space");

	WriteLog(LL_Debug, "fuck the ps4 it gives you aids and cancer at the same fucking time");
	WriteLog(LL_Debug, "payloadBase: %p", initParams->payloadBase);
	WriteLog(LL_Debug, "allocatedData: %p", initParams->allocatedBase);
	WriteLog(LL_Debug, "entryPoint: %p", initParams->entrypoint);
	WriteLog(LL_Debug, "kernelBase: %p", gKernelBase);
	WriteLog(LL_Debug, "kproc_exit: %p", kproc_exit);
	WriteLog(LL_Debug, "vmspace_alloc: %p", vmspace_alloc);
	WriteLog(LL_Debug, "pmap_activate: %p", pmap_activate);
	WriteLog(LL_Debug, "printf: %p", printf);
	WriteLog(LL_Debug, "avcontrol_sleep: %p", avcontrol_sleep);

	
	struct vmspace* vmspace = vmspace_alloc(0, PAGE_SIZE);
	WriteLog(LL_Debug, "here");
	if (!vmspace)
	{
		WriteLog(LL_Error, "vmspace_alloc failed\n");
		kproc_exit(0);
		return;
	}

	// Wait for the process to be filled out
	const auto s_MaxTimeout = 3;
	auto s_CurrentTimeout = 0;
	while (initParams->process == nullptr)
	{
		WriteLog(LL_Error, "waiting for process waiting 1s");
		avcontrol_sleep(1000);
		s_CurrentTimeout++;

		if (s_CurrentTimeout < s_MaxTimeout)
			continue;
		
		WriteLog(LL_Error, "error waiting for process");
		kproc_exit(0);
		return;
	}

	WriteLog(LL_Debug, "here");

	// Assign our new vmspace to our process
	initParams->process->p_vmspace = vmspace;
	if (initParams->process == curthread->td_proc)
	{
		WriteLog(LL_Debug, "here");
		pmap_activate(curthread);
		WriteLog(LL_Debug, "here");
	}

	//auto offset = offsetof(struct thread, td_retval[0]);

	/*auto offset = offsetof(struct thread, td_retval[0]);
	auto offset2 = offsetof(struct thread, unk35C);
	auto offset3 = offsetof(struct thread, td_flags);
	auto offset4 = offsetof(struct thread, td_ucred);
	auto offset5 = offsetof(struct thread, td_intrval);
	auto offset6 = offsetof(struct thread, td_name);
	auto offset7 = offsetof(struct thread, td_name);
	WriteLog(LL_Debug, "td_retval %p", offset);*/

	// Because we have now forked into a new realm of fuckery
	// We need to reserve the first 3 file descriptors in our process
	int descriptor = kopen(const_cast<char*>("/dev/console"), 1, 0);
	WriteLog(LL_Debug, "/dev/console descriptor: %d", descriptor);
	WriteLog(LL_Info, "dup2(desc, 1) result: %d", kdup2(descriptor, 1));
	WriteLog(LL_Info, "dup2(1, 2) result: %d", kdup2(1, 2));

	// Show over UART that we are running in a new process
	WriteLog(LL_Info, "oni_kernelInitialization in new process!\n");

	// Initialize miraframework
	auto s_Framework = Mira::Framework::GetFramework();

	WriteLog(LL_Debug, "here");

	// Set the initparams so we do not lose track of it
	s_Framework->SetInitParams(initParams);

	WriteLog(LL_Debug, "here");

	// We are successfully running
	s_Framework->GetInitParams()->isRunning = false;

	WriteLog(LL_Debug, "here");

	if (!s_Framework->Initialize())
	{
		WriteLog(LL_Error, "could not initialize mira framework.");
		WriteLog(LL_Debug, "MiraTerminate: %s", s_Framework->Terminate() ? "Success" : "Failure");
		kproc_exit(0);
		return;
	}

	// At this point we don't need kernel context anymore
	WriteLog(LL_Info, "Mira initialization complete");

	// This keeps the process alive
	while (s_Framework->GetInitParams()->isRunning)
		__asm__("nop");
	
	// Write our final goodbyes
	WriteLog(LL_Debug, "Mira kernel process is terminating");
	kproc_exit(0);
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
	WriteLog(LL_Info, "loading settings from (%s)", "MIRA_CONFIG_PATH");

	// TODO: Initialize message manager
	m_MessageManager = new Mira::Messaging::MessageManager();
	if (m_MessageManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate message manager.");
		return false;
	}

	WriteLog(LL_Debug, "here");

	// Initialize plugin manager
	m_PluginManager = new Mira::Plugins::PluginManager();
	if (m_PluginManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate plugin manager.");
		return false;
	}

	WriteLog(LL_Debug, "here");

	// Load the plugin manager
	if (!m_PluginManager->OnLoad())
	{
		WriteLog(LL_Error, "could not initialize default plugins");
		return false;
	}

	WriteLog(LL_Debug, "here");

	// Install eventhandler's
	if (!InstallEventHandlers())
		WriteLog(LL_Error, "could not register event handlers");

	WriteLog(LL_Debug, "here");

	// Initialize the rpc server
	m_RpcServer = new Mira::Messaging::Rpc::Server();
	if (m_RpcServer == nullptr)
	{
		WriteLog(LL_Error, "could not allocate rpc server.");
		return false;
	}
	if (!m_RpcServer->OnLoad())
	{
		WriteLog(LL_Error, "could not load rpc server.");
		return false;
	}

	// TODO: Install needed hooks for Mira

	// Install device driver
	m_CtrlDriver = new Mira::Driver::CtrlDriver();
	if (m_CtrlDriver == nullptr)
	{
		WriteLog(LL_Error, "could not allocate control driver.");
		return false;
	}

	WriteLog(LL_Info, "mira initialized successfully!");

	return true;
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
	m_SuspendTag = EVENTHANDLER_REGISTER(system_suspend_phase0, reinterpret_cast<void*>(Mira::Framework::OnMiraSuspend), GetFramework(), EVENTHANDLER_PRI_FIRST);
	m_ResumeTag = EVENTHANDLER_REGISTER(system_resume_phase2, reinterpret_cast<void*>(Mira::Framework::OnMiraResume), GetFramework(), EVENTHANDLER_PRI_LAST);
	m_ShutdownTag = EVENTHANDLER_REGISTER(shutdown_pre_sync, reinterpret_cast<void*>(Mira::Framework::OnMiraShutdown), GetFramework(), EVENTHANDLER_PRI_LAST);

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

	// TODO: Kdlsym
	auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
	auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

	EVENTHANDLER_DEREGISTER(power_suspend, m_SuspendTag);
	EVENTHANDLER_DEREGISTER(power_resume, m_ResumeTag);
	EVENTHANDLER_DEREGISTER(shutdown_pre_sync, m_ShutdownTag);

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
	WriteLog(LL_Warn, "SHUTDOWN SHUTDOWN SHUTDOWN");

	if (GetFramework() == nullptr)
		return;
	
	if (!GetFramework()->Terminate())
	{
		WriteLog(LL_Error, "could not terminate cleanly");
		return;
	}
}