#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

LINKABLE void* malloc_global(u32 size);
LINKABLE void free_global(void* mem);

#endif
