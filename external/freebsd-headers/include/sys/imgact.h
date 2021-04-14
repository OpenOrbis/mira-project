/*-
 * Copyright (c) 1993, David Greenman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: release/9.0.0/sys/sys/imgact.h 217151 2011-01-08 16:13:44Z kib $
 */

#ifndef _SYS_IMGACT_H_
#define	_SYS_IMGACT_H_

#include <sys/uio.h>
#include <sys/types.h>
#include <vm/vm.h>

#define MAXSHELLCMDLEN	PAGE_SIZE

struct image_args {
	char *buf;		/* pointer to string buffer */
	char *begin_argv;	/* beginning of argv in buf */
	char *begin_envv;	/* beginning of envv in buf */
	char *endp;		/* current `end' pointer of arg & env strings */
	char *fname;            /* pointer to filename of executable (system space) */
	char *fname_buf;	/* pointer to optional malloc(M_TEMP) buffer */
	int stringspace;	/* space left in arg & env buffer */
	int argc;		/* count of argument strings */
	int envc;		/* count of environment strings */
	int fd;			/* file descriptor of the executable */
};

struct image_params {
	struct proc *proc;	/* our process struct */
	struct label *execlabel;	/* optional exec label */
	struct vnode *vp;	/* pointer to vnode of file to exec */
	struct vm_object *object;	/* The vm object for this vp */
	struct vattr *attr;	/* attributes of file */
	const char *image_header; /* head of file to exec */
	unsigned long entry_addr; /* entry address of target executable */
	unsigned long reloc_base; /* load address of image */
	char vmspace_destroyed;	/* flag - we've blown away original vm space */
	char interpreted;	/* flag - this executable is interpreted */
	char opened;		/* flag - we have opened executable vnode */
	char *interpreter_name;	/* name of the interpreter */
	void *auxargs;		/* ELF Auxinfo structure pointer */
	struct sf_buf *firstpage;	/* first page that we mapped */
	unsigned long ps_strings; /* PS_STRINGS for BSD/OS binaries */
	size_t auxarg_size;
	struct image_args *args;	/* system call arguments */
	struct sysentvec *sysent;	/* system entry vector */
	char *execpath;
	unsigned long execpathp;
	char *freepath;
	unsigned long canary;
	int canarylen;
	unsigned long pagesizes;
	int pagesizeslen;
	vm_prot_t stack_prot;

	// PlayStation 4 Specific (Credits: ChendoChap)
	uint64_t dynamic_addr;
	uint64_t tls_mem_size;
	uint64_t tls_align;
	uint64_t tls_file_size;
	uint64_t tls_addr;
	uint64_t gnu_eh_frame_addr;
	uint64_t gnu_eh_frame_mem_size;
	uint8_t unkF0[0x88];
	//SelfAuthInfo unkF0; //sce custom section of ucred
	uint64_t unk178;
	uint64_t sce_procparam_addr;
	uint64_t sce_procparam_file_size;
	uint64_t sce_moduleparam_addr;
	uint64_t sce_moduleparam_file_size;

	uint32_t dynamic_phdr_index;
	uint64_t dynamic_file_offset;
	uint64_t dynamic_file_size;

	uint32_t sce_dynlibdata_phdr_index;
	uint64_t sce_dynlibdata_file_offset;
	uint64_t sce_dynlibdata_file_size;
	uint32_t sce_comment_phdr_index;
	uint64_t sce_comment_file_offset;
	uint64_t sce_comment_file_size;
	vm_object_t self_pager;
	char* execpath2;
	uint64_t load_start_address; //PT_LOAD min?
	uint64_t load_end_address;  //PT_LOAD max?
	uint32_t has_dynamic; //unsure if right term, set to 1 when it find PT_DYNAMIC
	uint32_t unk20C;
	uint32_t unk210;
	uint64_t sce_relro_addr;
	uint64_t sce_relro_size;
	uint32_t budget_ptype;
	uint16_t elf_type; //22C like the elf header type
	//2 bytes implicit padding?
};

#if defined(_KERNEL)
static_assert(offsetof(struct image_params, unkF0) == 0xF0, "unkF0 invalid offset");
static_assert(offsetof(struct image_params, unk178) == 0x178, "unk178 invalid offset");
#endif

#ifdef _KERNEL
struct sysentvec;
struct thread;

#define IMGACT_CORE_COMPRESS	0x01

int	exec_alloc_args(struct image_args *);
int	exec_check_permissions(struct image_params *);
register_t *exec_copyout_strings(struct image_params *);
void	exec_free_args(struct image_args *);
int	exec_new_vmspace(struct image_params *, struct sysentvec *);
void	exec_setregs(struct thread *, struct image_params *, u_long);
int	exec_shell_imgact(struct image_params *);
int	exec_copyin_args(struct image_args *, char *, enum uio_seg,
	char **, char **);
#endif

#endif /* !_SYS_IMGACT_H_ */
