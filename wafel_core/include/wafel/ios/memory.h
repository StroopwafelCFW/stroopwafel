#ifndef MEMORY_H
#define MEMORY_H

#include "../types.h"

#include "svc.h"

#define HEAPID_SHARED 0x0001
#define HEAPID_LOCAL 0xCAFE
#define HEAPID_CROSS_PROCESS 0xCAFF

static inline void* malloc_global(u32 size)
{
    return iosAlloc(HEAPID_SHARED, size);
}

static inline void free_global(void* mem){
    iosFree(HEAPID_SHARED, mem);
}

static inline void* malloc_local(u32 size){
    return iosAlloc(HEAPID_LOCAL, size);
}

static inline void free_local(void* mem){
    iosFree(HEAPID_LOCAL, mem);
}

static inline void* malloc_cross_process(u32 size){
    return iosAlloc(HEAPID_CROSS_PROCESS, size);
}

static inline void free_cross_process(void* mem){
    iosFree(HEAPID_CROSS_PROCESS, mem);
}

#endif
