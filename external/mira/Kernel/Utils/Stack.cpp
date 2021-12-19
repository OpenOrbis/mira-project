#include "Stack.hpp"
#include "Kdlsym.hpp"

const unsigned long c_StackCheckGuard = 0xDEADBABE;
unsigned long __stack_chk_guard = c_StackCheckGuard;

void __stack_chk_guard_setup(void)
{
     __stack_chk_guard = c_StackCheckGuard; //provide some magic numbers
}

void __stack_chk_fail(void)
{
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	printf("!!!!!!!!!!\n\n__stack_chk_fail: stack_corruption detected!!!!!!!!!!!\n\n");
	for (;;)
		__asm__("nop");
}