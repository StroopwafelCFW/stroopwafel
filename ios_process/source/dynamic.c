#include "dynamic.h"

#include "types.h"
#include "imports.h"
#include "ios/svc.h"
#include "utils.h"

extern void debug_printf(const char *format, ...);

static int is_relocated = 0;
uintptr_t dynamic_phys_base_addr = 0;

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn)
{
    const Elf32_Rel* rel = NULL;
    u64 relsz = 0;

    if (is_relocated) return;

    dynamic_phys_base_addr = base;

    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_REL:
                rel = (const Elf32_Rel*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELSZ:
                relsz = dyn->d_un.d_val / sizeof(Elf32_Rel);
                break;
        }
    }

    if (rel != NULL) {
        for (; relsz--; rel++)
        {
            //debug_printf("%x %x %x\n", ELF32_R_TYPE(rel->r_info), (base + rel->r_offset), *(u32*)(base + rel->r_offset));
            switch (ELF32_R_TYPE(rel->r_info))
            {
                case R_ARM_RELATIVE:
                {
                    u32* ptr = (u32*)(base + rel->r_offset);
                    if (*ptr < base) {
                        *ptr += base;
                    }
                    break;
                }
            }
        }
    }
    else {
        svc_sys_write("Failed to find DT_REL\n");
    }

    is_relocated = 1;
}

uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr)
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[0].p_memsz / sizeof(Elf32_Phdr);

    for (int i = 2; i < num_phdrs; i++)
    {
        if (addr >= phdrs[i].p_vaddr && addr <= phdrs[i].p_vaddr + phdrs[i].p_memsz) {
            uintptr_t ret = addr - phdrs[i].p_vaddr + phdrs[i].p_paddr;
            debug_printf("translated vaddr %08x -> %08x\n", addr, ret);
            return ret;
        }
    }

    debug_printf("Failed to translate vaddr %08x!\n", addr);

    return addr;
}

// TODO linking/symbol lookup