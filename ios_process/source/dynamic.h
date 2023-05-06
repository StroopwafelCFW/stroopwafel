#ifndef DYNAMIC_H
#define DYNAMIC_H

#include <elf.h>
#include <stddef.h>

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn);

#endif // DYNAMIC_H