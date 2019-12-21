#include "Debugger.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>
#include <Plugins/Debugger/Debugger.hpp>

extern "C"
{
	#include <sys/uio.h>
	#include <sys/proc.h>
	#include <sys/ioccom.h>

	#include <vm/vm.h>
	#include <vm/pmap.h>
	#include <machine/frame.h>
	#include <machine/psl.h>
	#include <machine/pmap.h>
	#include <machine/segments.h>
};

using namespace Mira::Plugins;

struct amd64_frame 
{
	struct amd64_frame      *f_frame;
	long                    f_retaddr;
	long                    f_arg0;
};

Debugger::Debugger() :
    m_TrapFatalHook(nullptr)
{
}

Debugger::~Debugger()
{

}

bool Debugger::OnLoad()
{
#if ONI_PLATFORM >= ONI_PLATFORM_ORBIS_BSD_500
	// Create the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = new Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(OnTrapFatal));
    
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        m_TrapFatalHook->Enable();
    }
#endif

	// Enable fuzzing
	return true; // EnableFuzzing();
}

bool Debugger::OnUnload()
{
	WriteLog(LL_Info, "deleting trap fatal hook");
	if (m_TrapFatalHook != nullptr)
    {
        if (m_TrapFatalHook->IsEnabled())
            m_TrapFatalHook->Disable();
        
        delete m_TrapFatalHook;
        m_TrapFatalHook = nullptr;
    }

	return true;
}

bool Debugger::OnSuspend()
{
	WriteLog(LL_Info, "disabling trap_fatal hook");
	if (m_TrapFatalHook)
		m_TrapFatalHook->Disable();
	
	return true;
}

bool Debugger::OnResume()
{
	WriteLog(LL_Info, "enabling trap_fatal hook");
	if (m_TrapFatalHook)
		m_TrapFatalHook->Enable();

	return true;
}

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

bool Debugger::IsStackSpace(void* p_Address)
{
    return ((reinterpret_cast<uint64_t>(p_Address) & 0xFFFFFFFF00000000) == 0xFFFFFF8000000000);
}

void Debugger::OnTrapFatal(struct trapframe* frame, vm_offset_t eva)
{
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto gdt = (struct user_segment_descriptor*)kdlsym(gdt);/* global descriptor tables */
	auto vm_fault_disable_pagefaults = (int(*)(void))kdlsym(vm_fault_disable_pagefaults);
	auto vm_fault_enable_pagefaults = (void(*)(int))kdlsym(vm_fault_enable_pagefaults);
	auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

	static const char *trap_msg[] = {
		"",					/*  0 unused */
		"privileged instruction fault",		/*  1 T_PRIVINFLT */
		"",					/*  2 unused */
		"breakpoint instruction fault",		/*  3 T_BPTFLT */
		"",					/*  4 unused */
		"",					/*  5 unused */
		"arithmetic trap",			/*  6 T_ARITHTRAP */
		"",					/*  7 unused */
		"",					/*  8 unused */
		"general protection fault",		/*  9 T_PROTFLT */
		"trace trap",				/* 10 T_TRCTRAP */
		"",					/* 11 unused */
		"page fault",				/* 12 T_PAGEFLT */
		"",					/* 13 unused */
		"alignment fault",			/* 14 T_ALIGNFLT */
		"",					/* 15 unused */
		"",					/* 16 unused */
		"",					/* 17 unused */
		"integer divide fault",			/* 18 T_DIVIDE */
		"non-maskable interrupt trap",		/* 19 T_NMI */
		"overflow trap",			/* 20 T_OFLOW */
		"FPU bounds check fault",		/* 21 T_BOUND */
		"FPU device not available",		/* 22 T_DNA */
		"double fault",				/* 23 T_DOUBLEFLT */
		"FPU operand fetch fault",		/* 24 T_FPOPFLT */
		"invalid TSS fault",			/* 25 T_TSSFLT */
		"segment not present fault",		/* 26 T_SEGNPFLT */
		"stack fault",				/* 27 T_STKFLT */
		"machine check trap",			/* 28 T_MCHK */
		"SIMD floating-point exception",	/* 29 T_XMMFLT */
		"reserved (unknown) fault",		/* 30 T_RESERVED */
		"",					/* 31 unused (reserved) */
		"DTrace pid return trap",		/* 32 T_DTRACE_RET */
		"DTrace fasttrap probe trap",		/* 33 T_DTRACE_PROBE */
	};

	int code, ss;
	u_int type;
	long esp;
	struct soft_segment_descriptor softseg;
	const char *msg;

	code = frame->tf_err;
	type = frame->tf_trapno;
	sdtossd(&gdt[NGDT * PCPU_GET(cpuid) + IDXSEL(frame->tf_cs & 0xffff)],
	    &softseg);

	if (type <= MAX_TRAP_MSG)
		msg = trap_msg[type];
	else
		msg = "UNKNOWN";
	printf("\n\nFatal trap %d: %s while in %s mode\n", type, msg,
	    ISPL(frame->tf_cs) == SEL_UPL ? "user" : "kernel");
#ifdef SMP
	/* two separate prints in case of a trap on an unmapped page */
	printf("cpuid = %d; ", PCPU_GET(cpuid));
	printf("apic id = %02x\n", PCPU_GET(apic_id));
#endif
	if (type == T_PAGEFLT) {
		printf("fault virtual address	= 0x%lx\n", eva);
		printf("fault code		= %s %s %s, %s\n",
			code & PGEX_U ? "user" : "supervisor",
			code & PGEX_W ? "write" : "read",
			code & PGEX_I ? "instruction" : "data",
			code & PGEX_P ? "protection violation" : "page not present");
	}
	printf("instruction pointer	= 0x%lx:0x%lx\n",
	       frame->tf_cs & 0xffff, frame->tf_rip);
        if (ISPL(frame->tf_cs) == SEL_UPL) {
		ss = frame->tf_ss & 0xffff;
		esp = frame->tf_rsp;
	} else {
		ss = GSEL(GDATA_SEL, SEL_KPL);
		esp = (long)&frame->tf_rsp;
	}
	printf("stack pointer	        = 0x%x:0x%lx\n", ss, esp);
	printf("frame pointer	        = 0x%x:0x%lx\n", ss, frame->tf_rbp);
	printf("code segment		= base 0x%lx, limit 0x%lx, type 0x%x\n",
	       softseg.ssd_base, softseg.ssd_limit, softseg.ssd_type);
	printf("			= DPL %d, pres %d, long %d, def32 %d, gran %d\n",
	       softseg.ssd_dpl, softseg.ssd_p, softseg.ssd_long, softseg.ssd_def32,
	       softseg.ssd_gran);
	printf("processor eflags	= ");
	if (frame->tf_rflags & PSL_T)
		printf("trace trap, ");
	if (frame->tf_rflags & PSL_I)
		printf("interrupt enabled, ");
	if (frame->tf_rflags & PSL_NT)
		printf("nested task, ");
	if (frame->tf_rflags & PSL_RF)
		printf("resume, ");
	printf("IOPL = %ld\n", (frame->tf_rflags & PSL_IOPL) >> 12);
	printf("current process		= ");
	if (curproc) {
		printf("%lu (%s)\n",
		    (u_long)curproc->p_pid, curthread->td_name/* ?
		    curthread->td_name : ""*/);
	} else {
		printf("Idle\n");
	}

#ifdef KDB
	if (debugger_on_panic || kdb_active)
		if (kdb_trap(type, 0, frame))
			return;
#endif
	printf("trap number		= %d\n", type);
	if (type <= MAX_TRAP_MSG)
		printf("%s\n", trap_msg[type]);
	else
		printf("unknown/reserved trap");
	
	auto s_Saved = vm_fault_disable_pagefaults();
	// Print extra information in case that Oni/Mira itself crashes
	auto s_Framework = Mira::Framework::GetFramework();
	if (s_Framework)
	{
		auto s_InitParams = s_Framework->GetInitParams();

		printf("mira base: %p size: %p\n", s_InitParams->payloadBase, s_InitParams->payloadSize);
		printf("mira proc: %p entrypoint: %p\n", s_InitParams->process, s_InitParams->entrypoint);
		printf("mira mira_entry: %p\n", mira_entry);

		if (s_Framework)
			printf("mira messageManager: %p pluginManager: %p rpcServer: %p\n", s_Framework->GetMessageManager(), s_Framework->GetPluginManager(), s_Framework->GetRpcServer());
	
		printf("OffsetFromKernelBase: %p\n", frame->tf_last_branch_from - (uint64_t)gKernelBase);
		printf("OffsetFromMiraEntry: [tf_last_branch_from-mira_entry]:%p [mira_entry-tf_last_branch_from]:%p\n", frame->tf_last_branch_from - reinterpret_cast<uint64_t>(mira_entry), reinterpret_cast<uint64_t>(mira_entry) - frame->tf_last_branch_from);
    }

	printf("call stack:\n");
    auto amdFrame = reinterpret_cast<struct amd64_frame*>(frame->tf_rbp);
	auto amdFrameCount = 0;
	while (Debugger::IsStackSpace(amdFrame))
	{
		printf("[%d] [r: %p] [f:%p]\n", amdFrameCount, amdFrame->f_retaddr, amdFrame);
		amdFrame = amdFrame->f_frame;
		amdFrameCount++;
	}
	vm_fault_enable_pagefaults(s_Saved);
	
	// Intentionally hang the thread
	/*for (;;)
		__asm__("nop");*/
	
	printf("exiting crashed thread\n");
	kthread_exit();

	// Allow the debugger to be placed here manually and continue exceution
	__asm__("pop %rbp;leave;ret;");
}

bool Debugger::Attach(int32_t p_ProcessId, bool p_StopOnAttach)
{
	//auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
	if (s_MainThread == nullptr)
	{
		WriteLog(LL_Error, "could not get mira main thread");
		return false;
	}

	if (m_ProcessId != -1)
	{
		WriteLog(LL_Error, "please disconnect from current process (%d).", m_ProcessId);
		return false;
	}
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		return false;
	}

	if (p_StopOnAttach)
	{
		auto s_KillResult = kkill_t(p_ProcessId, SIGSTOP, s_MainThread);
		if (s_KillResult < 0)
			WriteLog(LL_Error, "could not stop process for pid (%d).", p_ProcessId);
	}
	
	m_ProcessId = p_ProcessId;
	PROC_UNLOCK(s_Process);

	return true;
}

bool Debugger::Detach(bool p_ResumeOnDetach)
{
	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
	if (s_MainThread == nullptr)
	{
		WriteLog(LL_Error, "could not get mira main thread");
		return false;
	}

	if (m_ProcessId == -1)
	{
		WriteLog(LL_Error, "not attached, no pid");
		return true;
	}

	// Zero the registers
	memset(&m_Registers, 0, sizeof(m_Registers));
	memset(&m_FloatingRegisters, 0, sizeof(m_FloatingRegisters));
	memset(&m_DebugRegisters, 0, sizeof(m_DebugRegisters));

	if (p_ResumeOnDetach)
	{
		auto s_Result = kkill_t(m_ProcessId, SIGCONT, s_MainThread);
		if (s_Result < 0)
			WriteLog(LL_Error, "could not resume process (%d).", m_ProcessId);
	}

	m_ProcessId = -1;
	return true;
}

uint32_t Debugger::ReadData(uint64_t p_Address, uint8_t* p_OutData, uint32_t p_Size)
{
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

	if (m_ProcessId == -1)
	{
		WriteLog(LL_Error, "not attached to process.");
		return 0;
	}

	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
	if (s_MainThread == nullptr)
	{
		WriteLog(LL_Error, "could not get mira main thread");
		return 0;
	}

	auto proc_rwmem = (int(*)(struct proc* p, struct uio* uio))kdlsym(proc_rwmem);

	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(m_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_ProcessId);
		return 0;
	}

	struct iovec iov =
	{
		.iov_base = (caddr_t)p_OutData,
		.iov_len = p_Size
	};

	struct uio uio =
	{
		.uio_iov = &iov,
		.uio_iovcnt = 1,
		.uio_offset = (off_t)p_Address,
		.uio_resid = (ssize_t)p_Size,
		.uio_segflg = UIO_SYSSPACE,
		.uio_rw = UIO_READ,
		.uio_td = s_MainThread
	};

	auto s_Ret = proc_rwmem(s_Process, &uio);
	auto s_BytesRead = 0;
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "could not read process (%d) addr: (%p) sz: (%d).", m_ProcessId, p_Address, p_Size);
	}
	else
		s_BytesRead = p_Size;

	PROC_UNLOCK(s_Process);

	return s_BytesRead;
}

uint32_t Debugger::WriteData(uint64_t p_Address, uint8_t* p_Data, uint32_t p_Size)
{
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	if (m_ProcessId == -1)
	{
		WriteLog(LL_Error, "not attached to process.");
		return 0;
	}

	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
	if (s_MainThread == nullptr)
	{
		WriteLog(LL_Error, "could not get mira main thread");
		return 0;
	}

	auto proc_rwmem = (int(*)(struct proc* p, struct uio* uio))kdlsym(proc_rwmem);

	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(m_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", m_ProcessId);
		return 0;
	}

	struct iovec iov =
	{
		.iov_base = (caddr_t)p_Data,
		.iov_len = p_Size
	};

	struct uio uio =
	{
		.uio_iov = &iov,
		.uio_iovcnt = 1,
		.uio_offset = (off_t)p_Address,
		.uio_resid = (ssize_t)p_Size,
		.uio_segflg = UIO_SYSSPACE,
		.uio_rw = UIO_WRITE,
		.uio_td = s_MainThread
	};

	auto s_Ret = proc_rwmem(s_Process, &uio);
	auto s_BytesRead = 0;
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "could not read process (%d) addr: (%p) sz: (%d).", m_ProcessId, p_Address, p_Size);
	}
	else
		s_BytesRead = p_Size;

	PROC_UNLOCK(s_Process);

	return s_BytesRead;
}

bool Debugger::SingleStep()
{
	return true;
}

bool Debugger::UpdateRegisters()
{
	return true;
}

bool Debugger::UpdateWatches()
{
	return true;
}

bool Debugger::UpdateBreakpoints()
{
	return true;
}

bool Debugger::IsProcessAlive(int32_t p_ProcessId)
{
	auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
	auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		return false;
	}
	PROC_UNLOCK(s_Process);
	return true;
}