#ifndef DYNAMIC_H
#define DYNAMIC_H

#include <elf.h>
#include <stddef.h>

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn);
uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr);
Elf32_Phdr* ios_elf_add_phdr(uintptr_t addr);
void ios_elf_print_map();

extern uintptr_t dynamic_phys_base_addr;

#define MCP_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + MCP_ALT_BASE)
#define FS_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + FS_ALT_BASE)

#endif // DYNAMIC_H