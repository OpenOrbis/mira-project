#pragma once
#include <Utils/Types.hpp>

struct task_struct;
struct proc;
struct sockaddr;
struct cdev;
struct cdevsw;
struct ucred;
struct thread;
struct stat;
struct rusage;

// 7
//extern int kwait4(int pid, int *status, int options, struct rusage *rusage);
extern int kwait4_t(int pid, int *status, int options, struct rusage *rusage, struct thread* td);

// 203
//extern int kmlock(void* address, uint64_t size);
extern int kmlock_t(void* address, uint64_t size, struct thread* td);

// 324
//extern int kmlockall(int how);
extern int kmlockall_t(int how, struct thread* td);

// 477
extern caddr_t kmmap_t(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos, struct thread* td);
//extern caddr_t kmmap(caddr_t addr, size_t len, int prot, int flags, int fd, off_t pos);

// 478
extern off_t klseek_t(int fd, off_t offset, int whence, struct thread* td);
//extern off_t klseek(int fd, off_t offset, int whence);

//extern void kclose(int socket);
extern void kclose_t(int socket, struct thread* td);

//extern int ksocket(int, int, int);
extern int ksocket_t(int, int, int, struct thread* td);

//extern int kbind(int, const struct sockaddr*, size_t);
extern int kbind_t(int, const struct sockaddr*, size_t, struct thread* td);

//extern int klisten(int, int);
extern int klisten_t(int, int, struct thread* td);

//extern int kaccept(int, struct sockaddr*, size_t*);
extern int kaccept_t(int, struct sockaddr*, size_t*, struct thread* td);

//extern int krecv(int, void*, int, int);
extern int krecv_t(int, void*, int, int, struct thread* td);

//extern int kopen(const char* path, int flags, int mode);
extern int kopen_t(const char* path, int flags, int mode, struct thread* td);

//extern ssize_t kwrite(int d, const void* buf, size_t nbytes);
extern ssize_t kwrite_t(int d, const void* buf, size_t nbytes, struct thread* td);

//extern int kgetdents(int fd, char* buf, int nbytes);
extern int kgetdents_t(int fd, char* buf, int nbytes, struct thread* td);

//extern ssize_t kread(int fd, void* buf, size_t count);
extern ssize_t kread_t(int fd, void* buf, size_t count, struct thread* td);

//extern int ksend(int socket, caddr_t buf, size_t len, int flags);
extern int ksend_t(int socket, caddr_t buf, size_t len, int flags, struct thread* td);

//extern int kfstat(int fd, struct stat* sb);
extern int kfstat_t(int fd, struct stat* sb, struct thread* td);

//extern int kstat(char* path, struct stat* buf);
extern int kstat_t(char* path, struct stat* buf, struct thread* td);

//extern int kunlink(char* path);
extern int kunlink_t(char* path, struct thread* td);

//extern int ksetuid(uid_t uid);
extern int ksetuid_t(uid_t uid, struct thread* td);

//extern int kptrace(int req, pid_t pid, caddr_t addr, int data);
extern int kptrace_t(int req, pid_t pid, caddr_t addr, int data, struct thread* td);

//extern int kkill(int pid, int signum);
extern int kkill_t(int pid, int signum, struct thread* td);

//extern int kdup2(int oldd, int newd);
extern int kdup2_t(int oldd, int newd, struct thread* td);

//extern int kshutdown(int s, int how);
extern int kshutdown_t(int s, int how, struct thread* td);

//extern int kmkdir(char* path, int mode);
extern int kmkdir_t(char * path, int mode, struct thread* td);

//extern int krmdir(char* path);
extern int krmdir_t(char * path, struct thread* td);

extern int kmunmap_t(void *addr, size_t len, struct thread* td);
//extern int kmunmap(void *addr, size_t len);

//extern int ksetsockopt(int socket, int level, int name, caddr_t val, int valsize);
extern int ksetsockopt_t(int socket, int level, int name, caddr_t val, int valsize, struct thread* td);

//extern int kftruncate(int fd, off_t length);
extern int kftruncate_t(int fd, off_t length, struct thread* td);

//extern pid_t krfork(int flags);
extern pid_t krfork_t(int flags, struct thread* td);

//extern int kreboot(int opt);
extern int kreboot_t(int opt, struct thread* td);