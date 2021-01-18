// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Plugins/Debugging/Debugger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Mira.hpp>

extern "C"
{
	#include <sys/lock.h>
	#include <sys/uio.h>
	#include <sys/proc.h>
	#include <sys/ioccom.h>
	#include <sys/fcntl.h>

	#include <vm/vm.h>
	#include <vm/pmap.h>

	#include <machine/psl.h>
	#include <machine/pmap.h>
	#include <machine/segments.h>
	#include <machine/trap.h>
};

struct amd64_frame 
{
	struct amd64_frame      *f_frame;
	long                    f_retaddr;
	long                    f_arg0;
};

using namespace Mira::Plugins;

void sdtossd(struct user_segment_descriptor *sd, struct soft_segment_descriptor *ssd)
{
	ssd->ssd_base  = (sd->sd_hibase << 24) | sd->sd_lobase;
	ssd->ssd_limit = (sd->sd_hilimit << 16) | sd->sd_lolimit;
	ssd->ssd_type  = sd->sd_type;
	ssd->ssd_dpl   = sd->sd_dpl;
	ssd->ssd_p     = sd->sd_p;
	ssd->ssd_long  = sd->sd_long;
	ssd->ssd_def32 = sd->sd_def32;
	ssd->ssd_gran  = sd->sd_gran;
}

bool Debugger2::IsStackSpace(void* p_Address)
{
	return ((reinterpret_cast<uint64_t>(p_Address) & 0xFFFFFFFF00000000) == 0xFFFFFF8000000000);
}

void Debugger2::OnTrapFatal(struct trapframe* frame, vm_offset_t eva)
{
	int code, ss;
	u_int type;
	long esp;
	struct soft_segment_descriptor softseg;
	const char *msg;

	// Needed for writing to klog
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);

	// Global descriptor tables
	auto gdt = (struct user_segment_descriptor*)kdlsym(gdt);

	// Page faults have to be disabled while getting stack frame information
	auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);

	// Get the trap message based on the trap code
	#define MAX_TRAP_MSG		33

	static const char *trap_msg[] = {
		"",                               /*  0 unused */
		"privileged instruction fault",   /*  1 T_PRIVINFLT */
		"",                               /*  2 unused */
		"breakpoint instruction fault",   /*  3 T_BPTFLT */
		"",                               /*  4 unused */
		"",                               /*  5 unused */
		"arithmetic trap",                /*  6 T_ARITHTRAP */
		"",                               /*  7 unused */
		"",                               /*  8 unused */
		"general protection fault",       /*  9 T_PROTFLT */
		"trace trap",                     /* 10 T_TRCTRAP */
		"",                               /* 11 unused */
		"page fault",                     /* 12 T_PAGEFLT */
		"",                               /* 13 unused */
		"alignment fault",                /* 14 T_ALIGNFLT */
		"",                               /* 15 unused */
		"",                               /* 16 unused */
		"",	                              /* 17 unused */
		"integer divide fault",           /* 18 T_DIVIDE */
		"non-maskable interrupt trap",    /* 19 T_NMI */
		"overflow trap",                  /* 20 T_OFLOW */
		"FPU bounds check fault",         /* 21 T_BOUND */
		"FPU device not available",       /* 22 T_DNA */
		"double fault",                   /* 23 T_DOUBLEFLT */
		"FPU operand fetch fault",        /* 24 T_FPOPFLT */
		"invalid TSS fault",              /* 25 T_TSSFLT */
		"segment not present fault",      /* 26 T_SEGNPFLT */
		"stack fault",                    /* 27 T_STKFLT */
		"machine check trap",             /* 28 T_MCHK */
		"SIMD floating-point exception",  /* 29 T_XMMFLT */
		"reserved (unknown) fault",       /* 30 T_RESERVED */
		"",                               /* 31 unused (reserved) */
		"DTrace pid return trap",         /* 32 T_DTRACE_RET */
		"DTrace fasttrap probe trap",     /* 33 T_DTRACE_PROBE */
	};

	// Get the trap code and trap type based on that code
	code = frame->tf_err;
	type = frame->tf_trapno;

	sdtossd(&gdt[NGDT * PCPU_GET(cpuid) + IDXSEL(frame->tf_cs & 0xffff)], &softseg);

	if (type <= MAX_TRAP_MSG)
		msg = trap_msg[type];
	else
		msg = "UNKNOWN";

	// Print generic crash information
	printf("## Kernel panic: received fatal trap %d (%s) while in %s mode.\n##\n",
		type,
		msg,
		ISPL(frame->tf_cs) == SEL_UPL ? "user" : "kernel"
	);

#ifdef SMP
	// Two separate prints in case of a trap on an unmapped page
	printf("## cpuid = %d; ", PCPU_GET(cpuid));
	printf("## apic id = %02x\n##\n", PCPU_GET(apic_id));
#endif

	if (type == T_PAGEFLT) {
		printf("## fault virtual address: 0x%lx\n", eva);

		printf("## fault code: %s %s %s, %s\n##\n",
			code & PGEX_U ? "user" : "supervisor",
			code & PGEX_W ? "write" : "read",
			code & PGEX_I ? "instruction" : "data",
			code & PGEX_P ? "protection violation" : "page not present"
		);
	}

	printf("## instruction pointer: 0x%lx:0x%lx\n", frame->tf_cs & 0xffff, frame->tf_rip);

	if (ISPL(frame->tf_cs) == SEL_UPL) {
		ss = frame->tf_ss & 0xffff;
		esp = frame->tf_rsp;
	} else {
		ss = GSEL(GDATA_SEL, SEL_KPL);
		esp = (long)&frame->tf_rsp;
	}

	printf("## stack pointer: 0x%x:0x%lx\n", ss, esp);
	printf("## frame pointer: 0x%x:0x%lx\n", ss, frame->tf_rbp);

	printf("## code segment:  base 0x%lx, limit 0x%lx, type 0x%x\n",
		softseg.ssd_base,
		softseg.ssd_limit,
		softseg.ssd_type
	);

	printf("##                DPL %d, pres %d, long %d, def32 %d, gran %d\n",
		softseg.ssd_dpl,
		softseg.ssd_p,
		softseg.ssd_long,
		softseg.ssd_def32,
	  softseg.ssd_gran
	);

	printf("## processor eflags: ");

	if (frame->tf_rflags & PSL_T)
		printf("trace trap, ");
	if (frame->tf_rflags & PSL_I)
		printf("interrupt enabled, ");
	if (frame->tf_rflags & PSL_NT)
		printf("nested task, ");
	if (frame->tf_rflags & PSL_RF)
		printf("resume, ");
	printf("IOPL = %ld\n", (frame->tf_rflags & PSL_IOPL) >> 12);

	printf("## current process: ");

	if (curproc) 
		printf("%lu (%s)\n##\n", (u_long)curproc->p_pid, curthread->td_name);
	else
		printf("Idle\n##\n");

	printf("## kernel base: %p\n", gKernelBase);

	// Print mira specific info
	auto s_Framework = Mira::Framework::GetFramework();

	if(s_Framework)
	{
		auto s_InitParams = s_Framework->GetInitParams();

		if(s_InitParams)
		{
			printf("## mira base:   %p (size: %p)\n", s_InitParams->payloadBase, s_InitParams->payloadSize);
			printf("## mira allocated base: %p (size: %p)\n", s_InitParams->allocatedBase, s_InitParams->payloadSize);
			printf("## mira proc:   %p (entry: %p)\n", s_InitParams->process, s_InitParams->entrypoint);
			printf("## mira entrypoint (mira_entry): %p\n", mira_entry);
			printf("## instruction pointer offset (mira allocated base: %p)\n", frame->tf_rip - (uint64_t)s_InitParams->allocatedBase);
		}

		printf("##     messageManager: %p\n", s_Framework->GetMessageManager());
		printf("##     pluginManager:  %p\n", s_Framework->GetPluginManager());
	}

	printf("##\n");
	
	printf("## instruction pointer offset (kernel: %p) (mira_entry: %p)\n##\n",
		frame->tf_rip - (uint64_t)gKernelBase,
		frame->tf_rip - (uint64_t)mira_entry
	);
	
	printf("## last branch from: %p\n", frame->tf_last_branch_from);
	printf("## last branch to:   %p\n##\n", frame->tf_last_branch_to);

	// Print register state
	printf("## registers:\n");
	printf("## rax: 0x%016llx  rbx: 0x%016llx\n", frame->tf_rax, frame->tf_rbx);
	printf("## rcx: 0x%016llx  rdx: 0x%016llx\n", frame->tf_rcx, frame->tf_rdx);
	printf("## rsi: 0x%016llx  rdi: 0x%016llx\n", frame->tf_rsi, frame->tf_rdi);
	printf("## rbp: 0x%016llx  rsp: 0x%016llx\n", frame->tf_rbp, frame->tf_rsp);
	printf("## r8 : 0x%016llx  r9 : 0x%016llx\n", frame->tf_r8, frame->tf_r9);
	printf("## r10: 0x%016llx  r11: 0x%016llx\n", frame->tf_r10, frame->tf_r11);
	printf("## r12: 0x%016llx  r13: 0x%016llx\n", frame->tf_r12, frame->tf_r13);
	printf("## r14: 0x%016llx  r15: 0x%016llx\n##\n", frame->tf_r14, frame->tf_r15);

	// Print backtrace if it's available
	printf("## backtrace: \n");

	auto s_Saved = vm_fault_disable_pagefaults();
  auto amdFrame = reinterpret_cast<struct amd64_frame*>(frame->tf_rbp);

	if (amdFrame != nullptr)
	{
		auto amdFrameCount = 0;

		while (Debugger2::IsStackSpace(amdFrame))
		{
			printf("##  [%d] [return: %p] [from: %p]\n", amdFrameCount, amdFrame->f_retaddr, amdFrame);
			amdFrame = amdFrame->f_frame;
			amdFrameCount++;
		}
	}

	vm_fault_enable_pagefaults(s_Saved);

	// Done
	printf("## hanging crashed thread...\n");
	for(;;);

	// Allow the debugger to be placed here manually and continue execution at some point
	__asm__("pop %rbp;leave;ret;");
}