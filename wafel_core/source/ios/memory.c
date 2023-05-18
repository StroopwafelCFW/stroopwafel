#include "ios/memory.h"
#include "ios/svc.h"

void* malloc_global(u32 size)
{
    
    return svcAlloc(0x0001, size);
}
void free_global(void* mem)
{
    svcFree(0x0001, mem);
}