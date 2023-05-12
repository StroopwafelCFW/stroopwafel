#ifndef DYNAMIC_H
#define DYNAMIC_H

#include <elf.h>
#include <stddef.h>

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn);
uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr);

extern uintptr_t dynamic_phys_base_addr;

#define MCP_ALT_BASE (0x05200000)
#define MCP_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + MCP_ALT_BASE)

#define FS_ALT_BASE (0x10600000)
#define FS_ALTBASE_ADDR(_addr) (((u32)_addr) - dynamic_phys_base_addr + FS_ALT_BASE)

#endif // DYNAMIC_H