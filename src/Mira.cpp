#include "Mira.hpp"

//
// Boot
//
#include <Boot/InitParams.hpp>

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
#include <sys/eventhandler.h>
#include <sys/sysent.h>					// sysent_t
#include <sys/proc.h>					// proc
#include <sys/filedesc.h>				// filedesc

const char* gNull = "(null)";
uint8_t* gKernelBase = nullptr;
Mira::Boot::InitParams* gInitParams = nullptr;
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

extern "C" void mira_entry(void* args)
{
    if (args == nullptr)
        return;
    
    // These are the initialization parameters from the loader
    Mira::Boot::InitParams* initParams = static_cast<Mira::Boot::InitParams*>(args);
    if (initParams == nullptr)
        return;

    // Fill the kernel base address
	gKernelBase = (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall;
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);
	auto vmspace_alloc = (struct vmspace* (*)(vm_offset_t min, vm_offset_t max))kdlsym(vmspace_alloc);
	auto pmap_activate = (void(*)(struct thread *td))kdlsym(pmap_activate);
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);

    // Let'em know we made it
	printf(reinterpret_cast<const char*>("[+] mira has reached stage 2\n"));

	printf("[+] starting logging\n");
	auto s_Logger = Mira::Utils::Logger::GetInstance();
	if (!s_Logger)
	{
		printf("[-] could not allocate logger\n");
		kthread_exit();
		return;
	}

	// Create new vm_space
	WriteLog(LL_Debug, "Creating new vm space");

	vm_offset_t sv_minuser = MAX(sv->sv_minuser, PAGE_SIZE);
	struct vmspace* vmspace = vmspace_alloc(sv_minuser, sv->sv_maxuser);
	if (!vmspace)
	{
		WriteLog(LL_Error, "vmspace_alloc failed\n");
		kthread_exit();
		return;
	}

	// Assign our new vmspace to our process
	initParams->process->p_vmspace = vmspace;
	if (initParams->process == curthread->td_proc)
		pmap_activate(curthread);

	// Create new credentials
	(void)ksetuid_t(0, curthread);

	// Root and escape our thread
	//oni_threadEscape(curthread, NULL);

	// Because we have now forked into a new realm of fuckery
	// We need to reserve the first 3 file descriptors in our process
	int descriptor = kopen(const_cast<char*>("/dev/console"), 1, 0);
	kdup2(descriptor, 1);
	kdup2(1, 2);

	// Show over UART that we are running in a new process
	WriteLog(LL_Info, "oni_kernelInitialization in new process!\n");

	// Initialize miraframework
	auto s_Framework = Mira::Framework::GetFramework();

	// Set the initparams so we do not lose track of it
	s_Framework->SetInitParams(*initParams);

	if (!s_Framework->Initialize())
	{
		WriteLog(LL_Error, "could not initialize mira framework.");
		kthread_exit();
		return;
	}

	// At this point we don't need kernel context anymore
	WriteLog(LL_Info, "Mira initialization complete");
	kthread_exit();
}

bool Mira::Framework::SetInitParams(Mira::Boot::InitParams& p_Params)
{
	m_InitParams = p_Params;
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

	// Initialize plugin manager
	m_PluginManager = new Mira::Plugins::PluginManager();
	if (m_PluginManager == nullptr)
	{
		WriteLog(LL_Error, "could not allocate plugin manager.");
		return false;
	}

	// Load the plugin manager
	if (!m_PluginManager->OnLoad())
	{
		WriteLog(LL_Error, "could not initialize default plugins");
		return false;
	}

	// Install eventhandler's
	if (!InstallEventHandlers())
		WriteLog(LL_Error, "could not register event handlers");

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
	m_SuspendTag = EVENTHANDLER_REGISTER(system_suspend_phase0, reinterpret_cast<void*>(Mira::Framework::OnMiraSuspend), this, EVENTHANDLER_PRI_FIRST);
	m_ResumeTag = EVENTHANDLER_REGISTER(system_resume_phase2, reinterpret_cast<void*>(Mira::Framework::OnMiraResume), this, EVENTHANDLER_PRI_LAST);
	m_ShutdownTag = EVENTHANDLER_REGISTER(shutdown_pre_sync, reinterpret_cast<void*>(Mira::Framework::OnMiraShutdown), this, EVENTHANDLER_PRI_LAST);

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

void Mira::Framework::OnMiraSuspend(Mira::Framework* p_Framework)
{
	WriteLog(LL_Warn, "SUPSEND SUSPEND SUSPEND");

	if (p_Framework == nullptr)
		return;
	
	// Handle suspend events
	auto s_PluginManager = p_Framework->m_PluginManager;
	if (s_PluginManager)
		s_PluginManager->OnSuspend();
}

void Mira::Framework::OnMiraResume(Mira::Framework* p_Framework)
{
	WriteLog(LL_Warn, "RESUME RESUME RESUME");

	if (p_Framework == nullptr)
		return;

	// Handle resume events
	auto s_PluginManager = p_Framework->m_PluginManager;
	if (s_PluginManager)
		s_PluginManager->OnResume();
}

void Mira::Framework::OnMiraShutdown(Mira::Framework* p_Framework)
{
	WriteLog(LL_Warn, "SHUTDOWN SHUTDOWN SHUTDOWN");

	if (p_Framework == nullptr)
		return;
	
	auto s_PluignManager = p_Framework->m_PluginManager;
	if (s_PluignManager)
		s_PluignManager->OnUnload();
}