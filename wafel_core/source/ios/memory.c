#include "ios/memory.h"
#include "ios/svc.h"

void* malloc_global(u32 size)
{
    return iosAlloc(0x0001, size);
}
void free_global(void* mem)
{
    iosFree(0x0001, mem);
}