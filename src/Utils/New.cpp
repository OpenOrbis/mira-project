#include "New.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Logger.hpp>

#include <vm/vm.h>
#include <sys/malloc.h>

void * operator new(unsigned long int p_Size)
{
    vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));
	auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);

    uint64_t totalSize = p_Size + sizeof(uint64_t);
	uint8_t* data = reinterpret_cast<uint8_t*>(kmem_alloc(map, totalSize));
	if (data == nullptr)
	{
		WriteLog(LL_Error, "could not allocate new of size %llx", p_Size);
		for (;;)
			__asm__ ("nop");
	}

	// Set our pointer header
	(*(uint64_t*)data) = totalSize;

	// Get the returnable address
	auto retAddress = data + sizeof(uint64_t);

	// Zero the buffer
	memset(retAddress, 0, p_Size);

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

    if (p_Pointer == nullptr)
		return;

	uint8_t* data = ((uint8_t*)p_Pointer) - sizeof(uint64_t);

	uint64_t totalSize = *(uint64_t*)data;

	kmem_free(map, data, totalSize);
}

void operator delete[](void* p_Pointer)
{
	::operator delete(p_Pointer);
}
