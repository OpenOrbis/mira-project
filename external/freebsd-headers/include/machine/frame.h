/*-
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
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
 *	from: @(#)frame.h	5.2 (Berkeley) 1/18/91
 * $FreeBSD: release/9.0.0/sys/amd64/include/frame.h 190620 2009-04-01 13:09:26Z kib $
 */

#ifndef _MACHINE_FRAME_H_
#define _MACHINE_FRAME_H_ 1

/*
 * System stack frames.
 */

/*
 * Exception/Trap Stack Frame
 *
 * The ordering of this is specifically so that we can take first 6
 * the syscall arguments directly from the beginning of the frame.
 */

#ifndef MIRA_PLATFORM
struct trapframe {
	register_t	tf_rdi;
	register_t	tf_rsi;
	register_t	tf_rdx;
	register_t	tf_rcx;
	register_t	tf_r8;
	register_t	tf_r9;
	register_t	tf_rax;
	register_t	tf_rbx;
	register_t	tf_rbp;
	register_t	tf_r10;
	register_t	tf_r11;
	register_t	tf_r12;
	register_t	tf_r13;
	register_t	tf_r14;
	register_t	tf_r15;
	uint32_t	tf_trapno;
	uint16_t	tf_fs;
	uint16_t	tf_gs;
	register_t	tf_addr;
	uint32_t	tf_flags;
	uint16_t	tf_es;
	uint16_t	tf_ds;
	/* below portion defined in hardware */
	register_t	tf_err;
	register_t	tf_rip;
	register_t	tf_cs;
	register_t	tf_rflags;
	register_t	tf_rsp;
	register_t	tf_ss;
};
#else
struct trapframe
{
	register_t tf_rdi;			// 0x00
	register_t tf_rsi;			// 0x08
	register_t tf_rdx;			// 0x10
	register_t tf_rcx;			// 0x18
	register_t tf_r8;			// 0x20
	register_t tf_r9;			// 0x28
	register_t tf_rax;			// 0x30
	register_t tf_rbx;			// 0x38
	register_t tf_rbp;			// 0x40
	register_t tf_r10;			// 0x48
	register_t tf_r11;			// 0x50
	register_t tf_r12;			// 0x58
	register_t tf_r13;			// 0x60
	register_t tf_r14;			// 0x68
	register_t tf_r15;			// 0x70
	uint32_t tf_trapno;			// 0x78
	uint16_t tf_fs;				// 0x7C
	uint16_t tf_gs;				// 0x7E
	register_t tf_addr;			// 0x80
	uint32_t tf_flags;			// 0x88
	uint16_t tf_es;				// 0x8C
	uint16_t tf_ds;				// 0x8E

	register_t tf_last_branch_from;		// 0x90
	register_t tf_last_branch_to;		// 0x98

	/* below portion defined in hardware */
	register_t tf_err;			// 0xA0
	register_t tf_rip;			// 0xA8
	register_t tf_cs;			// 0xB0
	register_t tf_rflags;			// 0xB8
	register_t tf_rsp;			// 0xC0
	register_t tf_ss;			// 0xC8
};
#endif

#define	TF_HASSEGS	0x1
/* #define	_MC_HASBASES	0x2 */

#endif /* _MACHINE_FRAME_H_ */
