// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Utilities.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Plugins/Substitute/Substitute.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/lock.h>
    #include <sys/mutex.h>

    #include <sys/systm.h>
    #include <sys/proc.h>
    #include <sys/errno.h>
    #include <sys/ptrace.h>

    #include <vm/vm.h>
    #include <vm/pmap.h>
    #include <vm/vm_map.h>

	#include <sys/uio.h>
	#include <sys/mount.h>
    #include <sys/mman.h>

    #include <sys/syslimits.h>
    #include <sys/param.h>
};

using namespace Mira::OrbisOS;

// Credits: m0rph (https://github.com/m0rph3us1987)
// This allows the short jump hooks to jump far (into Mira's dynamically allocated memory, which could be anywhere)
void Utilities::HookFunctionCall(uint8_t* p_HookTrampoline, void* p_Function, void* p_Address)
{
    uint8_t* s_HookPayload = p_HookTrampoline;
    uint16_t* s_TempAddress = reinterpret_cast<uint16_t*>(p_HookTrampoline);
    s_TempAddress++;

    uint64_t* s_FunctionAddress = reinterpret_cast<uint64_t*>(s_TempAddress);

    cpu_disable_wp();

    // mov rax
    s_HookPayload[0] = 0x48;
    s_HookPayload[1] = 0xB8;

    *s_FunctionAddress = reinterpret_cast<uint64_t>(p_Function);

    s_HookPayload[0x0A] = 0xFF;
    s_HookPayload[0x0B] = 0xE0;

    int32_t s_CallAddress = (int32_t)(p_HookTrampoline - (uint8_t*)p_Address) - 5;
    s_HookPayload = reinterpret_cast<uint8_t*>(p_Address);
    s_HookPayload++;
    int32_t* s_Pointer = reinterpret_cast<int32_t*>(s_HookPayload);
    *s_Pointer = s_CallAddress;

    cpu_enable_wp();
}

uint64_t Utilities::PtraceIO(int32_t p_ProcessId, int32_t p_Operation, void* p_DestAddress, void* p_ToReadWriteAddress, size_t p_ToReadWriteSize)
{
    if (p_ProcessId < 0)
        return -EIO;
    
    auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
		return -EIO;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
		return -EIO;
	}

    struct ptrace_io_desc s_Desc
    {
        .piod_op = p_Operation,
        .piod_offs = p_DestAddress,
        .piod_addr = p_ToReadWriteAddress,
        .piod_len = p_ToReadWriteSize
    };

    uint64_t s_Ret = kptrace_t(PT_IO, p_ProcessId, (caddr_t)&s_Desc, 0, s_DebuggerThread);
    if (s_Ret != 0)
        return s_Ret;
    else
        return (uint64_t)s_Desc.piod_len;
}

// Credits: flatz (https://github.com/flatz)
int Utilities::ProcessReadWriteMemory(struct ::proc* p_Process, void* p_DestAddress, size_t p_Size, void* p_ToReadWriteAddress, size_t* p_BytesReadWrote, bool p_Write)
{
	if (p_Process == nullptr)
		return -EPROCUNAVAIL;
	
	if (p_DestAddress == nullptr)
		return -EINVAL;
	
	if (p_ToReadWriteAddress == nullptr)
		return -EINVAL;

	auto s_ThreadManager = Mira::Framework::GetFramework()->GetThreadManager();
	if (s_ThreadManager == nullptr)
	{
		WriteLog(LL_Error, "could not get thread manager.");
		return -EIO;
	}

	auto s_DebuggerThread = s_ThreadManager->GetDebuggerThread();
	if (s_DebuggerThread == nullptr)
	{
		WriteLog(LL_Error, "could not get debugger thread.");
		return -EIO;
	}

	struct iovec s_Iov;
	struct uio s_Uio;
	int s_Ret = 0;

	if (!p_Process) {
		return 1;
	}

	if (p_Size == 0) {
		if (p_BytesReadWrote) {
			*p_BytesReadWrote = 0;
		}

		return 0;
	}

	memset(&s_Iov, NULL, sizeof(s_Iov));
	s_Iov.iov_base = p_ToReadWriteAddress;
	s_Iov.iov_len = p_Size;

	memset(&s_Uio, NULL, sizeof(s_Uio));
	s_Uio.uio_iov = &s_Iov;
	s_Uio.uio_iovcnt = 1;
	s_Uio.uio_offset = (uint64_t)p_DestAddress;
	s_Uio.uio_resid = (uint64_t)p_Size;
	s_Uio.uio_segflg = UIO_SYSSPACE;
	s_Uio.uio_rw = p_Write ? UIO_WRITE : UIO_READ;
	s_Uio.uio_td = s_DebuggerThread;

	auto proc_rwmem = (int (*)(struct proc *p, struct uio *uio))kdlsym(proc_rwmem);
	s_Ret = proc_rwmem(p_Process, &s_Uio);

	if (p_BytesReadWrote) {
		*p_BytesReadWrote = (size_t)((uint64_t)p_Size - s_Uio.uio_resid);
	}

	return s_Ret;
}

// Credits: flatz
struct proc* Utilities::FindProcessByName(const char* p_Name) 
{
	auto _sx_slock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_slock);
	auto _sx_sunlock = (void(*)(struct sx *sx, const char *file, int line))kdlsym(_sx_sunlock);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);

	struct sx* allproclock = (struct sx*)kdlsym(allproc_lock);
	struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);

	struct proc* s_FoundProc = nullptr;

	if (!p_Name)
		return NULL;

	_sx_slock(allproclock, 0, __FILE__, __LINE__);

	do
	{
		struct proc* s_Proc = nullptr;

		FOREACH_PROC_IN_SYSTEM(s_Proc) 
		{
			PROC_LOCK(s_Proc);

			if (strcmp(s_Proc->p_comm, p_Name) == 0) {
				s_FoundProc = s_Proc;
				PROC_UNLOCK(s_Proc);
				break;
			}

			PROC_UNLOCK(s_Proc);
		}
	} while (false);

	_sx_sunlock(allproclock, __FILE__, __LINE__);

	return s_FoundProc;
}


// Credits: flatz
int Utilities::GetProcessVmMap(struct ::proc* p_Process, ProcVmMapEntry** p_Entries, size_t* p_NumEntries) 
{
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto vmspace_acquire_ref = (struct vmspace* (*)(struct proc *))kdlsym(vmspace_acquire_ref);
	auto vmspace_free = (void(*)(struct vmspace *))kdlsym(vmspace_free);
	auto wakeup = (void(*)(void*))kdlsym(wakeup);
	auto _vm_map_lock_read = (void(*)(vm_map_t map, const char *file, int line))kdlsym(_vm_map_lock_read);
	auto _vm_map_unlock_read = (void(*)(vm_map_t map, const char *file, int line))kdlsym(_vm_map_unlock_read);
	auto faultin = (void(*)(struct proc *p))kdlsym(faultin);

	struct vmspace* vm = nullptr;
	ProcVmMapEntry* info = nullptr;
	vm_map_t map = nullptr;
	vm_map_entry_t entry = nullptr;
	size_t n, i = 0;
	int ret = 0;
    size_t allocSize = 0;// n * sizeof(*info);

	if (!p_Process) {
		ret = EINVAL;
		goto error;
	}
	if (!p_Entries) {
		ret = EINVAL;
		goto error;
	}
	if (!p_NumEntries) {
		ret = EINVAL;
		goto error;
	}

	PROC_LOCK(p_Process);
	if (p_Process->p_flag & P_WEXIT) {
		PROC_UNLOCK(p_Process);
		ret = ESRCH;
		goto error;
	}
	_PHOLD(p_Process);
	PROC_UNLOCK(p_Process);

	vm = vmspace_acquire_ref(p_Process);
	if (!vm) {
		PRELE(p_Process);
		ret = ESRCH;
		goto error;
	}
	map = &vm->vm_map;

	vm_map_lock_read(map);
	for (entry = map->header.next, n = 0; entry != &map->header; entry = entry->next) {
		if (entry->eflags & MAP_ENTRY_IS_SUB_MAP)
			continue;
		++n;
	}
	if (n == 0)
		goto done;
	
    allocSize = n * sizeof(*info);
	info = (ProcVmMapEntry*)new uint8_t[allocSize];
	if (!info) {
		vm_map_unlock_read(map);
		vmspace_free(vm);

		PRELE(p_Process);

		ret = ENOMEM;
		goto error;
	}
	memset(info, 0, n * sizeof(*info));
	for (entry = map->header.next, i = 0; entry != &map->header; entry = entry->next) {
		if (entry->eflags & MAP_ENTRY_IS_SUB_MAP)
			continue;

		info[i].start = entry->start;
		info[i].end = entry->end;
		info[i].offset = entry->offset;
		memcpy(info[i].name, entry->name, sizeof(info[i].name));

		info[i].prot = 0;
		if (entry->protection & VM_PROT_READ)
			info[i].prot |= PROT_CPU_READ;
		if (entry->protection & VM_PROT_WRITE)
			info[i].prot |= PROT_CPU_WRITE;
		if (entry->protection & VM_PROT_EXECUTE)
			info[i].prot |= PROT_CPU_EXEC;
		if (entry->protection & VM_PROT_GPU_READ)
			info[i].prot |= PROT_GPU_READ;
		if (entry->protection & VM_PROT_GPU_WRITE)
			info[i].prot |= PROT_GPU_WRITE;

		++i;
	}

done:
	vm_map_unlock_read(map);
	vmspace_free(vm);

	PRELE(p_Process);

	*p_NumEntries = n;
	*p_Entries = info;

	info = NULL;
	ret = 0;

error:
	if (info)
        delete [] info;
		//kfree(info, allocSize);

	return ret;
}

// Allow / Disallow to write on a executable
int Utilities::ExecutableWriteProtection(struct proc* p, bool write_allowed) {
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);

    if (!s_ProcessThread) {
    	WriteLog(LL_Error, "[%d] Could not get the first thread.", p->p_pid);
        return -1;
    }

    // Get the start text address of my process
    uint64_t s_TextStart = 0;
    uint64_t s_TextSize = 0;
    ProcVmMapEntry* s_Entries = nullptr;
    size_t s_NumEntries = 0;
    auto s_Ret = Utilities::GetProcessVmMap(p, &s_Entries, &s_NumEntries);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "[%d] Could not get the VM Map.", p->p_pid);
        return -2;
    }

    if (s_Entries == nullptr || s_NumEntries == 0)
    {
        WriteLog(LL_Error, "[%d] Invalid entries (%p) or numEntries (%d)", p->p_pid, s_Entries, s_NumEntries);
        return -3;
    }

    for (auto i = 0; i < s_NumEntries; ++i)
    {
        if (s_Entries[i].prot == (PROT_READ | PROT_EXEC))
        {
            s_TextStart = (uint64_t)s_Entries[i].start;
            s_TextSize = ((uint64_t)s_Entries[i].end - (uint64_t)s_Entries[i].start);
            break;
        }
    }

    if (s_TextStart == 0 || s_TextSize)
    {
        WriteLog(LL_Error, "[%d] Could not find text start or size for this process !", p->p_pid);
        WriteLog(LL_Error, "[%d] Could not find text start or size for this process !", p->p_pid);

	    // Free the s_Entries
	    delete [] s_Entries;
	    s_Entries = nullptr;
        return -4;
    } else {
        WriteLog(LL_Info, "[%d] text pointer: %p !", p->p_pid, s_TextStart);
    }

    if (write_allowed) {
    	s_Ret = kmprotect_t((void*)s_TextStart, s_TextSize, (PROT_READ | PROT_WRITE | PROT_EXEC), s_ProcessThread);
    	if (s_Ret < 0) {
    		WriteLog(LL_Error, "[%d] Unable to mprotect(1) ! (err: %d)", p->p_pid, s_Ret);
    	}
    } else {
    	s_Ret = kmprotect_t((void*)s_TextStart, s_TextSize, (PROT_READ | PROT_EXEC), s_ProcessThread);
    	if (s_Ret < 0) {
    		WriteLog(LL_Error, "[%d] Unable to mprotect(2) ! (err: %d)", p->p_pid, s_Ret);
    	}
    }

    // Free the s_Entries
    delete [] s_Entries;
    s_Entries = nullptr;
    return 0;
}

// Mount NullFS folder
int Utilities::MountNullFS(char* where, char* what, int flags)
{
    auto mount_argf = (struct mntarg*(*)(struct mntarg *ma, const char *name, const char *fmt, ...))kdlsym(mount_argf);
    auto kernel_mount = (int(*)(struct mntarg	*ma, int flags))kdlsym(kernel_mount);

    struct mntarg* ma = NULL;

    ma = mount_argf(ma, "fstype", "%s", "nullfs");
    ma = mount_argf(ma, "fspath", "%s", where);
    ma = mount_argf(ma, "target", "%s", what);

    if (ma == NULL) {
    	WriteLog(LL_Error, "Something is wrong, ma value is null after argument");
    	return 50;
    }

    return kernel_mount(ma, flags);
}

// Kill a process by this proc
int Utilities::KillProcess(struct proc* p)
{
	auto killproc = (int(*)(struct proc *p, const char *why))kdlsym(killproc);
	return killproc(p, "Mira");
}

// Based on the work of JOGolden (JKPatch)
// Create a PThread (POSIX Thread) on remote process
int Utilities::CreatePOSIXThread(struct proc* p, void* entrypoint) {
	auto thr_create = (int (*)(struct thread * td, uint64_t ctx, void* start_func, void *arg, char *stack_base, size_t stack_size, char *tls_base, long * child_tid, long * parent_tid, uint64_t flags, uint64_t rtp))kdlsym(kern_thr_create);

    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);

	// Check if arguments is correct
	if (!p || !entrypoint) {
		WriteLog(LL_Error, "Invalid argument !");
		return -1;
	}

	// Got substitute plugin
	Plugins::Substitute* substitute = Mira::Plugins::Substitute::GetPlugin();
	if (!substitute) {
		WriteLog(LL_Error, "Substitute dependency is required.");
		return -2;
	}

	size_t s_Size = 0;
	int s_Ret = 0;
	char* s_TitleId = (char*)((uint64_t)p + 0x390);

	WriteLog(LL_Info, "[%s] Creating POSIX Thread (Entrypoint: %p) ...", s_TitleId, entrypoint);

	// Resolve all addresses
	void* s_scePthreadAttrInit = substitute->FindOriginalAddress(p, "scePthreadAttrInit", 0);
	void* s_scePthreadAttrSetstacksize = substitute->FindOriginalAddress(p, "scePthreadAttrSetstacksize", 0);
	void* s_scePthreadCreate = substitute->FindOriginalAddress(p, "scePthreadCreate", 0);
	void* s_pthread_getthreadid_np = substitute->FindOriginalAddress(p, "pthread_getthreadid_np", 0);

	if (!s_scePthreadAttrInit || !s_scePthreadAttrSetstacksize || !s_scePthreadCreate || !s_pthread_getthreadid_np) {
		WriteLog(LL_Error, "[%s] Unable to resolve addresses !", s_TitleId);
		WriteLog(LL_Error, "[%s] scePthreadAttrInit: %p", s_TitleId, s_scePthreadAttrInit);
		WriteLog(LL_Error, "[%s] scePthreadAttrSetstacksize: %p", s_TitleId, s_scePthreadAttrSetstacksize);
		WriteLog(LL_Error, "[%s] scePthreadCreate: %p", s_TitleId, s_scePthreadCreate);
		WriteLog(LL_Error, "[%s] pthread_getthreadid_np: %p", s_TitleId, s_pthread_getthreadid_np);

		return -3;
	}

	// Determine thr_initial by finding the first instruction *cmp* and got the relative address
	unsigned char s_ValidInstruction[3];
	s_Size = sizeof(s_ValidInstruction);
	s_Ret = proc_rw_mem(p, s_pthread_getthreadid_np, s_Size, s_ValidInstruction, &s_Size, false);
	if (s_Ret > 0) {
		WriteLog(LL_Error, "[%s] Unable to read process memory at %p !", s_TitleId, s_pthread_getthreadid_np);
		return -4;
	}

	if ( !(s_ValidInstruction[0] == 0x48 && s_ValidInstruction[1] == 0x83 && s_ValidInstruction[2] == 0x3D) ) {
		WriteLog(LL_Error, "[%s] Invalid instruction detected ! Abord.", s_TitleId);
		return -5;
	} 

	uint64_t s_RelativeAddress = 0;
	s_Size = sizeof(uint32_t);
	s_Ret = proc_rw_mem(p, (void*)((uint64_t)s_pthread_getthreadid_np + 0x3), s_Size, &s_RelativeAddress, &s_Size, false);
	if (s_Ret > 0) {
		WriteLog(LL_Error, "[%s] Unable to read process memory at %p !", s_TitleId, (void*)((uint64_t)s_pthread_getthreadid_np + 0x3));
		return -6;
	}

	void* s_thr_initial = (void*)((uint64_t)s_pthread_getthreadid_np + s_RelativeAddress + 0x8);

	// Payload containts all call needed for create a thread (The payload is inside the folders is in /src/OrbisOS/asm/, compile with NASM)
	unsigned char s_Payload[0x100] = "\x4D\x49\x52\x41\x4D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x72\x70\x63\x73\x74\x75\x62\x00\x48\x8B\x3D\xD9\xFF\xFF\xFF\x48\x8B\x37\x48\x8B\xBE\xE0\x01\x00\x00\xE8\x7A\x00\x00\x00\x48\x8D\x3D\xD3\xFF\xFF\xFF\x4C\x8B\x25\xA4\xFF\xFF\xFF\x41\xFF\xD4\xBE\x00\x00\x08\x00\x48\x8D\x3D\xBD\xFF\xFF\xFF\x4C\x8B\x25\x96\xFF\xFF\xFF\x41\xFF\xD4\x4C\x8D\x05\xB4\xFF\xFF\xFF\xB9\x00\x00\x00\x00\x48\x8B\x15\x70\xFF\xFF\xFF\x48\x8D\x35\x99\xFF\xFF\xFF\x48\x8D\x3D\x8A\xFF\xFF\xFF\x4C\x8B\x25\x73\xFF\xFF\xFF\x41\xFF\xD4\xC6\x05\x50\xFF\xFF\xFF\x01\xBF\x00\x00\x00\x00\xE8\x01\x00\x00\x00\xC3\xB8\xAF\x01\x00\x00\x49\x89\xCA\x0F\x05\xC3\xB8\xA5\x00\x00\x00\x49\x89\xCA\x0F\x05\xC3\x55\x48\x89\xE5\x53\x48\x83\xEC\x18\x48\x89\x7D\xE8\x48\x8D\x75\xE8\xBF\x81\x00\x00\x00\xE8\xDA\xFF\xFF\xFF\x48\x83\xC4\x18\x5B\x5D\xC3";

	// Setup payload
	struct posixldr_header* s_PayloadHeader = (struct posixldr_header*)s_Payload;
	s_PayloadHeader->stubentry = (uint64_t)entrypoint;
	s_PayloadHeader->scePthreadAttrInit = (uint64_t)s_scePthreadAttrInit;
	s_PayloadHeader->scePthreadAttrSetstacksize = (uint64_t)s_scePthreadAttrSetstacksize;
	s_PayloadHeader->scePthreadCreate = (uint64_t)s_scePthreadCreate;
	s_PayloadHeader->thr_initial = (uint64_t)s_thr_initial;

	// Setup memory space in remote process
	size_t s_StackSize = 0x8000;
	size_t s_PayloadSize = sizeof(s_Payload) + s_StackSize; // 0x8000 is stack
	auto s_PayloadSpace = kmmap_t(nullptr, s_PayloadSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PREFAULT_READ, -1, 0, s_ProcessThread);
	if (s_PayloadSpace == nullptr || s_PayloadSpace == MAP_FAILED || (uint64_t)s_PayloadSpace < 0) {
		WriteLog(LL_Error, "[%s] Unable to allocate remote process memory (%llx size) (ret: %llx)", s_TitleId, s_PayloadSize, s_PayloadSpace);
		return -7;
	}

	// Copy payload to process
	s_Size = sizeof(s_Payload);
	s_Ret = proc_rw_mem(p, (void*)(s_PayloadSpace), s_Size, s_Payload, &s_Size, true);
	if (s_Ret > 0) {
		WriteLog(LL_Error, "[%s] Unable to write process memory at %p !", s_TitleId, (void*)(s_PayloadSpace));
		kmunmap_t(s_PayloadSpace, s_PayloadSize, s_ProcessThread);
		return -8;
	}

	// Define stack address
	char* s_StackAddr = (char*)((uint64_t)s_PayloadSpace + sizeof(s_Payload));
	void* s_PayloadEntrypoint = (void*)((uint64_t)s_PayloadSpace + s_PayloadHeader->entrypoint);

	// Create thread
	s_Ret = thr_create(s_ProcessThread, NULL, s_PayloadEntrypoint, NULL, s_StackAddr, s_StackSize, NULL, NULL, NULL, 0, NULL);
	if (s_Ret) {
		WriteLog(LL_Error, "[%s] Unable to launch thread ! (ret: %d)", s_TitleId, s_Ret);
		kmunmap_t(s_PayloadSpace, s_PayloadSize, s_ProcessThread);
		return -9;
	}

	/* TODO : Cleanup remote process memory
	// Wait until it's done
	void* s_ResultAddr = (void*)( (uint64_t)s_PayloadSpace + sizeof(uint32_t) + sizeof(uint64_t) );

	uint8_t s_Ldrdone = 0;
	s_Size = sizeof(uint8_t);
	while (s_Ldrdone == 0) {
		s_Ret = proc_rw_mem(p, s_ResultAddr, s_Size, &s_Ldrdone, &s_Size, false);
		if (s_Ret) {
			WriteLog(LL_Error, "[%s] Unable to read process memory at %p !", s_TitleId, s_ResultAddr);
			break;
		}
	}
	

	// Cleanup remote memory
	kmunmap_t(s_PayloadSpace, s_PayloadSize, s_ProcessThread);
	*/

	WriteLog(LL_Info, "[%s] Creating POSIX Thread (Entrypoint: %p) : Done.", s_TitleId, entrypoint);

	return 0;
}

// Load SPRX via a POSIX Thread
int Utilities::LoadPRXModule(struct proc* p, const char* prx_path)
{
    struct thread* s_ProcessThread = FIRST_THREAD_IN_PROC(p);
    if (!s_ProcessThread) {
		WriteLog(LL_Error, "Invalid process !");
			return -1;
    }

	// Check if arguments is correct
	if (!p || !prx_path || strlen(prx_path) > PATH_MAX) {
		WriteLog(LL_Error, "Invalid argument !");
		return -1;
	}

	// Got substitute plugin
	Plugins::Substitute* substitute = Mira::Plugins::Substitute::GetPlugin();
	if (!substitute) {
		WriteLog(LL_Error, "Substitute dependency is required.");
		return -2;
	}

	size_t s_Size = 0;
	int s_Ret = 0;
	char* s_TitleId = (char*)((uint64_t)p + 0x390);

	WriteLog(LL_Info, "[%s] Loading PRX (%s) over POSIX ...", s_TitleId, prx_path);

    // Find sceKernelLoadStartModule address
    void* s_LoadStartModule = substitute->FindOriginalAddress(p, "sceKernelLoadStartModule", 0);
    if (!s_LoadStartModule) {
        WriteLog(LL_Error, "[%s] could not find sceKernelLoadStartModule !", s_TitleId);
        return -3;
    }

	// Payload containts all call needed for create a thread (The payload is inside the folders is in /src/OrbisOS/asm/, compile with NASM)
	unsigned char s_Payload[0x100] = "\x4D\x49\x52\x41\x25\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x72\x78\x73\x74\x75\x62\x00\x48\x8B\x3D\xE1\xFF\xFF\xFF\x48\x31\xF6\x48\x31\xD2\x48\x31\xC9\x4D\x31\xC0\x4D\x31\xC9\x4C\x8B\x25\xD3\xFF\xFF\xFF\x41\xFF\xD4\x31\xC0\xC3";	

	// Allocate memory
	size_t s_PayloadSize = 0x8000; //sizeof(s_Payload) + PATH_MAX but need more for allow the allocation
	auto s_PayloadSpace = kmmap_t(nullptr, s_PayloadSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PREFAULT_READ, -1, 0, s_ProcessThread);
	if (s_PayloadSpace == nullptr || s_PayloadSpace == MAP_FAILED || (uint64_t)s_PayloadSpace < 0) {
		WriteLog(LL_Error, "[%s] Unable to allocate remote process memory (%llx size) (ret: %llx)", s_TitleId, s_PayloadSize, s_PayloadSpace);
		return -3;
	}

	// Setup payload
	struct prxldr_header* s_PayloadHeader = (struct prxldr_header*)s_Payload;
	s_PayloadHeader->prx_path = (uint64_t)((uint64_t)s_PayloadSpace + sizeof(s_Payload));
	s_PayloadHeader->sceKernelLoadStartModule = (uint64_t)s_LoadStartModule;

	// Copy payload to process
	s_Size = sizeof(s_Payload);
	s_Ret = proc_rw_mem(p, (void*)(s_PayloadSpace), s_Size, s_Payload, &s_Size, true);
	if (s_Ret > 0) {
		WriteLog(LL_Error, "[%s] Unable to write process memory at %p !", s_TitleId, (void*)(s_PayloadSpace));
		return -4;
	}

	// Copy path to process
	s_Size = strlen(prx_path) + 1;
	s_Ret = proc_rw_mem(p, (void*)(s_PayloadHeader->prx_path), s_Size, (void*)prx_path, &s_Size, true);
	if (s_Ret > 0) {
		WriteLog(LL_Error, "[%s] Unable to write process memory at %p !", s_TitleId, (void*)(s_PayloadHeader->prx_path));
		return -5;
	}

	// Calculate entrypoint
	void* s_Entrypoint = (void*)((uint64_t)s_PayloadSpace + s_PayloadHeader->entrypoint);

	// Create a POSIX Thread
	CreatePOSIXThread(p, s_Entrypoint);

	/* TODO : Cleanup remote process memory */

	WriteLog(LL_Info, "[%s] Loading PRX (%s) over POSIX : Done.", s_TitleId, prx_path);

	return 0;
}
