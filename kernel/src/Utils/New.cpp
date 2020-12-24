// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "New.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

extern "C"
{
	#include <vm/vm.h>
	#include <sys/malloc.h>

	#include <vm/uma.h>
};

void * operator new(unsigned long int p_Size)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnew-returns-null"
	if (p_Size == 0)
		return nullptr;
#pragma clang diagnostic pop
	
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
	if (p_Pointer == nullptr)
		return;
	
	auto free = (void(*)(void* addr, struct malloc_type* type))kdlsym(free);
	auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

	free(p_Pointer, M_TEMP);
}

void operator delete(void* p_Pointer, unsigned long int p_Size) noexcept
{
	::operator delete(p_Pointer);
}

void operator delete[](void* p_Pointer) noexcept
{
	::operator delete(p_Pointer);
}
