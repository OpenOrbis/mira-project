#include <Utils/SysWrappers.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Syscall.hpp>

extern "C"
{
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
    #include <sys/pcpu.h>
    #include <sys/proc.h>
    #include <vm/vm.h>

    #include <sys/_iovec.h>
    #include <sys/uio.h>

    #include <fcntl.h>
}


#ifndef MAP_FAILED
#define MAP_FAILED      ((void *)-1)
#endif

int kwait4(int pid, int *status, int options, struct rusage *rusage)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto wait4 = (int(*)(struct thread* thread, struct wait_args*))sysents[SYS_WAIT4].sy_call;
	if (!wait4)
		return -1;

	int error;
	struct wait_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.pid = pid;
	uap.status = status;
	uap.options = options;
	uap.rusage = rusage;
	error = wait4(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kmlock(void* address, uint64_t size)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto mlock = (int(*)(struct thread*, struct mlock_args*))sysents[SYS_MLOCK].sy_call;
	if (!mlock)
		return -1;

	int error;
	struct mlock_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.addr = (const void*)((uint64_t)address & 0xffffffffffffc000);
	uap.len = size;
	error = mlock(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kmlockall(int how)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mlockall = (int(*)(struct thread*, struct mlockall_args*))sysents[SYS_MLOCKALL].sy_call;
	if (!sys_mlockall)
		return -1;

	int error;
	struct mlockall_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.how = how;
	error = sys_mlockall(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

off_t klseek(int fd, off_t offset, int whence)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_lseek = (int(*)(struct thread*, struct lseek_args*))(void*)sysents[SYS_LSEEK].sy_call;
	
	if (!sys_lseek)
		return -1;

	int error;
	struct lseek_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.offset = offset;
	uap.whence = whence;
	error = sys_lseek(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}
caddr_t kmmap(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mmap = (int(*)(struct thread*, struct mmap_args*))sysents[SYS_MMAP].sy_call;
	if (!sys_mmap)
		return (caddr_t)-1;

	int error;
	struct mmap_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.addr = addr;
	uap.len = len;
	uap.prot = prot;
	uap.flags = flags;
	uap.fd = fd;
	uap.pos = pos;
	error = sys_mmap(td, &uap);
	if (error)
		return (caddr_t)(int64_t)-error;

	// return
	return (caddr_t)td->td_retval[0];
}

int kmunmap(void *addr, size_t len)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_munmap = (int(*)(struct thread*, struct munmap_args*))sysents[SYS_MUNMAP].sy_call;
	if (!sys_munmap)
		return -1;

	int error;
	struct munmap_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.addr = addr;
	uap.len = len;
	error = sys_munmap(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

ssize_t kread(int fd, void* buf, size_t count)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_read = (int(*)(struct thread*, struct read_args*))sysents[SYS_READ].sy_call;
	if (!sys_read)
		return -1;

	int error;
	struct read_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.buf = buf;
	uap.nbyte = count;

	error = sys_read(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kfstat(int fd, struct stat* sb)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_fstat = (int(*)(struct thread*, struct fstat_args*))sysents[SYS_FSTAT].sy_call;
	if (!sys_fstat)
		return -1;

	int error;
	struct fstat_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.sb = (struct stat *)sb;

	error = sys_fstat(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kstat(char* path, struct stat* buf)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_stat = (int(*)(struct thread*, struct stat_args*))sysents[SYS_STAT].sy_call;
	if (!sys_stat)
		return -1;

	int error;
	struct stat_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.ub = buf;

	error = sys_stat(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kstat_t(char* path, struct stat* buf, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_stat = (int(*)(struct thread*, struct stat_args*))sysents[SYS_STAT].sy_call;
	if (!sys_stat)
		return -1;

	int error;
	struct stat_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.ub = buf;

	error = sys_stat(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

void kclose_t(int socket, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_close = (int(*)(struct thread *, struct close_args *))sysents[SYS_CLOSE].sy_call;
	if (!ksys_close)
		return;

	int error;
	struct close_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = socket;
	error = ksys_close(td, &uap);
	if (error)
		return;
}

void kclose(int socket)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_close = (int(*)(struct thread *, struct close_args *))sysents[SYS_CLOSE].sy_call;
	if (!ksys_close)
		return;

	int error;
	struct close_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = socket;
	error = ksys_close(td, &uap);
	if (error)
		return;
}

int ksocket(int a, int b, int c)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_socket = (int(*)(struct thread*, struct socket_args*))sysents[SYS_SOCKET].sy_call;

	int error;
	struct socket_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.domain = a;
	uap.type = b;
	uap.protocol = c;
	error = ksys_socket(td, &uap);
	if (error)
		return -error;


	// return socket
	return td->td_retval[0];
}

ssize_t kwrite(int d, const void* buf, size_t nbytes)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_write = (int(*)(struct thread*, struct write_args*))sysents[SYS_WRITE].sy_call;
	if (!ksys_write)
		return -1;

	int error;
	struct write_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = d;
	uap.buf = buf;
	uap.nbyte = nbytes;
	error = ksys_write(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}
int kgetdents(int fd, char* buf, int nbytes)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_getdents = (int(*)(struct thread*, struct getdents_args*))sysents[SYS_GETDENTS].sy_call;

	int error;
	struct getdents_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.buf = buf;
	uap.count = (size_t)nbytes;
	error = sys_getdents(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kbind(int socket, const struct sockaddr * b, size_t c)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_bind = (int(*)(struct thread *, struct bind_args *))sysents[SYS_BIND].sy_call;

	int error;
	struct bind_args uap;

	(void)memset(&uap, 0, sizeof(uap));

	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = socket;
	uap.name = (caddr_t)b;
	uap.namelen = (int)c;
	error = ksys_bind(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int klisten(int sockfd, int backlog)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_listen = (int(*)(struct thread *, struct listen_args *))sysents[SYS_LISTEN].sy_call;
	int error;
	struct listen_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = sockfd;
	uap.backlog = backlog;
	error = ksys_listen(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kaccept(int sock, struct sockaddr * b, size_t* c)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_accept = (int(*)(struct thread *, struct accept_args *))sysents[SYS_ACCEPT].sy_call;
	int error;
	struct accept_args uap;

	struct thread* td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = sock;
	uap.name = b;
	uap.anamelen = (__socklen_t*)c;
	error = ksys_accept(td, &uap);

	// success?
	if (error == 0)
	{
		// return socket
		return td->td_retval[0];
	}
	// interrupted?
	else if ((error == EINTR) || (error == ERESTART))
	{
		// failed
		return -EINTR;
	}
	// failed?
	else if (error > 0)
		return -error;
	else
		return error;
}

int krecv(int s, void * buf, int len, int flags)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_recvfrom = (int(*)(struct thread *, struct recvfrom_args *))sysents[SYS_RECVFROM].sy_call;
	if (!ksys_recvfrom)
		return -1;

	int error;
	struct recvfrom_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = s;
	uap.buf = static_cast<caddr_t>(buf);
	uap.len = len;
	uap.flags = flags;
	uap.from = 0;
	uap.fromlenaddr = 0;
	error = ksys_recvfrom(td, &uap);

	// success?
	if (error == 0)
	{
		// return length
		return td->td_retval[0];
	}
	// invalid?
	else if (error == EFAULT)
	{
		// invalid memory
		return -EFAULT;
	}
	// interrupted?
	else if ((error == EINTR) || (error == ERESTART))
	{
		// failed
		return -EINTR;
	}
	// failed?
	else if (error > 0)
		return -error;
	else
		return error;
}

int ksend(int socket, caddr_t buf, size_t len, int flags)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_sendto = (int(*)(struct thread *, struct sendto_args *))sysents[SYS_SENDTO].sy_call;
	if (!ksys_sendto)
		return -1;

	int error;
	struct sendto_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = socket;
	uap.buf = buf;
	uap.len = len;
	uap.flags = flags;
	uap.to = 0;
	uap.tolen = 0;
	error = ksys_sendto(td, &uap);

	// success?
	if (error == 0)
	{
		// return length
		return td->td_retval[0];
	}
	// invalid?
	else if (error == EFAULT)
	{
		// invalid memory
		return -EFAULT;
	}
	// interrupted?
	else if ((error == EINTR) || (error == ERESTART))
	{
		// failed
		return -EINTR;
	}
	// failed?
	else if (error > 0)
		return -error;
	else
		return error;
}

int kopen_t(char* path, int flags, int mode, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_open = (int(*)(struct thread *, struct open_args *))sysents[SYS_OPEN].sy_call;

	int error;
	struct open_args uap;
	//struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.flags = flags;
	uap.mode = mode;

	error = ksys_open(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kopen(char* path, int flags, int mode)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_open = (int(*)(struct thread *, struct open_args *))sysents[SYS_OPEN].sy_call;

	int error;
	struct open_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.flags = flags;
	uap.mode = mode;

	error = ksys_open(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kdup2(int oldd, int newd)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_dup2 = (int(*)(struct thread *, struct dup2_args *))sysents[SYS_DUP2].sy_call;

	int error;
	struct dup2_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.from = oldd;
	uap.to = newd;

	error = sys_dup2(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kmkdir(char * path, int mode)
{
	auto kern_mkdirat = (int (*)(struct thread *td, int fd, char *path, enum uio_seg segflg, int mode))kdlsym(kern_mkdirat);

	struct thread *td = curthread;

	// clear the return
	td->td_retval[0] = 0;

	int error = kern_mkdirat(td, AT_FDCWD, path, UIO_SYSSPACE, mode);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}


int kmkdir_t(char * path, int mode, struct thread* td)
{
	auto kern_mkdirat = (int (*)(struct thread *td, int fd, char *path, enum uio_seg segflg, int mode))kdlsym(kern_mkdirat);
	// clear the return
	td->td_retval[0] = 0;

	int error = kern_mkdirat(td, AT_FDCWD, path, UIO_SYSSPACE, mode);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int krmdir(char * path)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_rmdir = (int(*)(struct thread *, struct rmdir_args *))sysents[SYS_RMDIR].sy_call;

	int error;
	struct rmdir_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;

	error = sys_rmdir(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int krmdir_t(char * path, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_rmdir = (int(*)(struct thread *, struct rmdir_args *))sysents[SYS_RMDIR].sy_call;

	int error;
	struct rmdir_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;

	error = sys_rmdir(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kshutdown(int s, int how)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_shutdown = (int(*)(struct thread *, struct shutdown_args *))sysents[SYS_SHUTDOWN].sy_call;

	int error;
	struct shutdown_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = s;
	uap.how = how;

	error = sys_shutdown(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kunlink(char* path)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_unlink = (int(*)(struct thread *, struct unlink_args *))sysents[SYS_UNLINK].sy_call;

	int error;
	struct unlink_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;

	error = sys_unlink(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int ksetuid(uid_t uid)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_setuid = (int(*)(struct thread *, struct setuid_args *))sysents[SYS_SETUID].sy_call;

	int error;
	struct setuid_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.uid = uid;

	error = sys_setuid(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int ksetuid_t(uid_t uid, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_setuid= (int(*)(struct thread *, struct setuid_args *))sysents[SYS_SETUID].sy_call;

	int error;
	struct setuid_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.uid = uid;

	error = sys_setuid(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kptrace(int req, pid_t pid, caddr_t addr, int data)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_ptrace = (int(*)(struct thread *, struct ptrace_args *))sysents[SYS_PTRACE].sy_call;

	int error;
	struct ptrace_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.req = req;
	uap.pid = pid;
	uap.addr = addr;
	uap.data = data;

	error = sys_ptrace(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kkill(int pid, int signum)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_kill = (int(*)(struct thread *, struct kill_args *))sysents[SYS_KILL].sy_call;

	int error;
	struct kill_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.pid = pid;
	uap.signum = signum;

	error = sys_kill(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int ksetsockopt(int socket, int level, int name, caddr_t val, int valsize)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_setsockopt = (int(*)(struct thread *, struct setsockopt_args *))sysents[SYS_SETSOCKOPT].sy_call;

	int error;
	struct setsockopt_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.s = socket;
	uap.level = level;
	uap.name = name;
	uap.val = val;
	uap.valsize = valsize;

	error = sys_setsockopt(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kftruncate(int fd, off_t length)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_ftruncate = (int(*)(struct thread *, struct ftruncate_args *))sysents[SYS_FTRUNCATE].sy_call;

	int error;
	struct ftruncate_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.length = length;

	error = sys_ftruncate(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

pid_t krfork_t(int flags, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_rfork= (int(*)(struct thread *, struct rfork_args *))sysents[SYS_RFORK].sy_call;

	int error;
	struct rfork_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.flags = flags;
	error = sys_rfork(td, &uap);
	if (error)
		return -error;

	// success
	return (pid_t)td->td_retval[0];
}

int kreboot(int opt)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_reboot = (int(*)(struct thread*, struct reboot_args*))sysents[SYS_REBOOT].sy_call;
	if (!sys_reboot)
		return -1;

	int error;
	struct reboot_args uap;
	struct thread *td = curthread;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.opt = opt;
	error = sys_reboot(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}