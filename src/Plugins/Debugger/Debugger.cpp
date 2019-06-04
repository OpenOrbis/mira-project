#include "Debugger.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

// thread
#include <sys/proc.h>

// stackframe
#include <machine/frame.h>

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
    // Create an enable the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = new Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(OnTrapFatal));
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        m_TrapFatalHook->Enable();
    }
}

Debugger::~Debugger()
{
    if (m_TrapFatalHook != nullptr)
    {
        if (m_TrapFatalHook->IsEnabled())
            m_TrapFatalHook->Disable();
        
        delete m_TrapFatalHook;
        m_TrapFatalHook = nullptr;
    }
}

void Debugger::OnTrapFatal(struct trapframe* p_Frame, vm_offset_t p_Eva)
{
    //auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

	uint32_t amdFrameCount = 0;
	struct amd64_frame* amdFrame = nullptr;
    void* s_Rsp = nullptr;
    static const char* dash = "-----------------------";

    if (p_Frame == nullptr)
        goto hang_thread;

    

    if (curthread == nullptr)
        goto hang_thread;

    // Set the correct RSP
    s_Rsp = ((uint8_t*)p_Frame - 0xA8);

	WriteLog(LL_Info, "kernel panic detected");
	WriteLog(LL_Info, "call stizzack");
    amdFrame = (struct amd64_frame*)p_Frame->tf_rbp;
	while (IsStackSpace(amdFrame))
	{
		WriteLog(LL_Debug, "[%d] - f: %p ret: %p", amdFrameCount, amdFrame, amdFrame->f_retaddr);
		amdFrame = amdFrame->f_frame;
		amdFrameCount++;
	}

	// Print extra information in case that Oni/Mira itself crashes
	/* if (gInitParams)
	{
		WriteLog(LL_Info, dash);
		WriteLog(LL_Info, "mira base: %p size: %p", gInitParams->payloadBase, gInitParams->payloadSize);
		WriteLog(LL_Info, "mira proc: %p entrypoint: %p", gInitParams->process);
		WriteLog(LL_Info, "mira oni_kernelInitialization: %p", oni_kernelInitialization);

		if (gFramework)
			WriteLog(LL_Info, "mira messageManager: %p pluginManager: %p rpcServer: %p", gFramework->messageManager, gFramework->pluginManager, gFramework->rpcServer);
	
		WriteLog(LL_Info, "OffsetFromKernelBase: %p", frame->tf_last_branch_from - (uint64_t)gKernelBase);
		WriteLog(LL_Info, "OffsetFromMiraBase: %p", frame->tf_last_branch_from - gInitParams->payloadBase);
    }*/

	WriteLog(LL_Info, dash);
	WriteLog(LL_Info, "gKernelBase: %p", gKernelBase);
	WriteLog(LL_Info, "thread: %p proc: %p pid: %d path: %s", curthread, curthread->td_proc, curthread->td_proc->p_pid, curthread->td_proc->p_elfpath);
	WriteLog(LL_Info, "eva: %p", p_Eva);
	WriteLog(LL_Info, "rdi: %p", p_Frame->tf_rdi);
	WriteLog(LL_Info, "rsi: %p", p_Frame->tf_rsi);
	WriteLog(LL_Info, "rdx: %p", p_Frame->tf_rdx);
	WriteLog(LL_Info, "rcx: %p", p_Frame->tf_rcx);
	WriteLog(LL_Info, "r8: %p", p_Frame->tf_r8);
	WriteLog(LL_Info, "r9: %p", p_Frame->tf_r9);
	WriteLog(LL_Info, "rax: %p", p_Frame->tf_rax);
	WriteLog(LL_Info, "rbx: %p", p_Frame->tf_rbx);
	WriteLog(LL_Info, "rbp: %p", p_Frame->tf_rbp);
	WriteLog(LL_Info, "r10: %p", p_Frame->tf_r10);
	WriteLog(LL_Info, "r11: %p", p_Frame->tf_r11);
	WriteLog(LL_Info, "r12: %p", p_Frame->tf_r12);
	WriteLog(LL_Info, "r13: %p", p_Frame->tf_r13);
	WriteLog(LL_Info, "r14: %p", p_Frame->tf_r14);
	WriteLog(LL_Info, "r15: %p", p_Frame->tf_r15);
	WriteLog(LL_Info, "trapno: %u", p_Frame->tf_trapno);
	WriteLog(LL_Info, "fs: %u", p_Frame->tf_fs);
	WriteLog(LL_Info, "gs: %u", p_Frame->tf_gs);
	WriteLog(LL_Info, "addr: %p", p_Frame->tf_addr);
	WriteLog(LL_Info, "flags: %u", p_Frame->tf_flags);
	WriteLog(LL_Info, "es: %u", p_Frame->tf_es);
	WriteLog(LL_Info, "ds: %u", p_Frame->tf_ds);
	WriteLog(LL_Info, "last branch from: %p", p_Frame->tf_last_branch_from);
	WriteLog(LL_Info, "last branch to: %p", p_Frame->tf_last_branch_to);
	WriteLog(LL_Info, "err: %p", p_Frame->tf_err);
	WriteLog(LL_Info, "rip: %p", p_Frame->tf_rip);
	WriteLog(LL_Info, "cs: %p", p_Frame->tf_cs);
	WriteLog(LL_Info, "rflags: %p", p_Frame->tf_rflags);
	WriteLog(LL_Info, "rsp adjusted: %p rsp: %p", s_Rsp, p_Frame->tf_rsp);
	WriteLog(LL_Info, "err: %p", p_Frame->tf_err);
	WriteLog(LL_Info, dash);

	// If the kernel itself crashes, we don't want this to be debuggable, otherwise the entire console hangs
	/*if (curthread->td_proc->p_pid == 0)
	{
		// See if we have a trap fatal reference, if not we just hang and let the console die
		if (m_TrapFatalHook == nullptr)
			goto hang_thread;

		// Get the original hook
		void(*onTrapFatal)(struct trapframe* frame, vm_offset_t eva) = hook_getFunctionAddress(gTrapFatalHook);

		// Disable the hook
		

		// Call original sce trap fatal
		onTrapFatal(frame, eva);
		return;
	}*/


hang_thread:
    for (;;)
        __asm__("nop");

/*kill_thread:
    kthread_exit();
    return;*/
    
    // Allow the debugger to be placed here manually to continue execution
    __asm__("pop %rbp;leave;ret;");
}

bool Debugger::IsStackSpace(void* p_Address)
{
    return ((reinterpret_cast<uint64_t>(p_Address) & 0xFFFFFFFF00000000) == 0xFFFFFF8000000000);
}