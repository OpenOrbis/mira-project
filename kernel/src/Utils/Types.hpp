#pragma once
#include <sys/types.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

#ifndef MSGPACK
#define MSGPACK __attribute__((__packed__))
#endif

typedef uint32_t u_int32_t;