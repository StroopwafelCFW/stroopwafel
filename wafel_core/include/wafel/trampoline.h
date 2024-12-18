#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "dynamic.h"

LINKABLE void* trampoline_install(uintptr_t addr, void *trampoline, void *trampoline_end);

typedef struct {
    int r[13];
    u32 lr;
    u32 stack[];
} PACKED trampoline_state;

LINKABLE void trampoline_blreplace(uintptr_t addr, void *target);
LINKABLE void trampoline_t_blreplace_with_regs(uintptr_t addr, void *target);
LINKABLE void trampoline_hook_before(uintptr_t addr, void (*target)(trampoline_state*));

typedef struct {
    int r[8];
    u32 lr;
    u32 stack[];
} PACKED trampoline_t_state;

LINKABLE void trampoline_t_blreplace(uintptr_t addr, void *target);
LINKABLE void trampoline_t_blreplace_with_regs(uintptr_t addr, void *target);
LINKABLE void trampoline_t_hook_before(uintptr_t addr, void (*target)(trampoline_t_state*));

#endif //TRAMPOLINE_H