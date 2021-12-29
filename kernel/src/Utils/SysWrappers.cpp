// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Utils/SysWrappers.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/_Syscall.hpp>

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

///
/// 7: sys_wait4
//
int kwait4(int pid, int* status, int options, struct rusage* rusage)
{
	return kwait4_t(pid, status, options, rusage, curthread);
}

int kwait4_internal(int pid, int *status, int options, struct rusage *rusage, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto wait4 = (int(*)(struct thread* thread, struct wait_args*))sysents[SYS_WAIT4].sy_call;
	if (!wait4)
		return -1;

	int error;
	struct wait_args uap;

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

int kwait4_t(int pid, int* status, int options, struct rusage* rusage, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kwait4_internal(pid, status, options, rusage, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}

			return ret;
		}

		break;
	}

	return ret;
}

//
// 203: sys_mlock
//
int kmlock(void* address, uint64_t size)
{
	return kmlock_t(address, size, curthread);
}

int kmlock_internal(void* address, uint64_t size, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto mlock = (int(*)(struct thread*, struct mlock_args*))sysents[SYS_MLOCK].sy_call;
	if (!mlock)
		return -1;

	int error;
	struct mlock_args uap;

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

int kmlock_t(void* address, uint64_t size, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kmlock_internal(address, size, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}



//
// 324: sys_mlockall
//
int kmlockall(int how)
{
	return kmlockall_t(how, curthread);
}

int kmlockall_internal(int how, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mlockall = (int(*)(struct thread*, struct mlockall_args*))sysents[SYS_MLOCKALL].sy_call;
	if (!sys_mlockall)
		return -1;

	int error;
	struct mlockall_args uap;

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

int kmlockall_t(int how, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kmlockall_internal(how, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

//
// 477: sys_mmap
//
caddr_t __attribute__((noinline)) kmmap_internal(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mmap = (int(*)(struct thread*, struct mmap_args*))sysents[SYS_MMAP].sy_call;
	if (!sys_mmap)
		return (caddr_t)-1;

	int error;
	struct mmap_args uap;

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

caddr_t kmmap_t(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos, struct thread* td)
{
	int64_t ret = (-EIO);
	int retry = 0;

	for (;;)
	{
		ret = (int64_t)kmmap_internal(addr, len, prot, flags, fd, pos, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return (caddr_t)ret;
		}

		break;
	}

	return (caddr_t)ret;
}

caddr_t kmmap(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos)
{
	return kmmap_t(addr, len, prot, flags, fd, pos, curthread);
}

//
// 478: sys_lseek
//

off_t klseek_internal(int fd, off_t offset, int whence, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_lseek = (int(*)(struct thread*, struct lseek_args*))(void*)sysents[SYS_LSEEK].sy_call;
	
	if (!sys_lseek)
		return -1;

	int error;
	struct lseek_args uap;

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

off_t klseek_t(int fd, off_t offset, int whence, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = klseek_internal(fd, offset, whence, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

off_t klseek(int fd, off_t offset, int whence)
{
	return klseek_t(fd, offset, whence, curthread);
}

//
// 15: sys_chmod
//
int kchmod_internal(const char *path, int mode, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_chmod = (int(*)(struct thread*, struct chmod_args*))sysents[SYS_CHMOD].sy_call;
	if (!sys_chmod)
		return -1;

	int error;
	struct chmod_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = (char*)path;
	uap.mode = mode;
	error = sys_chmod(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kchmod_t(const char *path, int mode, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kchmod_internal(path, mode, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kchmod(const char *path, int mode)
{
	return kchmod_t(path, mode, curthread);
}

//
// 73: sys_munmap
//
int kmunmap_internal(void *addr, size_t len, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_munmap = (int(*)(struct thread*, struct munmap_args*))sysents[SYS_MUNMAP].sy_call;
	if (!sys_munmap)
		return -1;

	int error;
	struct munmap_args uap;

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

int kmunmap_t(void *addr, size_t len, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kmunmap_internal(addr, len, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kmunmap(void *addr, size_t len)
{
	return kmunmap_t(addr, len, curthread);
}

//
// 3: sys_read
// 
ssize_t kread_internal(int fd, void* buf, size_t count, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_read = (int(*)(struct thread*, struct read_args*))sysents[SYS_READ].sy_call;
	if (!sys_read)
		return -1;

	int error;
	struct read_args uap;

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

ssize_t kread_t(int fd, void* buf, size_t count, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kread_internal(fd, buf, count, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

ssize_t kread(int fd, void* buf, size_t count)
{
	return kread_t(fd, buf, count, curthread);
}

//
// 189: sys_fstat
//
int kfstat_internal(int fd, struct stat* sb, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_fstat = (int(*)(struct thread*, struct fstat_args*))sysents[SYS_FSTAT].sy_call;
	if (!sys_fstat)
		return -1;

	int error;
	struct fstat_args uap;

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

int kfstat_t(int fd, struct stat* sb, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kfstat_internal(fd, sb, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kfstat(int fd, struct stat* sb)
{
	return kfstat_t(fd, sb, curthread);
}

//
// 188: sys_stat
//

int kstat_internal(char* path, struct stat* buf, struct thread* td)
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

int kstat_t(char* path, struct stat* buf, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kstat_internal(path, buf, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kstat(char* path, struct stat* buf)
{
	return kstat_t(path, buf, curthread);
}

//
// 6: sys_close
//
int kclose_internal(int fd, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_close = (int(*)(struct thread *, struct close_args *))sysents[SYS_CLOSE].sy_call;
	if (!ksys_close)
		return -1337;

	int error;
	struct close_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	error = ksys_close(td, &uap);
	if (error)
		return -error;
	
	return 0;
}

void kclose_t(int fd, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kclose_internal(fd, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return;
		}

		break;
	}

	return;
}

void kclose(int socket)
{
	kclose_t(socket, curthread);
}

//
// 97: sys_socket
//
int ksocket_internal(int a, int b, int c, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_socket = (int(*)(struct thread*, struct socket_args*))sysents[SYS_SOCKET].sy_call;
	
	int error;
	struct socket_args uap;

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

int ksocket_t(int a, int b, int c, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksocket_internal(a, b, c, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int ksocket(int a, int b, int c)
{
	return ksocket_t(a, b, c, curthread);
}

//
// 4: sys_write
//
ssize_t kwrite_internal(int d, const void* buf, size_t nbytes, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_write = (int(*)(struct thread*, struct write_args*))sysents[SYS_WRITE].sy_call;
	if (!ksys_write)
		return -1;

	int error;
	struct write_args uap;
	memset(&uap, 0, sizeof(uap));

	// clear errors
	td->td_retval[0] = 0;
	//const auto off = offsetof(struct thread, td_retval);

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

ssize_t kwrite_t(int d, const void* buf, size_t nbytes, struct thread* td)
{
	ssize_t ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kwrite_internal(d, buf, nbytes, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

ssize_t kwrite(int d, const void* buf, size_t nbytes)
{
	return kwrite_t(d, buf, nbytes, curthread);
}

//
// 272: sys_getdents
//
int kgetdents_internal(int fd, char* buf, int nbytes, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_getdents = (int(*)(struct thread*, struct getdents_args*))sysents[SYS_GETDENTS].sy_call;

	int error;
	struct getdents_args uap;

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

int kgetdents_t(int fd, char* buf, int nbytes, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kgetdents_internal(fd, buf, nbytes, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kgetdents(int fd, char* buf, int nbytes, struct thread* td)
{
	return kgetdents_t(fd, buf, nbytes, curthread);
}

//
// 104: sys_bind
//
int kbind_internal(int socket, const struct sockaddr * b, size_t c, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_bind = (int(*)(struct thread *, struct bind_args *))sysents[SYS_BIND].sy_call;

	int error;
	struct bind_args uap;

	(void)memset(&uap, 0, sizeof(uap));

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

int kbind_t(int socket, const struct sockaddr * b, size_t c, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kbind_internal(socket, b, c, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kbind(int socket, const struct sockaddr * b, size_t c)
{
	return kbind_t(socket, b, c, curthread);
}

//
// 106: sys_listen
//
int klisten_internal(int sockfd, int backlog, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_listen = (int(*)(struct thread *, struct listen_args *))sysents[SYS_LISTEN].sy_call;
	int error;
	struct listen_args uap;

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

int klisten_t(int sockfd, int backlog, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = klisten_internal(sockfd, backlog, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int klisten(int sockfd, int backlog)
{
	return klisten_t(sockfd, backlog, curthread);
}

//
// 30: sys_accept
//

int kaccept_internal(int sock, struct sockaddr * b, size_t* c, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_accept = (int(*)(struct thread *, struct accept_args *))sysents[SYS_ACCEPT].sy_call;
	int error;
	struct accept_args uap;

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

int kaccept_t(int sock, struct sockaddr * b, size_t* c, struct thread* td)
{
	int retry = 0;
	int ret = -EIO;

	for (;;)
	{
		ret = kaccept_internal(sock, b, c, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kaccept(int sock, struct sockaddr * b, size_t* c)
{
	return kaccept_t(sock, b, c, curthread);
}

//
// 29: sys_recvfrom
int krecv_internal(int s, void * buf, int len, int flags, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_recvfrom = (int(*)(struct thread *, struct recvfrom_args *))sysents[SYS_RECVFROM].sy_call;
	if (!ksys_recvfrom)
		return -1;

	int error;
	struct recvfrom_args uap;

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

int krecv_t(int s, void * buf, int len, int flags, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = krecv_internal(s, buf, len, flags, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int krecv(int s, void * buf, int len, int flags)
{
	return krecv_t(s, buf, len, flags, curthread);
}

//
// 133: sys_sendto
//
int ksend_internal(int socket, caddr_t buf, size_t len, int flags, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_sendto = (int(*)(struct thread *, struct sendto_args *))sysents[SYS_SENDTO].sy_call;
	if (!ksys_sendto)
		return -1;

	int error;
	struct sendto_args uap;

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

int ksend_t(int socket, caddr_t buf, size_t len, int flags, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksend_internal(socket, buf, len, flags, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int ksend(int socket, caddr_t buf, size_t len, int flags)
{
	return ksend_t(socket, buf, len, flags, curthread);
}

//
// 5: sys_open
//
int kopen_internal(const char* path, int flags, int mode, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto ksys_open = (int(*)(struct thread *, struct open_args *))sysents[SYS_OPEN].sy_call;

	int error;
	struct open_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = const_cast<char*>(path);
	uap.flags = flags;
	uap.mode = mode;

	error = ksys_open(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kopen_t(const char* path, int flags, int mode, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kopen_internal(path, flags, mode, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kopen(const char* path, int flags, int mode)
{
	return kopen_t(path, flags, mode, curthread);
}

//
// 90: sys_dup2
//
int kdup2_internal(int oldd, int newd, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_dup2 = (int(*)(struct thread *, struct dup2_args *))sysents[SYS_DUP2].sy_call;

	int error;
	struct dup2_args uap;

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

int kdup2_t(int oldd, int newd, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kdup2_internal(oldd, newd, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kdup2(int oldd, int newd)
{
	return kdup2_t(oldd, newd, curthread);
}

//
// 136: sys_mkdir
//
int kmkdir_internal(char * path, int mode, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mkdir = (int (*)(struct thread*, struct mkdir_args*))sysents[SYS_MKDIR].sy_call;
	
	int error;
	struct mkdir_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.mode = mode;

	error = sys_mkdir(td, &uap);
	if (error)
		return -error;

	// success
	return td->td_retval[0];
}

int kmkdir_t(char * path, int mode, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kmkdir_internal(path, mode, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kmkdir(char * path, int mode)
{
	return kmkdir_t(path, mode, curthread);
}

//
// 137: sys_rmdir
//
int krmdir_internal(char * path, struct thread* td)
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

int krmdir_t(char * path, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = krmdir_internal(path, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int krmdir(char * path)
{
	return krmdir_t(path, curthread);
}

//
// 134: sys_shutdown
//
int kshutdown_internal(int s, int how, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_shutdown = (int(*)(struct thread *, struct shutdown_args *))sysents[SYS_SHUTDOWN].sy_call;

	int error;
	struct shutdown_args uap;

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

int kshutdown_t(int s, int how, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kshutdown_internal(s, how, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kshutdown(int s, int how)
{
	return kshutdown_t(s, how, curthread);
}

//
// 10: sys_unlink
//
int kunlink_internal(char* path, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_unlink = (int(*)(struct thread *, struct unlink_args *))sysents[SYS_UNLINK].sy_call;

	int error;
	struct unlink_args uap;

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

int kunlink_t(char* path, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kunlink_internal(path, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kunlink(char* path)
{
	return kunlink_t(path, curthread);
}

//
// 23: sys_setuid
//
int ksetuid_internal(uid_t uid, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_setuid = (int(*)(struct thread *, struct setuid_args *))sysents[SYS_SETUID].sy_call;

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

int ksetuid_t(uid_t uid, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksetuid_internal(uid, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int ksetuid(uid_t uid)
{
	return ksetuid_t(uid, curthread);
}

//
// 26: sys_ptrace
//
int kptrace_internal(int req, pid_t pid, caddr_t addr, int data, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_ptrace = (int(*)(struct thread *, struct ptrace_args *))sysents[SYS_PTRACE].sy_call;

	int error;
	struct ptrace_args uap;

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

int kptrace_t(int req, pid_t pid, caddr_t addr, int data, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kptrace_internal(req, pid, addr, data, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kptrace(int req, pid_t pid, caddr_t addr, int data)
{
	return kptrace_t(req, pid, addr, data, curthread);
}

//
// 37: sys_kill
//
int kkill_internal(int pid, int signum, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_kill = (int(*)(struct thread *, struct kill_args *))sysents[SYS_KILL].sy_call;

	int error;
	struct kill_args uap;

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

int kkill_t(int pid, int signum, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kkill_internal(pid, signum, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kkill(int pid, int signum)
{
	return kkill_t(pid, signum, curthread);
}

//
// 105: sys_setsockopt
//
int ksetsockopt_internal(int socket, int level, int name, caddr_t val, int valsize, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_setsockopt = (int(*)(struct thread *, struct setsockopt_args *))sysents[SYS_SETSOCKOPT].sy_call;

	int error;
	struct setsockopt_args uap;

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

int ksetsockopt_t(int socket, int level, int name, caddr_t val, int valsize, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksetsockopt_internal(socket, level, name, val, valsize, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int ksetsockopt(int socket, int level, int name, caddr_t val, int valsize)
{
	return ksetsockopt_t(socket, level, name, val, valsize, curthread);
}

//
// 480: sys_ftruncate
//
int kftruncate_internal(int fd, off_t length, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_ftruncate = (int(*)(struct thread *, struct ftruncate_args *))sysents[SYS_FTRUNCATE].sy_call;

	int error;
	struct ftruncate_args uap;

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

int kftruncate_t(int fd, off_t length, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kftruncate_internal(fd, length, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kftruncate(int fd, off_t length)
{
	return kftruncate_t(fd, length, curthread);
}

//
// 251: sys_rfork
//
pid_t krfork_internal(int flags, struct thread* td)
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

pid_t krfork_t(int flags, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = krfork_internal(flags, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

pid_t krfork(int flags, struct thread* td)
{
	return krfork_t(flags, curthread);
}

//
// 55: sys_reboot
//
int kreboot_internal(int opt, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_reboot = (int(*)(struct thread*, struct reboot_args*))sysents[SYS_REBOOT].sy_call;
	if (!sys_reboot)
		return -1;

	int error;
	struct reboot_args uap;

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

int kreboot_t(int opt, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kreboot_internal(opt, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kreboot(int opt)
{
	return kreboot_t(opt, curthread);
}

//
// 74: mprotect
int mprotect_internal(const void* addr, size_t len, int prot, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_mprotect = (int(*)(struct thread*, struct mprotect_args*))sysents[SYS_MPROTECT].sy_call;
	if (!sys_mprotect)
		return -1;

	int error;
	struct mprotect_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.addr = addr;
	uap.len = len;
	uap.prot = prot;
	error = sys_mprotect(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kmprotect_t(void* addr, size_t len, int prot, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = mprotect_internal(addr, len, prot, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kselect_internal(int	nfds, fd_set *readfds, fd_set *writefds, fd_set	*exceptfds, struct	timeval	*timeout, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_select = (int(*)(struct thread*, struct select_args*))sysents[SYS_SELECT].sy_call;
	if (!sys_select)
		return -1;

	int error;
	struct select_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.nd = nfds;
	uap.in = readfds;
	uap.ou = writefds;
	uap.ex = exceptfds;
	uap.tv = timeout;
	error = sys_select(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kselect_t(int	nfds, fd_set *readfds, fd_set *writefds, fd_set	*exceptfds, struct	timeval	*timeout, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kselect_internal(nfds, readfds, writefds, exceptfds, timeout, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kioctl_internal(int fd, u_long com, caddr_t data, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_ioctl = (int(*)(struct thread*, struct ioctl_args*))sysents[SYS_IOCTL].sy_call;
	if (!sys_ioctl)
		return -1;

	int error;
	struct ioctl_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd = fd;
	uap.com = com;
	uap.data = data;

	error = sys_ioctl(td, &uap);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	printf("err: (%d), retval: (%lld)\n", error, td->td_retval[0]);

	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kioctl_t(int fd, u_long com, caddr_t data, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kioctl_internal(fd, com, data, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}


int kdynlib_load_prx_internal(char* path, int32_t flags, int32_t* handle_out, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_dynlib_load_prx = (int(*)(struct thread*, struct dynlib_load_prx_args*))sysents[SYS_DYNLIB_LOAD_PRX].sy_call;
	if (!sys_dynlib_load_prx)
		return -1;

	int error;
	struct dynlib_load_prx_args uap;
	uap.prx_path = path;
	uap.flags = flags;
	uap.handle_out = handle_out;
	uap.unk = 0;

	error = sys_dynlib_load_prx(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kdynlib_load_prx_t(char* path, int32_t flags, int32_t* handle_out, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kdynlib_load_prx_internal(path, flags, handle_out, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

// int kdynlib_unload_prx_internal(int32_t handle, struct thread* td)
// {
// 	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
// 	struct sysent* sysents = sv->sv_table;
// 	auto sys_dynlib_load_prx = (int(*)(struct thread*, struct dynlib_load_prx_args*))sysents[SYS_DYNLIB_LOAD_PRX].sy_call;
// 	if (!sys_dynlib_load_prx)
// 		return -1;

// 	int error;
// 	struct dynlib_load_prx_args uap;
// 	uap.prx_path = path;
// 	uap.flags = flags;
// 	uap.handle_out = handle_out;
// 	uap.unk = 0;

// 	error = sys_dynlib_load_prx(td, &uap);
// 	if (error)
// 		return -error;

// 	return td->td_retval[0];
// }

int kdynlib_get_obj_member_internal(uint32_t handle, uint32_t index, uint64_t value, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_dynlib_get_obj_member = (int(*)(struct thread*, struct dynlib_get_obj_member*))sysents[SYS_DYNLIB_GET_OBJ_MEMBER].sy_call;
	if (!sys_dynlib_get_obj_member)
		return -1;

	int error;
	struct dynlib_get_obj_member uap;
	uap.handle = handle;
	uap.index = index;
	uap.value = value;

	error = sys_dynlib_get_obj_member(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kdynlib_get_obj_member_t(uint32_t handle, uint32_t index, void** value, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kdynlib_get_obj_member_internal(handle, index, (uint64_t)value, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kunmount(char* path, int flags, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_unmount = (int(*)(struct thread*, struct unmount_args*))sysents[SYS_UNMOUNT].sy_call;
	if (!sys_unmount)
		return -1;

	int error;
	struct unmount_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.flags = flags;

	error = sys_unmount(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kunmount_t(char* path, int flags, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kunmount(path, flags, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int knmount(struct iovec* iov, int iovlen, unsigned int flags, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_nmount = (int(*)(struct thread*, struct nmount_args*))sysents[SYS_NMOUNT].sy_call;
	if (!sys_nmount)
		return -1;

	int error;
	struct nmount_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.iovp = iov;
	uap.iovcnt = iovlen;
	uap.flags = flags;

	error = sys_nmount(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int knmount_t(struct iovec* iov, int iovlen, unsigned int flags, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = knmount(iov, iovlen, flags, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int klink(const	char *path, const char	*link, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_link = (int(*)(struct thread*, struct link_args*))sysents[SYS_LINK].sy_call;
	if (!sys_link)
		return -1;

	int error;
	struct link_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = (char*)path;
	uap.link = (char*)link;

	error = sys_link(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int klink_t(const char *path, const char *link, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = klink(path, link, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}


int klinkat(int	fd1, const char	*path1,	int fd2, const char *path2, int	flag, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_linkat = (int(*)(struct thread*, struct linkat_args*))sysents[SYS_LINKAT].sy_call;
	if (!sys_linkat)
		return -1;

	int error;
	struct linkat_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.fd1 = fd1;
	uap.path1 = (char*)path1;
	uap.fd2 = fd2;
	uap.path2 = (char*)path2;
	uap.flag = flag;

	error = sys_linkat(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int klinkat_t(int fd1, const char *path1, int fd2, const char *path2, int flag, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = klinkat(fd1, path1, fd2, path2, flag, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int ksandbox_path_internal(char* path, struct thread* td) {
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_sandbox_path = (int(*)(struct thread*, struct sandbox_path_args*))sysents[SYS_SANDBOX_PATH].sy_call;
	if (!sys_sandbox_path)
		return -1;

	int error;
	struct sandbox_path_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;

	error = sys_sandbox_path(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int ksandbox_path_t(char* path, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksandbox_path_internal(path, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kshm_open_internal(const char * path, int flags, mode_t mode, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_shm_open = (int(*)(struct thread*, struct shm_open_args*))sysents[SYS_SHM_OPEN].sy_call;
	if (!sys_shm_open)
		return -1;

	int error;
	struct shm_open_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = path;
	uap.flags = flags;
	uap.mode = mode;

	error = sys_shm_open(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kshm_open_t(const char * path, int flags, mode_t mode, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kshm_open_internal(path, flags, mode, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kshm_unlink_internal(const char * name, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_shm_unlink = (int(*)(struct thread*, struct shm_unlink_args*))sysents[SYS_SHM_UNLINK].sy_call;
	if (!sys_shm_unlink)
		return -1;

	int error;
	struct shm_unlink_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.path = name;

	error = sys_shm_unlink(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kshm_unlink_t(const char * name, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kshm_unlink_internal(name, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kdynlib_dlsym_internal(int64_t p_PrxId, const char* p_FunctionName, void* p_DestinationFunctionOffset, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_dynlib_dlsym = (int(*)(struct thread*, struct dynlib_dlsym_args*))sysents[SYS_DYNLIB_DLSYM].sy_call;
	if (!sys_dynlib_dlsym)
		return -1;

	int error;
	struct dynlib_dlsym_args uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.handle = p_PrxId;
	uap.symbol = p_FunctionName;
	uap.address_out = (void**)p_DestinationFunctionOffset;

	error = sys_dynlib_dlsym(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int kdynlib_dlsym_t(int64_t p_PrxId, const char* p_FunctionName, void* p_DestinationFunctionOffset, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kdynlib_dlsym_internal(p_PrxId, p_FunctionName, p_DestinationFunctionOffset, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

int kthr_create_internal(void* stack_base, size_t* stack_size, void*(*start_func)(void*), void* arg, long flags, long* new_thread_ID, struct thread* td)
{
	return -1;
	// auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	// struct sysent* sysents = sv->sv_table;
	// auto sys_dynlib_dlsym = (int(*)(struct thread*, struct dynlib_dlsym_args*))sysents[SYS_DYNLIB_DLSYM].sy_call;
	// if (!sys_dynlib_dlsym)
	// 	return -1;

	// int error;
	// struct thr_create_args uap;

	// // clear errors
	// td->td_retval[0] = 0;

	// // call syscall
	// uap.ctx
	// error = sys_dynlib_dlsym(td, &uap);
	// if (error)
	// 	return -error;

	// // return socket
	// return td->td_retval[0];
}

int kthr_create_t(void* stack_base, size_t* stack_size, void*(*start_func)(void*), void* arg, long flags, long* new_thread_ID, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = kthr_create_internal(stack_base, stack_size, start_func, arg, flags, new_thread_ID, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}

struct ps4gdb_sys_gen_uap{
	uint64_t p1;
	uint64_t p2;
	uint64_t p3;
	uint64_t p4;
	uint64_t p5;
	uint64_t p6;
	uint64_t p7;
};

int ksysctl_internal(int *name, uint32_t namelen, void *old, size_t *oldlenp, void *newp, size_t newlen, struct thread* td)
{
	auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
	struct sysent* sysents = sv->sv_table;
	auto sys_sysctl = (int(*)(struct thread*, struct ps4gdb_sys_gen_uap*))sysents[SYS___SYSCTL].sy_call;
	if (!sys_sysctl)
		return -1;

	int error;
	struct ps4gdb_sys_gen_uap uap;

	// clear errors
	td->td_retval[0] = 0;

	// call syscall
	uap.p1 = (uint64_t)name;
	uap.p2 = (uint64_t)namelen;
    uap.p3 = (uint64_t)old;
	uap.p4 = (uint64_t)oldlenp;
    uap.p5 = (uint64_t)newp;
	uap.p6 = (uint64_t)newlen;

	error = sys_sysctl(td, &uap);
	if (error)
		return -error;

	// return socket
	return td->td_retval[0];
}

int ksysctl_t(int *name, uint32_t namelen, void *old, size_t *oldlenp, void *newp, size_t newlen, struct thread* td)
{
	int ret = -EIO;
	int retry = 0;

	for (;;)
	{
		ret = ksysctl_internal(name, namelen, old, oldlenp, newp, newlen, td);
		if (ret < 0)
		{
			if (ret == -EINTR)
			{
				if (retry > MaxInterruptRetries)
					break;
					
				retry++;
				continue;
			}
			
			return ret;
		}

		break;
	}

	return ret;
}
