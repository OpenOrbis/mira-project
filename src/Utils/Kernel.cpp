#include "Kernel.hpp"
#include <Mira.hpp>
#include <Utils/Kdlsym.hpp>
extern "C"
{
    #include <sys/proc.h>
    #include <sys/uio.h>
}

int proc_rw_mem(struct proc* p, void* ptr, size_t size, void* data, size_t* n, int write) 
{
    auto proc_rwmem = (int(*)(struct proc* p, struct uio* uio))kdlsym(proc_rwmem);
    struct thread* td = Mira::Framework::GetFramework()->GetMainThread();
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