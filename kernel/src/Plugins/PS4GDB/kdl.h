#pragma once

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
#include <Utils/Kdlsym.hpp>

struct ps4gdb_sys_gen_uap{
	uint64_t p1;
	uint64_t p2;
	uint64_t p3;
	uint64_t p4;
	uint64_t p5;
	uint64_t p6;
	uint64_t p7;
};

#define PS4GDB_kprintf() auto kprintf = (void(*)(const char *format, ...))kdlsym(printf);
#define PS4GDB_kbzero() auto kbzero = (void(*)(void *ptr,int count))kdlsym(bzero);
#define PS4GDB_ksys_socket() auto ksys_socket = (int(*)(struct thread *, struct socket_args *))kdlsym(sys_socket);
#define PS4GDB_ksys_bind() auto ksys_bind = (int(*)(struct thread *, struct bind_args *))kdlsym(sys_bind);
#define PS4GDB_ksys_listen() auto ksys_listen = (int(*)(struct thread *, struct listen_args *))kdlsym(sys_listen);
#define PS4GDB_ksys_accept() auto ksys_accept = (int(*)(struct thread *, struct accept_args *))kdlsym(sys_accept);
#define PS4GDB_ksys_close() auto ksys_close = (int(*)(struct thread *, struct close_args *))kdlsym(sys_close);
#define PS4GDB_kkthread_add() auto kkthread_add = (int(*)(void (*func)(void *), void *arg, struct proc *procp,struct thread **newtdpp, int flags, int pages,	const char *fmt, ...))kdlsym(kthread_add);
#define PS4GDB_ksys_read() auto ksys_read = (int(*)(struct thread *, struct read_args *))kdlsym(sys_read);
#define PS4GDB_ksys_write() auto ksys_write = (int(*)(struct thread *, struct write_args *))kdlsym(sys_write);
#define PS4GDB_kM_TEMP() struct malloc_type *kM_TEMP = (struct malloc_type *)kdlsym(M_TEMP);
#define PS4GDB_kmalloc() auto kmalloc = (void*(*)(unsigned long size, struct malloc_type *type, int flags))kdlsym(malloc);
#define PS4GDB_kfree() auto kfree = (void(*)(void *addr, struct malloc_type *type))kdlsym(free);
#define PS4GDB_ksys_open() auto ksys_open = (int (*)(struct thread *, struct open_args *))kdlsym(sys_open);
#define PS4GDB_ksys_getpid() auto ksys_getpid = (int (*)(struct thread *))kdlsym(sys_getpid);
#define PS4GDB_kkproc_exit() auto kkproc_exit = (int(*)(int code))kdlsym(kproc_exit);
#define PS4GDB_ksys_mmap() auto ksys_mmap = (void*(*)(struct thread *, struct mmap_args *))kdlsym(sys_mmap);
#define PS4GDB_ksys_munmap() auto ksys_munmap = (void*(*)(struct thread *, struct munmap_args *))kdlsym(sys_munmap);
#define PS4GDB_kkthread_exit() auto kkthread_exit = (void(*)())kdlsym(kthread_exit);
#define PS4GDB_kvmspace_alloc() auto kvmspace_alloc = (struct vmspace*(*)(vm_offset_t min, vm_offset_t max))kdlsym(vmspace_alloc);
#define PS4GDB_kpmap_activate() auto kpmap_activate = (void(*)(struct thread *td))kdlsym(pmap_activate);
#define PS4GDB_kstrlen() auto kstrlen = (int(*)(const char *str))kdlsym(strlen);
#define PS4GDB_kstrcpy() auto kstrcpy = (char*(*)( char *destination, const char *source))kdlsym(strcpy);
#define PS4GDB_kcopyout() auto kcopyout = (int(*)(const void *kaddr, void *uaddr, size_t len))kdlsym(copyout);
#define PS4GDB_ksys_shutdown() auto ksys_shutdown = (int(*)(struct thread *td,struct ps4gdb_sys_gen_uap *uap))kdlsym(sys_shutdown);
#define PS4GDB_ksys_ptrace() auto ksys_ptrace = (int(*)(struct thread *td,struct ps4gdb_sys_gen_uap *uap))kdlsym(sys_ptrace);
#define PS4GDB_ksys_wait4() auto ksys_wait4 = (int(*)(struct thread *td,struct ps4gdb_sys_gen_uap *uap))kdlsym(sys_wait4);
#define PS4GDB_ksys_sysctl() auto ksys_sysctl = (int(*)(struct thread *td,struct ps4gdb_sys_gen_uap *uap))kdlsym(sys_sysctl);
#define PS4GDB_ksnprintf() auto ksnprintf = (int(*)( char * s, size_t n, const char * format, ... ))kdlsym(snprintf);
#define PS4GDB_kavcontrol_sleep() auto kavcontrol_sleep = (int(*)(int amount))kdlsym(avcontrol_sleep);

