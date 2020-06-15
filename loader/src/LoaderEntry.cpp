// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

extern "C"
{
	#include <Utils/_Syscall.hpp>
};

#include <Utils/New.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Boot/Patches.hpp>
#include <Boot/InitParams.hpp>

#include <Utils/Dynlib.hpp>
#include <Utils/Logger.hpp>

#include <sys/elf64.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/unistd.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/imgact.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_param.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <netinet/in.h>

#include <Boot/MiraLoader.hpp>

using namespace Mira::Utils;
using namespace MiraLoader;

#define ALLOC_3MB	0x300000
#define ALLOC_5MB	0x500000

uint8_t* gKernelBase = nullptr;

struct kexec_uap
{
	void* func;
	void* arg0;
};

int(*sceNetSocket)(const char *, int, int, int) = nullptr;
int(*sceNetSocketClose)(int) = nullptr;
int(*sceNetBind)(int, struct sockaddr *, int) = nullptr;
int(*sceNetListen)(int, int) = nullptr;
int(*sceNetAccept)(int, struct sockaddr *, unsigned int *) = nullptr;
int(*sceNetRecv)(int, void *, size_t, int) = nullptr;

int(*snprintf)(char *str, size_t size, const char *format, ...) = nullptr;

void miraloader_kernelInitialization(struct thread* td, struct kexec_uap* uap);


void WriteNotificationLog(const char* text)
{
	if (!text)
		return;

	// Load the sysutil module, needs to be rooted for this to work
	int32_t moduleId = -1;
	Dynlib::LoadPrx("/system/common/lib/libSceSysUtil.sprx", &moduleId);

	// Validate that the module loaded properly
	if (moduleId == -1)
		return;

	int(*sceSysUtilSendSystemNotificationWithText)(int messageType, const char* message) = NULL;

	// Resolve the symbol
	Dynlib::Dlsym(moduleId, "sceSysUtilSendSystemNotificationWithText", &sceSysUtilSendSystemNotificationWithText);

	if (sceSysUtilSendSystemNotificationWithText)
		sceSysUtilSendSystemNotificationWithText(222, text);

	Dynlib::UnloadPrx(moduleId);
}

void mira_escape(struct thread* td, void* uap)
{
	gKernelBase = (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall;
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);

	printf("[+] mira_escape\n");

	struct ucred* cred = td->td_proc->p_ucred;
	struct filedesc* fd = td->td_proc->p_fd;

	cred->cr_uid = 0;
	cred->cr_ruid = 0;
	cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;

	cred->cr_prison = *(struct prison**)kdlsym(prison0);
	fd->fd_rdir = fd->fd_jdir = *(struct vnode**)kdlsym(rootvnode);

	// set diag auth ID flags
	td->td_ucred->cr_sceAuthID = SceAuthenticationId_t::Decid;

	// make system credentials
	td->td_ucred->cr_sceCaps[0] = SceCapabilities_t::Max;
	td->td_ucred->cr_sceCaps[1] = SceCapabilities_t::Max;

	// Apply patches
	cpu_disable_wp();

	Mira::Boot::Patches::install_prePatches();

	cpu_enable_wp();

	printf("[-] mira_escape\n");
}


extern "C" void* mira_entry(void* args)
{
	// Escape the jail and sandbox
	syscall2(KEXEC_SYSCALL_NUM, reinterpret_cast<void*>(mira_escape), NULL);

	//int32_t sysUtilModuleId = -1;
	int32_t netModuleId = -1;
	int32_t libcModuleId = -1;
	//int32_t libKernelWebModuleId = -1;

	{
		Dynlib::LoadPrx("libSceLibcInternal.sprx", &libcModuleId);

		Dynlib::Dlsym(libcModuleId, "snprintf", &snprintf);

	}

	// Networking resolving
	{
		Dynlib::LoadPrx("libSceNet.sprx", &netModuleId);

		Dynlib::Dlsym(netModuleId, "sceNetSocket", &sceNetSocket);
		Dynlib::Dlsym(netModuleId, "sceNetSocketClose", &sceNetSocketClose);
		Dynlib::Dlsym(netModuleId, "sceNetBind", &sceNetBind);
		Dynlib::Dlsym(netModuleId, "sceNetListen", &sceNetListen);
		Dynlib::Dlsym(netModuleId, "sceNetAccept", &sceNetAccept);
		Dynlib::Dlsym(netModuleId, "sceNetRecv", &sceNetRecv);
	}

	// Allocate a 3MB buffer
	size_t bufferSize = ALLOC_5MB;
	uint8_t* buffer = (uint8_t*)_mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (!buffer)
	{
		WriteNotificationLog("could not allocate 5MB buffer");
		return nullptr;
	}
	memset(buffer, 0, bufferSize);
	//Loader::Memset(buffer, 0, bufferSize);
	// Hold our server socket address
	struct sockaddr_in serverAddress = { 0 };
	//Loader::Memset(&serverAddress, 0, sizeof(serverAddress));

	// Listen on port 9021
	serverAddress.sin_len = sizeof(serverAddress);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = __bswap16(9021); // port 9021
	serverAddress.sin_family = AF_INET;

	// Create a new socket
	int32_t serverSocket = sceNetSocket("miraldr", AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0)
	{
		WriteNotificationLog("socket error");
		return nullptr;
	}

	// Bind to localhost
	int32_t result = sceNetBind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (result < 0)
	{
		WriteNotificationLog("bind error");
		return nullptr;
	}

	// Listen
	result = sceNetListen(serverSocket, 10);
	if (result < 0)
	{
		WriteNotificationLog("listen error");
		return nullptr;
	}

	WriteNotificationLog("waiting for clients");

	// Wait for a client to send something
	int32_t clientSocket = sceNetAccept(serverSocket, nullptr, nullptr);
	if (clientSocket < 0)
	{
		WriteNotificationLog("accept errror");
		return nullptr;
	}

	int32_t currentSize = 0;
	int32_t recvSize = 0;

	// Recv one byte at a time until we get our buffer
	while ((recvSize = sceNetRecv(clientSocket, buffer + currentSize, bufferSize - currentSize, 0)) > 0)
		currentSize += recvSize;

	// Close the client and server socket connections
	sceNetSocketClose(clientSocket);
	sceNetSocketClose(serverSocket);

	// Determine if we launch a elf or a payload
	if (buffer[0] == ELFMAG0 &&
		buffer[1] == ELFMAG1 &&
		buffer[2] == ELFMAG2 &&
		buffer[3] == ELFMAG3) // 0x7F 'ELF'
	{


		// TODO: Check for custom Mira elf section to determine launch type
		char buf[64];
		memset(buf, 0, sizeof(buf));

		snprintf(buf, sizeof(buf), "elf: %p elfSize: %llx", buffer, currentSize);
		WriteNotificationLog(buf);

		uint8_t isLaunchingKernel = true;
		if (isLaunchingKernel)
		{
			Mira::Boot::InitParams initParams = { 0 };
			initParams.isElf = true;
			initParams.isRunning = false;
			initParams.payloadBase = (uint64_t)buffer;
			initParams.payloadSize = currentSize;
			initParams.process = nullptr;
			initParams.elfLoader = nullptr;
			initParams.entrypoint = nullptr;

			syscall2(KEXEC_SYSCALL_NUM, reinterpret_cast<void*>(miraloader_kernelInitialization), reinterpret_cast<void*>(&initParams));
		}
		else
		{
			// Launch ELF
			// TODO: Check/Add a flag to the elf that determines if this is a kernel or userland elf
			// Loader(const void* p_Elf, uint32_t p_ElfSize, ElfLoaderType_t p_Type);
			MiraLoader::Loader loader(buffer, currentSize, ElfLoaderType_t::UserProc);

			auto entryPoint = reinterpret_cast<void(*)(void*)>(loader.GetEntrypoint());
			if (!entryPoint)
			{
				WriteNotificationLog("could not find entry point");
				return NULL;
			}

			// Launch userland
			entryPoint(nullptr);
		}
	}
	else
	{
		// Launch Userland Payload
		WriteNotificationLog("launching payload");

		void(*payload_start)() = (void(*)())buffer;
		payload_start();
	}

	return nullptr;
}

void miraloader_kernelInitialization(struct thread* td, struct kexec_uap* uap)
{
	// If we do not have a valid parameter passed, kick back
	if (!uap || !uap->arg0)
		return;

	Mira::Boot::InitParams* userInitParams = reinterpret_cast<Mira::Boot::InitParams*>(uap->arg0);

	// Thread should already be escaped from earlier

	// Fill the kernel base address
	gKernelBase = (uint8_t*)kernelRdmsr(0xC0000082) - kdlsym_addr_Xfast_syscall;
	//void(*critical_enter)(void) = kdlsym(critical_enter);
	//void(*crtical_exit)(void) = kdlsym(critical_exit);
	auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
	auto kmem_free = (void(*)(void* map, void* addr, size_t size))kdlsym(kmem_free);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto kproc_create = (int(*)(void(*func)(void*), void* arg, struct proc** newpp, int flags, int pages, const char* fmt, ...))kdlsym(kproc_create);
	vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	//auto memset = (void* (*)(void *s, int c, size_t n))kdlsym(memset);
	auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
	auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

	// Allocate a new logger for the MiraLoader
	auto s_Logger = Mira::Utils::Logger::GetInstance();
	if (!s_Logger)
	{
		printf("[-] could not allocate logger\n");
		kthread_exit();
		return;
	}
	printf("logger created\n");

	// Create launch parameters, this is floating in "free kernel space" so the other process should
	// be able to grab and use the pointer directly
	Mira::Boot::InitParams*  initParams = (Mira::Boot::InitParams*)kmem_alloc(map, sizeof(Mira::Boot::InitParams));
	if (!initParams)
	{
		WriteLog(LL_Error, "could not allocate initialization parameters.\n");
		return;
	}
	memset(initParams, 0, sizeof(*initParams));

	// Copyin our new arguments from userland
	int copyResult = copyin(userInitParams, initParams, sizeof(*initParams));
	if (copyResult != 0)
	{
		kmem_free(map, initParams, sizeof(*initParams));
		WriteLog(LL_Error, "could not copyin initalization parameters (%d)\n", copyResult);
		return;
	}

	// initparams are read from the uap in this syscall func
	uint64_t payloadSize = initParams->payloadSize;
	uint64_t payloadBase = initParams->payloadBase;

	// Allocate some memory
	uint8_t* kernelElf = (uint8_t*)kmem_alloc(map, payloadSize);
	if (!kernelElf)
	{
		// Free the previously allocated initialization parameters
		kmem_free(map, initParams, sizeof(*initParams));
		WriteLog(LL_Error, "could not allocate kernel payload.\n");
		return;
	}
	memset(kernelElf, 0, payloadSize);
	WriteLog(LL_Debug, "payloadBase: %p payloadSize: %llx kernelElf: %p\n", payloadBase, payloadSize, kernelElf);

	// Copy the ELF data from userland
	copyResult = copyin((const void*)payloadBase, kernelElf, payloadSize);
	if (copyResult != 0)
	{
		// Intentionally blow the fuck up
		WriteLog(LL_Error, "fuck, this is bad...\n");
		for (;;)
			__asm__("nop");
	}

	WriteLog(LL_Debug, "finished allocating and copying ELF from userland");

	// Determine if we launch a elf or a payload
	uint32_t magic = *(uint32_t*)kernelElf;
	if (magic != 0x464C457F)
	{
		printf("invalid elf header.\n");
		return;
	}
	WriteLog(LL_Debug, "elf header: %X\n", magic);

	// Launch ELF
	MiraLoader::Loader* loader = new MiraLoader::Loader(kernelElf, payloadSize, ElfLoaderType_t::KernelProc); //malloc(sizeof(ElfLoader_t), M_LINKER, M_WAITOK);
	if (!loader)
	{
		printf("could not allocate loader\n");
		return;
	}

	// Update the loader
	initParams->elfLoader = loader;
	initParams->entrypoint = reinterpret_cast<void(*)(void*)>(loader->GetEntrypoint());
	initParams->allocatedBase = reinterpret_cast<uint64_t>(loader->GetAllocatedMap());
	initParams->payloadBase = reinterpret_cast<uint64_t>(kernelElf);
	initParams->payloadSize = payloadSize;

	// Update the initial running state
	initParams->isRunning = false;

	auto s_EntryPoint = initParams->entrypoint;
	if (s_EntryPoint != nullptr)
	{
		printf("[+]entrypoint: %p", s_EntryPoint);
		(void)kproc_create(s_EntryPoint, initParams, &initParams->process, 0, 200, "miraldr2"); // 8MiB stack
	}
	else
	{
		printf("[-]could not get entry point.\n");
	}		
}