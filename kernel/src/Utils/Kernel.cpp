// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Kernel.hpp"
#include <Mira.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Types.hpp>
#include <Utils/Kdlsym.hpp>

extern "C"
{
    #include <sys/proc.h>
    #include <sys/uio.h>
    #include <sys/mount.h>
    #include <machine/stdarg.h>
}

int proc_rw_mem(struct proc* p, void* ptr, size_t size, void* data, size_t* n, int write) 
{
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

    auto proc_rwmem = (int(*)(struct proc* p, struct uio* uio))kdlsym(proc_rwmem);
    struct thread* td = s_DebuggerThread;
    struct iovec iov;
    struct uio uio;
    int ret;

    if (!p) {
        ret = EINVAL;
        goto error;
    }

    if (size == 0) {
        if (n)
            *n = 0;
        ret = 0;
        goto error;
    }

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = (caddr_t)data;
    iov.iov_len = size;

    memset(&uio, 0, sizeof(uio));
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_offset = (off_t)ptr;
    uio.uio_resid = (ssize_t)size;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_rw = write ? UIO_WRITE : UIO_READ;
    uio.uio_td = td;

    ret = proc_rwmem(p, &uio);
    if (n)
        *n = (size_t)((ssize_t)size - uio.uio_resid);

error:
   return ret;
}

int proc_rw_mem_pid(int pid, void* ptr, size_t size, void* data, size_t* n, int write)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
    struct proc* process = pfind(pid);
    if (process == nullptr)
        return -EPROCUNAVAIL;
    _mtx_unlock_flags(&process->p_mtx, 0, __FILE__, __LINE__);

    return proc_rw_mem(process, ptr, size, data, n, write);
}


struct proc *proc_find_by_name(const char *name)
{
    if (!name) {
        return NULL;
    }

    struct proclist* allproc = (struct proclist*)*(uint64_t*)kdlsym(allproc);
    WriteLog(LL_Info, "All Proc: %p", allproc);

    struct proc* p = NULL;
    FOREACH_PROC_IN_SYSTEM(p)
    {
        if (!memcmp(p->p_comm, name, strlen(name))) {
            return p;
        }
    }

    return NULL;
}

void build_iovec(struct iovec **iov, int *iovlen, const char *name, const char *val, size_t len)
{
    auto strdup = (char*(*)(const char *string, struct malloc_type* type))kdlsym(strdup);
    auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
    auto realloc = (void*(*)(void *addr, unsigned long size, struct malloc_type *mtp, int flags))kdlsym(realloc);
    struct malloc_type* M_MOUNT = (struct malloc_type*)kdlsym(M_MOUNT);

    int i;

    if (*iovlen < 0)
        return;

    i = *iovlen;

    *iov = (struct iovec*)realloc(*iov, sizeof **iov * (i + 2), M_MOUNT, 0);

    if (*iov == NULL) {
        *iovlen = -1;
        return;
    }

    if (name) {
        (*iov)[i].iov_base = strdup(name, M_MOUNT);
        (*iov)[i].iov_len = strlen(name) + 1;
    } else {
        (*iov)[i].iov_base = NULL;
        (*iov)[i].iov_len = 0;
    }
    i++;

    if (val) {
        (*iov)[i].iov_base = strdup(val, M_MOUNT);

        if (len == (size_t)-1) {
                len = strlen(val) + 1;
        }

        (*iov)[i].iov_len = (int)len;
    } else {
        (*iov)[i].iov_base = NULL;
        (*iov)[i].iov_len = 0;
    }

    *iovlen = ++i;
}
