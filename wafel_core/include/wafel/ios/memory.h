#ifndef MEMORY_H
#define MEMORY_H

#include "../types.h"

#include "ios/memory.h"
#include "ios/svc.h"

static inline void* malloc_global(u32 size)
{
    return iosAlloc(0x0001, size);
}

static inline void free_global(void* mem)
{
    iosFree(0x0001, mem);
}

#endif
