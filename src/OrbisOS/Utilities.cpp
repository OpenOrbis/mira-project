#include "Utilities.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kdlsym.hpp>

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
    
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();

    struct ptrace_io_desc s_Desc
    {
        .piod_op = p_Operation,
        .piod_offs = p_DestAddress,
        .piod_addr = p_ToReadWriteAddress,
        .piod_len = p_ToReadWriteSize
    };

    uint64_t s_Ret = kptrace_t(PT_IO, p_ProcessId, (caddr_t)&s_Desc, 0, s_MainThread);
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

	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
	if (s_MainThread == nullptr)
		return -EIO;

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
	s_Uio.uio_td = s_MainThread;

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

	struct proc* s_Proc = nullptr;

	if (!p_Name)
		return NULL;

	_sx_slock(allproclock, 0, __FILE__, __LINE__);

	FOREACH_PROC_IN_SYSTEM(s_Proc) {
		PROC_LOCK(s_Proc);

		// Do it because struct* proc is broken
		if (strcmp(s_Proc->p_comm, p_Name) == 0) {
			PROC_UNLOCK(s_Proc);
			goto done;
		}

		PROC_UNLOCK(s_Proc);
	}

	s_Proc = nullptr;

done:
	_sx_sunlock(allproclock, __FILE__, __LINE__);

	return s_Proc;
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

int Utilities::MountNullFS(char* where, char* what, int flags, struct thread* td)
{
    struct iovec* iov = NULL;
    int iovlen = 0;

    build_iovec(&iov, &iovlen, "fstype", "nullfs", (size_t)-1);
    build_iovec(&iov, &iovlen, "fspath", where, (size_t)-1); // Where i want to add
    build_iovec(&iov, &iovlen, "target", what, (size_t)-1); // What i want to add

    return knmount_t(iov, iovlen, flags, td);
}