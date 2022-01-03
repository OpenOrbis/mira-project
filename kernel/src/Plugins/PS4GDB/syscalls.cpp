extern "C"
{
	#include <sys/param.h>
	#include <sys/types.h>
	#include <sys/filedesc.h>
	#include <sys/stdint.h>
	#include <sys/fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/mman.h>
	#include <sys/unistd.h>
	#include <vm/vm.h>
	#include <sys/malloc.h>
	#include <sys/errno.h>
	#include <sys/sx.h>
	#include <sys/sysproto.h>
	#include <sys/proc.h>
}

#include "kdl.h"
#include "log.h"
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>

void ps4gdb_sys_memset(void *buff, uint8_t value, int len)
{
	uint8_t *tmp = (uint8_t*)buff;

	int i = 0;
	for(i = 0; i < len; i++)
		tmp[i] = value;

	return;
}

int ps4gdb_sys_socket(int domain, int type, int protocol){
	return ksocket_t(domain, type, protocol, curthread);
}

int ps4gdb_sys_bind(int s, int port){
	PS4GDB_kbzero();
	
	struct sockaddr_in sockadr;
	kbzero(&sockadr, sizeof(sockadr));
	sockadr.sin_family = AF_INET;
	sockadr.sin_port = htons(port);
	sockadr.sin_addr.s_addr = INADDR_ANY;

	return kbind_t(s, (sockaddr*)&sockadr, sizeof(sockadr), curthread);
}

int ps4gdb_sys_listen(int s, int backlog){
	return klisten_t(s, backlog, curthread);
}

int ps4gdb_sys_accept(int s, struct sockaddr *name, int namelen){
	return kaccept_t(s, name, (size_t*)&namelen, curthread);
}

int ps4gdb_sys_shutdown(int fd, int how){
	return kshutdown_t(fd, how, curthread);
}

int ps4gdb_sys_close(int fd){
	kclose_t(fd, curthread);
	return 0;
}

int ps4gdb_sys_read(int fd, void *addr, size_t len){
	return kread_t(fd, addr, len, curthread);
}

void* ps4gdb_sys_mmap(void *addr, size_t len, int prot, int flags, int fd, uint64_t pos){
	return kmmap_t((caddr_t)addr, len, prot, flags, fd, pos, curthread);
}

void* ps4gdb_sys_munmap(void *addr, size_t len){
	kmunmap_t((caddr_t)addr, len, curthread);
	return (void*)0;
	
}

int ps4gdb_sys_write(int fd, void *addr, int len){
	return kwrite_t(fd, addr, len, curthread);
}

int ps4gdb_sys_open(char *path, int flags, int mode){
	return kopen_t(path, mode, flags, curthread);
}

int ps4gdb_sys_getpid(){
	PS4GDB_ksys_getpid();

	struct thread *td = curthread;
	td->td_retval[0] = 0;
	ksys_getpid(td);
	return td->td_retval[0];
}

int ps4gdb_sys_ptrace(int req, pid_t pid, void *addr, int data){
	return kptrace_t(req, pid, (caddr_t)addr, data, curthread);
}

int ps4gdb_sys_wait4(int wpid, int *status, int	options, void *rusage2){
	return kwait4_t(wpid, status, options, (rusage*)rusage2, curthread);
}

int ps4gdb_sys_sysctl(int *name, uint32_t namelen, void *old, size_t *oldlenp, void *newaddr, size_t newlen){
    PS4GDB_ksys_sysctl();
	PS4GDB_kmalloc();
	PS4GDB_kfree();
	PS4GDB_kM_TEMP();

    // alloc and prepare uap
	struct ps4gdb_sys_gen_uap *uap = (struct ps4gdb_sys_gen_uap*)kmalloc(sizeof(struct ps4gdb_sys_gen_uap),kM_TEMP,0x102);
	ps4gdb_sys_memset(uap,0x00, sizeof(struct ps4gdb_sys_gen_uap));
    uap->p1 = (uint64_t)name;
	uap->p2 = (uint64_t)namelen;
    uap->p3 = (uint64_t)old;
	uap->p4 = (uint64_t)oldlenp;
    uap->p5 = (uint64_t)newaddr;
	uap->p6 = (uint64_t)newlen;

	struct thread *td = curthread;
	td->td_retval[0] = 0;

	int error = ksys_sysctl(td,uap);
	kfree((void*)uap,kM_TEMP);

	if(error){
		LOG_ERROR("sys_sysctl returned %d\n",error);
		td->td_retval[0] = -1;
	}

	return td->td_retval[0];
}

