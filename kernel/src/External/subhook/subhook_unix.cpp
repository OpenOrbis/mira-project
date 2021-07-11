/*
 * Copyright (c) 2012-2018 Zeex
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>

#include <mira/Kernel/Utils/Kdlsym.hpp>

#define SUBHOOK_CODE_PROTECT_FLAGS (PROT_READ | PROT_WRITE | PROT_EXEC)

int subhook_unprotect(void *address, size_t size) {
    return 0;
    /*long pagesize;

    pagesize = sysconf(_SC_PAGESIZE);
    address = (void *)((long)address & ~(pagesize - 1));

    return mprotect(address, size, SUBHOOK_CODE_PROTECT_FLAGS);*/
}

void *subhook_alloc_code(size_t size) {
    if (!size)
		size = sizeof(uint64_t);
	
	void* kmem_alloc = kdlsym(kmem_alloc);
	struct vm_map * map = (struct vm_map *)(*(uint64_t *)(kdlsym(kernel_map)));

	vm_offset_t data = ((vm_offset_t(*)(struct vm_map *, vm_size_t))kmem_alloc)(map, size);
	if (!data)
		return NULL;

	// Set our pointer header
	(*(uint64_t*)data) = size;

	// Return the start of the requested data
	return (uint8_t*)data + sizeof(uint64_t);
}

int subhook_free_code(void *address, size_t size) {
  	if (!address)
		return 0;

	struct vm_map * map = (struct vm_map *)(*(uint64_t *)(kdlsym(kernel_map)));

	void* kmem_free = kdlsym(kmem_free);

	
	uint8_t* data = ((uint8_t*)address) - sizeof(uint64_t);

	uint64_t alloc_size = *(uint64_t*)data;

	((void(*)(void* map, void* addr, size_t size))kmem_free)(map, data, alloc_size);

    return 0;
}