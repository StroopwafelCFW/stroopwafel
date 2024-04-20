#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "dynamic.h"

LINKABLE void install_trampoline(uintptr_t addr, void *target, void *trampoline, void *trampoline_end, void *trampoline_target, void *trampoline_chain);
LINKABLE void blreplace_trampoline(uintptr_t addr, void *target);

#endif //TRAMPOLINE_H