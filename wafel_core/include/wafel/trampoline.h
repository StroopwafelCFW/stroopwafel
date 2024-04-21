#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "dynamic.h"

LINKABLE void* trampoline_install(uintptr_t addr, void *trampoline, void *trampoline_end);
LINKABLE void trampoline_blreplace(uintptr_t addr, void *target);

typedef struct {
    int r[13];
    u32 lr;
    u32 stack[];
} PACKED trampoline_state;

LINKABLE void trampoline_hook_before(uintptr_t addr, void *target);

#endif //TRAMPOLINE_H