#include <flatcc/portable/paligned_alloc.h>

inline void *__portable_aligned_alloc(size_t alignment, size_t size)
{
    malloc_t _malloc = (malloc_t)kdlsym(malloc);
	struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    char *raw;
    void *buf;
    size_t total_size = (size + alignment - 1 + sizeof(void *));

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    raw = (char *)_malloc(total_size, M_TEMP, 0x0102 /*M_ZERO | M_WAITOK*/);
    buf = raw + alignment - 1 + sizeof(void *);
    buf = (void *)(((size_t)buf) & ~(alignment - 1));
    ((void **)buf)[-1] = raw;
    return buf;
}

inline void __portable_aligned_free(void *p)
{
    free_t _free = (free_t)kdlsym(free);
	struct malloc_type* M_TEMP = (struct malloc_type*)kdlsym(M_TEMP);

    char *raw;
    
    if (p) {
        raw = (char*)((void **)p)[-1];
        _free(raw, M_TEMP);
    }
}