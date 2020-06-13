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
#include <Utils/_Syscall.hpp>
#include <MD5/md5.h>
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


typedef int FILE;

void *(*fmemcpy)(void *destination, const void *source, size_t num);
FILE *(*fopen)(const char *filename, const char *mode) = nullptr;
size_t (*fread)(void *ptr, size_t size, size_t count, FILE *stream) = nullptr;
size_t (*fwrite)(const void * ptr, size_t size, size_t count, FILE *stream ) = nullptr;
int (*fseek)(FILE *stream, long int offset, int origin) = nullptr;
long int(*ftell)(FILE *stream) = nullptr;
int (*fclose)(FILE *stream) = nullptr;
void *(*fuck_malloc)(size_t size) = nullptr;
void (*fuck_free)(void *ptr) = nullptr;

int(*snprintf)(char *str, size_t size, const char *format, ...) = nullptr;

void miraloader_kernelInitialization(struct thread* td, struct kexec_uap* uap);

int (*sceKernelOpen)(const char *path, int flags, int mode)= nullptr;
ssize_t (*sceKernelRead)(int fd, void *buf, size_t nbyte)= nullptr;
int (*sceKernelLseek)(int fd, off_t offset, int whence)= nullptr;
int (*sceKernelClose)(int fd)= nullptr;
ssize_t (*sceKernelWrite)(int d, const void *buf, size_t nbytes)= nullptr;
int (*printf)(const char *_Restrict, ...);


char* usbpath()
{
	int usb;
	static char usbbuf[100];
	usbbuf[0] = '\0';
	for (int x = 0; x <= 7; x++)
	{
		snprintf(usbbuf, 100, "/mnt/usb%i/.dirtest", x);
		usb = sceKernelOpen(usbbuf, 0x0001 | 0x0200 | 0x0400, 0777);
		if (usb != -1)
		{
			sceKernelClose(usb);
			
			snprintf(usbbuf, 100, "/mnt/usb%i", x);

			return usbbuf;

		}
	}

	return NULL;
}


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

#define DIFFERENT_HASH 1
#define SAME_HASH 0

int MD5_hash_compare(const char *usbfile)
{
	 unsigned char c[16];
	 unsigned char c2[16];
    int i;
    FILE *usb = fopen (usbfile, "rb");
    FILE *hdd = fopen ("/user/MiraLoader.elf", "rb");
    MD5_CTX mdContext;

    MD5_CTX mdContext2;
    int bytes2 = 0;
    unsigned char data2[1024];

    int bytes = 0;
    unsigned char data[1024];

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, usb)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
   


    MD5_Init (&mdContext2);
    while ((bytes2 = fread (data2, 1, 1024, hdd)) != 0)
        MD5_Update (&mdContext2, data2, bytes2);
    MD5_Final (c2,&mdContext2);


  printf("MD5 HASH OF USB ELF IS %i%i%i%i%i%i%i%i%i%i%i%i%i%i%i\n", c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
  printf("MD5 HASH OF HDD ELF IS %i%i%i%i%i%i%i%i%i%i%i%i%i%i%i\n", c2[1], c2[2], c2[3], c2[4], c2[5], c2[6], c2[7], c2[8], c2[9], c2[10], c2[11], c2[12], c2[13], c2[14], c2[15]);


    for(i = 0; i < 16; i++) 
    {
    	printf("c[%i] = %i\n", i, c[i]);
    	printf("c2[%i] = %i\n", i, c2[i]);
         if(c[i] != c2[i])
         {
         	return DIFFERENT_HASH;
         }


    }

    fclose (usb);
    fclose (hdd);

return SAME_HASH;
}

     int ftruncate(int fd, off_t length)
     {
     	return syscall(480, fd, length);
     }


    int munmap(void *addr,	size_t len)
    {
    	return syscall(73, addr, len);
    }

int copyFile(char *sourcefile)
{

    int sfd, dfd;
    char *src, *dest;
    size_t filesize;

    /* SOURCE */
    sfd = sceKernelOpen(sourcefile, O_RDONLY, 0x000);
    filesize = sceKernelLseek(sfd, 0, SEEK_END);

    if(filesize < 0) return -1;

    src = _mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, sfd, 0);

    /* DESTINATION */
    dfd = sceKernelOpen("/user/MiraLoader.elf", O_RDWR | O_CREAT | O_TRUNC, 0666);

    ftruncate(dfd, filesize);

    dest = _mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, 0);

    /* COPY */

    fmemcpy(dest, src, filesize);


    munmap(src, filesize);
    munmap(dest, filesize);

    sceKernelClose(sfd);
    sceKernelClose(dfd);


    return 0;
}
//libkernel_web.sprx
//libkernel_sys.sprx

extern "C" void* mira_entry(void* args)
{
	// Escape the jail and sandbox
	syscall2(KEXEC_SYSCALL_NUM, reinterpret_cast<void*>(mira_escape), NULL);

	//int32_t sysUtilModuleId = -1;
	int32_t netModuleId = -1;
	int32_t libcModuleId = -1;
	int32_t libkernModuleId = -1;
	//int32_t libKernelWebModuleId = -1;

	{
		Dynlib::LoadPrx("libSceLibcInternal.sprx", &libcModuleId);

		Dynlib::Dlsym(libcModuleId, "snprintf", &snprintf);

		/* FILE Stuff*/

	Dynlib::Dlsym(libcModuleId, "fopen", &fopen);
	Dynlib::Dlsym(libcModuleId, "memcpy", &fmemcpy);
	Dynlib::Dlsym(libcModuleId, "fread", &fread);
	Dynlib::Dlsym(libcModuleId, "fwrite", &fwrite);
	Dynlib::Dlsym(libcModuleId, "fseek", &fseek);
	Dynlib::Dlsym(libcModuleId, "ftell", &ftell);
	Dynlib::Dlsym(libcModuleId, "fclose", &fclose);
	Dynlib::Dlsym(libcModuleId, "printf", &printf);



		if(Dynlib::LoadPrx("libkernel_web.sprx", &libkernModuleId))
		    Dynlib::LoadPrx("libkernel_sys.sprx", &libkernModuleId);

	Dynlib::Dlsym(libkernModuleId, "sceKernelOpen", &sceKernelOpen);
	Dynlib::Dlsym(libkernModuleId, "sceKernelClose", &sceKernelClose);
	Dynlib::Dlsym(libkernModuleId, "sceKernelLseek", &sceKernelLseek);
	Dynlib::Dlsym(libkernModuleId, "sceKernelWrite", &sceKernelWrite);




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

	int32_t currentSize = 0;
	int32_t recvSize = 0;

     memset(buffer, 0, bufferSize);

if (strlen(usbpath()) == 0)
{
network:

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



	// Recv one byte at a time until we get our buffer
	while ((recvSize = sceNetRecv(clientSocket, buffer + currentSize, bufferSize - currentSize, 0)) > 0)
		currentSize += recvSize;

	// Close the client and server socket connections
	sceNetSocketClose(clientSocket);
	sceNetSocketClose(serverSocket);
}

int hddfile = sceKernelOpen("/user/MiraLoader.elf", 0x0000, 0x0000); 

if (strlen(usbpath()) != 0 ||  hddfile > 0)
{
 char filebuffer[200] = { 0 };




snprintf(filebuffer, 200, "%s/MiraLoader.elf", usbpath());
int filefd = sceKernelOpen(filebuffer, 0x000, 0x000);
if(filefd > 0)
{
	WriteNotificationLog("Found USB");
	printf("USB at %s\n", filebuffer);
	//WriteNotificationLog(filebuffer);
}



if (filefd > 0)
{

	if (sceKernelOpen("/user/MiraLoader.elf", 0x0000, 0x0000) > 0)
	{
		printf("HDD ELF already exists checking Hashs\n");
		if (MD5_hash_compare(filebuffer) != SAME_HASH)
		{

            printf("MD5_hash_compare Report different HASH copying\n");
			if (copyFile(filebuffer) == 0)
			{

				filefd = hddfile;
				
			}
			else
			{
				WriteNotificationLog("Copy FAILED switching to Network Mode");
				goto network;
			}
		}
		else
		{
			printf("[UPDATE] MD5_hash_compare Reports SAME HASH no copying needed\n");
		}
	}
	else
	{
		printf("HDD ELF DOESNT Exist && USB is connected with file copying from %s\n", filebuffer);
		if (copyFile(filebuffer) == -1)
		{
			WriteNotificationLog("Copy FAILED switching to Network Mode");
			goto network;
		}

	}

}
else
{
	WriteNotificationLog("Loading from HDD no USB");
	printf("Loading from HDD no USB\n");
	filefd = hddfile;
}
	


    currentSize = sceKernelLseek(filefd, 0, 2);

	if (currentSize < 0)
	{
        WriteNotificationLog("invaild file size switching to Network Mode");
        goto network;

    }


    sceKernelLseek(filefd, 0, 0);

	 
	 buffer = (unsigned char*)_mmap(0,currentSize,PROT_READ,MAP_FILE|MAP_PRIVATE,filefd,0);
  if (buffer == MAP_FAILED) {WriteNotificationLog("Failed to alloc MMAP switching to Network Mode");  goto network;}


  	sceKernelClose(filefd);
	sceKernelClose(hddfile);

}
	
else
{
WriteNotificationLog("File Not Found Switching to Network Mode");
goto network;

}



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