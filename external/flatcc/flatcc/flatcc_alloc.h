#ifndef FLATCC_ALLOC_H
#define FLATCC_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif



/*
 * These allocation abstractions are __only__ for runtime libraries.
 *
 * The flatcc compiler uses Posix allocation routines regardless
 * of how this file is configured.
 *
 * This header makes it possible to use systems where malloc is not
 * valid to use. In this case the portable library will not help
 * because it implements Posix / C11 abstractions.
 *
 * Systems like FreeRTOS do not work with Posix memory calls and here it
 * can be helpful to override runtime allocation primitives.
 *
 * In general, it is better to customize the allocator and emitter via
 * flatcc_builder_custom_init and to avoid using the default emitter
 * specific high level calls the copy out a buffer that must later be
 * deallocated. This provides full control of allocation withou the need
 * for this file.
 *
 *
 * IMPORTANT
 *
 * If you override malloc, free, etc., make sure your applications
 * use the same allocation methods. For example, samples/monster.c
 * and several test cases are no longer guaranteed to work out of the
 * box.
 *
 * The changes must only affect target runtime compilation including the
 * the runtime library libflatccrt.
 *
 * The host system flatcc compiler and the compiler library libflatcc
 * should NOT be compiled with non-Posix allocation since the compiler
 * has a dependency on the runtime library and the wrong free operation
 * might be callled. The safest way to avoid this problem this is to
 * compile flatcc with the CMake script and the runtime files with a
 * dedicated build system for the target system.
 */

//#include <stdlib.h>
#include <flatcc/portable/paligned_alloc.h>
#include <Utils/Kdlsym.hpp>
#include <sys/malloc.h>

// typedef void*(*malloc_t)(unsigned long size, struct malloc_type* type, int flags);
// typedef void*(*realloc_t)(void *addr, unsigned long size, struct malloc_type *mtp, int flags);
// typedef void(*free_t)(void* addr, struct malloc_type* type);

static inline void *__mira_malloc(size_t size)
{
    
    malloc_t malloc = (malloc_t)kdlsym(malloc);
	struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    return malloc(size, M_TEMP, 0x0102);
}

static inline void *__mira_realloc(void* p, size_t size)
{
    
    realloc_t _realloc = (realloc_t)kdlsym(realloc);
    struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    return _realloc(p, size, M_TEMP, 0x0102);
}

static inline void *__mira_calloc(size_t num, size_t size)
{
    malloc_t malloc = (malloc_t)kdlsym(malloc);
	struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    return malloc(num * size, M_TEMP, 0x0102);
}

static inline void __mira_free(void* p)
{
    
    free_t free = (free_t)kdlsym(free);
	struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    free(p, M_TEMP);
}

#ifndef FLATCC_ALLOC
#define FLATCC_ALLOC(n) __mira_malloc(n)
#endif

#ifndef FLATCC_FREE
#define FLATCC_FREE(p) __mira_free(p)
#endif

#ifndef FLATCC_REALLOC
#define FLATCC_REALLOC(p, n) __mira_realloc(p, n)
#endif

#ifndef FLATCC_CALLOC
#define FLATCC_CALLOC(nm, n) __mira_calloc(nm, n)
#endif

#ifndef FLATCC_ALIGNED_ALLOC
#define FLATCC_ALIGNED_ALLOC(a, n) aligned_alloc(a, n)
//aligned_alloc(a, n)
#endif

#ifndef FLATCC_ALIGNED_FREE
#define FLATCC_ALIGNED_FREE(p) aligned_free(p)
#endif

// /*
//  * Implements `aligned_alloc` and `aligned_free`.
//  * Even with C11, this implements non-standard aligned_free needed for portable
//  * aligned_alloc implementations.
//  */
// #ifndef FLATCC_USE_GENERIC_ALIGNED_ALLOC

// #ifndef FLATCC_NO_PALIGNED_ALLOC
// #include <flatcc/portable/paligned_alloc.h>
// #else
// #if !defined(__aligned_free_is_defined) || !__aligned_free_is_defined
// #define aligned_free free
// #endif
// #endif

// #else /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */

// #ifndef FLATCC_ALIGNED_ALLOC
// static inline void *__flatcc_aligned_alloc(size_t alignment, size_t size)
// {
//     char *raw;
//     void *buf;
//     size_t total_size = (size + alignment - 1 + sizeof(void *));

//     if (alignment < sizeof(void *)) {
//         alignment = sizeof(void *);
//     }
//     raw = (char *)(size_t)FLATCC_ALLOC(total_size);
//     buf = raw + alignment - 1 + sizeof(void *);
//     buf = (void *)(((size_t)buf) & ~(alignment - 1));
//     ((void **)buf)[-1] = raw;
//     return buf;
// }
// #define FLATCC_ALIGNED_ALLOC(alignment, size) __flatcc_aligned_alloc(alignment, size)
// #endif /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */

// #ifndef FLATCC_ALIGNED_FREE
// static inline void __flatcc_aligned_free(void *p)
// {
//     char *raw;

//     if (!p) return;
//     raw = ((void **)p)[-1];

//     FLATCC_FREE(raw);
// }
// #define FLATCC_ALIGNED_FREE(p) __flatcc_aligned_free(p)
// #endif

// #endif /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */


// #include <flatcc/portable/paligned_alloc.h>



#ifdef __cplusplus
}
#endif

#endif /* FLATCC_ALLOC_H */
