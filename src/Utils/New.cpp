#include "New.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

#include <vm/vm.h>
#include <sys/malloc.h>

#include <vm/uma.h>

uint32_t upper_power_of_two(uint32_t v)
{
	v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

volatile static uint64_t g_AllocSize = 0;

void * operator new(unsigned long int p_Size)
{
    vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
	//auto uma_large_malloc = (void*(*)(size_t size, int flags))kdlsym(uma_large_malloc);
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);

    uint64_t totalSize = p_Size + sizeof(uint64_t);

	//uint8_t* data = reinterpret_cast<uint8_t*>(uma_large_malloc(p_Size, M_WAITOK | M_ZERO | M_EXEC));
	uint8_t* data = reinterpret_cast<uint8_t*>(kmem_alloc(map, totalSize));
	if (data == nullptr)
	{
		//WriteLog(LL_Error, "could not allocate new of size %llx", p_Size);
		printf("could not allocate new size %llx.\n", p_Size);
		for (;;)
			__asm__ ("nop");
	}	
	
	// Update this for tracking
	g_AllocSize += totalSize;

	// Set our pointer header
	(*(uint64_t*)data) = totalSize;

	// Get the returnable address
	uint8_t* retAddress = data + sizeof(uint64_t);

	// Zero the buffer
	memset(retAddress, 0, p_Size);

	//printf("[+] [0x%x] %p sz: (%x) allocsz: (%x)\n", g_AllocSize, retAddress, p_Size, totalSize);

	// Return the start of the requested data
	return retAddress;
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
void operator delete(void* p_Pointer)
{
	vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_free = (void(*)(void* map, void* addr, size_t size))kdlsym(kmem_free);
	//auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	
    if (p_Pointer == nullptr)
		return;

	uint8_t* data = ((uint8_t*)p_Pointer) - sizeof(uint64_t);

	uint64_t totalSize = *(uint64_t*)data;

	kmem_free(map, data, totalSize);

	g_AllocSize -= totalSize;

	//printf("[-] [0x%x] %p %x\n", g_AllocSize, p_Pointer, totalSize);
}

void operator delete[](void* p_Pointer)
{
	::operator delete(p_Pointer);
}
