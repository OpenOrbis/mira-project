#include "KernelDebug.hpp"

#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <Plugins/PluginManager.hpp>
#include <Plugins/Debugging/Debugger.hpp>
#include <Mira.hpp>

extern "C" {
	#include <machine/reg.h>
	#include <machine/trap.h>
	#include <machine/pmap.h>
	#include <machine/psl.h>
	#include <machine/segments.h>
	#include <machine/trap.h>
}

using namespace Mira::Debug;

static inline void write_dr0(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr0" :: "r" (linear));
}

static inline void write_dr1(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr1" :: "r" (linear));
}

static inline void write_dr2(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr2" :: "r" (linear));
}

static inline void write_dr3(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr3" :: "r" (linear));
}

static inline void write_dr4(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr4" :: "r" (linear));
}

static inline void write_dr5(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr5" :: "r" (linear));
}

static inline void write_dr6(unsigned long linear)
{
    __asm__ __volatile__ ("mov %0, %%dr6" :: "r" (linear));
}

static inline void write_dr7(unsigned long val)
{
    __asm__ __volatile__ ("mov %0, %%dr7" :: "r" (val));
}

static inline unsigned long read_dr7(void)
{
    unsigned long val;

    __asm__ __volatile__ ("mov %%dr7, %0" : "=r" (val));

    return val;
}

void KernelDebug::EnableTrap(int index, void* address, char dr_len, char dr_breaktype) {
	switch(index) {
		case 0: {
			write_dr0((unsigned long)address);
			break;
		}

		case 1: {
			write_dr1((unsigned long)address);
			break;
		}

		case 2: {
			write_dr2((unsigned long)address);
			break;
		}

		case 3: {
			write_dr3((unsigned long)address);
			break;
		}

		case 4: {
			write_dr4((unsigned long)address);
			break;
		}

		case 5: {
			write_dr5((unsigned long)address);
			break;
		}

		case 6: {
			write_dr6((unsigned long)address);
			break;
		}

		default: {
			WriteLog(LL_Error, "Index out of bounce");
			return;
		}
	}

	unsigned long Dr7 = read_dr7();
	Dr7 &= ~DBREG_DR7_MASK(index); 
	Dr7 |= DBREG_DR7_SET(index, dr_len, dr_breaktype, DBREG_DR7_LOCAL_ENABLE | DBREG_DR7_GLOBAL_ENABLE);
	write_dr7(Dr7);
}

void KernelDebug::DisableTrap(int index) {
	if (index > 6) {
		WriteLog(LL_Error, "Index out of bounce");
		return;
	}

	unsigned long Dr7 = read_dr7();
	Dr7 &= ~DBREG_DR7_MASK(index);
	Dr7 |= DBREG_DR7_SET(index, NULL, NULL, NULL);
	write_dr7(Dr7);

	switch(index) {
		case 0: {
			write_dr0(NULL);
			break;
		}

		case 1: {
			write_dr1(NULL);
			break;
		}

		case 2: {
			write_dr2(NULL);
			break;
		}

		case 3: {
			write_dr3(NULL);
			break;
		}

		case 4: {
			write_dr4(NULL);
			break;
		}

		case 5: {
			write_dr5(NULL);
			break;
		}

		case 6: {
			write_dr6(NULL);
			break;
		}
	}
}