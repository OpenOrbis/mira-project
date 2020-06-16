// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "New.hpp"
#ifdef __PS4__
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>

#include <vm/vm.h>
#include <sys/malloc.h>

#include <vm/uma.h>
#else
#include <stdlib.h>
#endif

#include <Utils/Logger.hpp>



void * operator new(unsigned long int p_Size)
{
#ifdef __PS4__
	auto malloc = (void*(*)(unsigned long size, struct malloc_type* type, int flags))kdlsym(malloc);
	auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

	if (p_Size >= 0x10000000)
	{
		auto printf = (void(*)(const char *format, ...))kdlsym(printf);
		printf("op_new error: requested (%llx) data\n\n\n", p_Size);

		for (;;)
			__asm__("nop");
	}

	return malloc(p_Size, M_TEMP, M_ZERO | M_NOWAIT);
#else
	return malloc(p_Size);
#endif
}

// placement new
void * operator new(unsigned long int p_Size, void * p_Pointer)
{
    return p_Pointer;
}

// operator new[]
void * operator new[] (unsigned long int p_Size)
{
	return ::operator new(p_Size);
}

// Delete
void operator delete(void* p_Pointer) noexcept
{
#ifdef __PS4__
	if (p_Pointer == nullptr)
		return;
	
	auto free = (void(*)(void* addr, struct malloc_type* type))kdlsym(free);
	auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

	free(p_Pointer, M_TEMP);
#else
	free(p_Pointer);
#endif
}

void operator delete[](void* p_Pointer) noexcept
{
	::operator delete(p_Pointer);
}
