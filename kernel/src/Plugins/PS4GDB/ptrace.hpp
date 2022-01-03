extern "C"
{
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/_timespec.h>
#include <sys/resource.h>
#include <sys/ptrace.h>
}
 
int ptrace_attach(int pid);
int ptrace_detach(int pid);
int ptrace_continue(int pid);
int ptrace_get_regs(int pid, void *addr);
int ptrace_set_regs(int pid, void *addr);int ptrace_step(int pid);
int ptrace_suspend(int pid);
int ptrace_resume(int pid);
int ptrace_io(int pid, int op, void *off, void *addr, unsigned long long len);
int ptrace_setstep(int pid);
int ptrace_clearstep(int pid);

