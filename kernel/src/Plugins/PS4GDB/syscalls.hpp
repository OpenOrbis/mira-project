extern "C"
{
    #include <sys/param.h>
    #include <sys/types.h>
    #include <sys/stdint.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/_timespec.h>
    #include <sys/resource.h>
}

int ps4gdb_sys_socket(int domain, int type, int protocol);
int ps4gdb_sys_bind(int s, int port);
int ps4gdb_sys_listen(int s, int backlog);
int ps4gdb_sys_accept(int s, struct sockaddr *name, int namelen);
int ps4gdb_sys_close(int fd);
int ps4gdb_sys_read(int fd, void *addr, size_t len);
int ps4gdb_sys_write(int fd, void *addr, int len);
int ps4gdb_sys_getdents(int fd, void *addr, int len);
int ps4gdb_sys_open(char *path, int flags, int mode);
int ps4gdb_sys_getpid();
void *ps4gdb_sys_mmap(void *addr, size_t len, int prot, int flags, int fd, uint64_t pos);
int ps4gdb_sys_munmap(void *addr, size_t len);
int ps4gdb_sys_shutdown(int fd, int how);
int ps4gdb_sys_mkdir(char *path, int mode);
int ps4gdb_sys_rmdir(char *path);
int ps4gdb_sys_stat(const	char * restrictpath, void *sb);
int ps4gdb_sys_ptrace(int req, pid_t pid, void *addr, int data);
int ps4gdb_sys_wait4(int wpid, int *status, int	options, void *rusage);
int ps4gdb_sys_sysctl(int *name, uint32_t namelen, void *old, size_t *oldlenp, void *newaddr, size_t newlen);

