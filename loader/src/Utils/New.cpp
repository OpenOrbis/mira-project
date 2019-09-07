#include <Utils/New.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

#include <vm/vm.h>
#include <sys/malloc.h>

#include <vm/uma.h>

void * operator new(unsigned long int p_Size)
{
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto malloc = (void*(*)(unsigned long size, struct malloc_type* type, int flags))kdlsym(malloc);
	auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

	printf("[+] %llx\n", p_Size);

	auto s_Allocation = malloc(p_Size, M_TEMP, M_ZERO | M_WAITOK);
	printf("[+] %p\n", s_Allocation);

	return s_Allocation;
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
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto free = (void(*)(void* addr, struct malloc_type* type))kdlsym(free);
	auto M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

	printf("[-] %p\n", p_Pointer);

	free(p_Pointer, M_TEMP);
}

void operator delete[](void* p_Pointer) noexcept
{
	::operator delete(p_Pointer);
}
