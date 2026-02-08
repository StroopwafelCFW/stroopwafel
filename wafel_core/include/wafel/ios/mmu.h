#ifndef WAFEL_IOS_MMU_H
#define WAFEL_IOS_MMU_H

#include "../types.h"

typedef struct {
    u32 paddr;
    u32 vaddr;
    u32 size;
    u32 domain;
    u32 type;
    u32 cached;
} ios_map_shared_info_t;

#endif // WAFEL_IOS_MMU_H
