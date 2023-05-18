#ifndef DYNAMIC_H
#define DYNAMIC_H

#include <elf.h>
#include <stddef.h>

#define LINKABLE __attribute__ ((weak))
#define MAGIC_DONE (0x444F4E45)

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn);
LINKABLE uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr);
LINKABLE Elf32_Phdr* ios_elf_add_phdr(uintptr_t addr);
LINKABLE void ios_elf_print_map();

LINKABLE uintptr_t wafel_plugin_max_addr(uintptr_t base);
LINKABLE uintptr_t wafel_get_symbol_addr(uintptr_t base, const char* name);
LINKABLE void wafel_register_plugin(uintptr_t base);
LINKABLE uintptr_t wafel_find_symbol(const char* name);

extern uintptr_t dynamic_phys_base_addr;

#define MCP_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + MCP_ALT_BASE)
#define FS_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + FS_ALT_BASE)

#endif // DYNAMIC_H