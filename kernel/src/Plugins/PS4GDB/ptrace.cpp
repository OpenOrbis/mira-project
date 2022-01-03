extern "C"
{
	#include <sys/ptrace.h>
}

#include "syscalls.hpp"
#include "log.h"
 
int ptrace_attach(int pid){
    return ps4gdb_sys_ptrace(PT_ATTACH,pid,0,0);
}
 
int ptrace_detach(int pid){
    return ps4gdb_sys_ptrace(PT_DETACH,pid,(void*)SIGCONT,0);
}

int ptrace_continue(int pid){
	return ps4gdb_sys_ptrace(PT_CONTINUE,pid,(void*)1,0);
}

int ptrace_get_regs(int pid, void *addr){
	return ps4gdb_sys_ptrace(PT_GETREGS, pid, addr, 0);
}

int ptrace_set_regs(int pid, void *addr){
	return ps4gdb_sys_ptrace(PT_SETREGS, pid, addr, 0);
}

int ptrace_step(int pid){
	return ps4gdb_sys_ptrace(PT_STEP, pid, (void *)1, 0);
}

int ptrace_suspend(int pid){
	return ps4gdb_sys_ptrace(PT_SUSPEND, pid,(void*)0, 0);
}

int ptrace_resume(int pid){
	return ps4gdb_sys_ptrace(PT_RESUME, pid,(void*)1, 0);
}

int ptrace_setstep(int pid){
	return ps4gdb_sys_ptrace(PT_SETSTEP, pid,(void*)0, 0);
}

int ptrace_clearstep(int pid){
	return ps4gdb_sys_ptrace(PT_CLEARSTEP, pid,(void*)0, 0);
}

void *ptrace_io(int pid, int op, void *off, void *addr, unsigned long long len){
	struct ptrace_io_desc io_desc;
	unsigned long long ret;
	
	io_desc.piod_op = op;
	io_desc.piod_offs = off;
	io_desc.piod_addr = addr;
	io_desc.piod_len = len;
	ret = ps4gdb_sys_ptrace(PT_IO, pid, (void*)&io_desc, 0);
	
    if(ret == 0){
		return (void*)0;
	}
	else{
		return (void*)-1;
	}
}

