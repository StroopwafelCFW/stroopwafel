#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "dynamic.h"

LINKABLE void install_trampoline(void *addr, void *target);

#endif //TRAMPOLINE_H