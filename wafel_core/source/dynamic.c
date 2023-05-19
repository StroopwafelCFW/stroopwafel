#include "dynamic.h"

#include "config.h"
#include "types.h"
#include "imports.h"
#include "ios/svc.h"
#include "utils.h"
#include <string.h>

extern void debug_printf(const char *format, ...);

extern int is_relocated;
u32 _wafel_is_relocated(uintptr_t base);
void _wafel_set_relocated(uintptr_t base);

u32 _wafel_is_relocated(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    uintptr_t base_entry = base + hdr->e_entry;

    u32 ret = *(u32*)(base_entry + 0x14);
    return ret;
}

void _wafel_set_relocated(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    uintptr_t base_entry = base + hdr->e_entry;

    *(u32*)(base_entry + 0x14) = 1;
}

void __wafel_dynamic(uintptr_t base, const Elf32_Dyn* dyn)
{
    const Elf32_Rel* rel = NULL;
    u64 relsz = 0;

    if (_wafel_is_relocated(base)) return;

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

    _wafel_set_relocated(base);
}

uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr)
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[0].p_memsz / sizeof(Elf32_Phdr);

    for (int i = 2; i < num_phdrs; i++)
    {
        if (addr >= phdrs[i].p_vaddr && addr <= phdrs[i].p_vaddr + phdrs[i].p_memsz) {
            uintptr_t ret = addr - phdrs[i].p_vaddr + phdrs[i].p_paddr;
            //debug_printf("translated vaddr %08x -> %08x\n", addr, ret);
            return ret;
        }
    }

    debug_printf("Failed to translate vaddr %08x!\n", addr);

    return addr;
}

Elf32_Phdr* ios_elf_add_phdr(uintptr_t addr)
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[0].p_memsz / sizeof(Elf32_Phdr);

    int i;
    for (i = 2; i < num_phdrs; i++)
    {
        if (addr < phdrs[i].p_vaddr) {
            break;
        }
    }

    memmove((void*)(phdrs[1].p_vaddr+sizeof(Elf32_Phdr)), (void*)phdrs[1].p_vaddr, phdrs[1].p_memsz);
    phdrs[1].p_vaddr += sizeof(Elf32_Phdr);
    phdrs[1].p_paddr += sizeof(Elf32_Phdr);

    memmove(&phdrs[i+1], &phdrs[i], (num_phdrs-i)*sizeof(Elf32_Phdr));
    phdrs[0].p_memsz += sizeof(Elf32_Phdr);
    phdrs[0].p_filesz += sizeof(Elf32_Phdr);
    phdrs[2].p_filesz += sizeof(Elf32_Phdr);

    debug_printf("Inserted %08x before %08x and after %08x!\n", addr, phdrs[i+1].p_vaddr, phdrs[i-1].p_vaddr);

    return &phdrs[i];
}

void ios_elf_print_map()
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[0].p_memsz / sizeof(Elf32_Phdr);

    debug_printf("  Type           Offset   VirtAddr   PhysAddr   FileSiz    MemSiz     Flg      Align\n");
    for (int i = 0; i < num_phdrs; i++)
    {
        const char* type_str = "UNK";
        if (phdrs[i].p_type == PT_LOAD) {
            type_str = "LOAD";
        }
        else if (phdrs[i].p_type == PT_PHDR) {
            type_str = "PHDR";
        }
        else if (phdrs[i].p_type == PT_NOTE) {
            type_str = "NOTE";
        }
        debug_printf("  %-14s 0x%06x 0x%08x 0x%08x 0x%08x 0x%08x 0x%06x 0x%x\n", type_str, phdrs[i].p_offset, phdrs[i].p_vaddr, phdrs[i].p_paddr, phdrs[i].p_filesz, phdrs[i].p_memsz, phdrs[i].p_flags, phdrs[i].p_align);
    }
}

uintptr_t* elfs = NULL;
uint32_t num_elfs = 0;

Elf32_Phdr* wafel_get_plugin_phdrs(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    return (Elf32_Phdr*)(base + hdr->e_phoff);
}

u32 wafel_get_plugin_num_phdrs(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    return hdr->e_phnum;
}

Elf32_Dyn* wafel_get_plugin_dynamic(uintptr_t base)
{
    Elf32_Phdr* paPhdrs = wafel_get_plugin_phdrs(base);
    u32 num_phdrs = wafel_get_plugin_num_phdrs(base);

    for (u32 i = 0; i < num_phdrs; i++)
    {
        if (paPhdrs[i].p_type == PT_DYNAMIC) {
            return (Elf32_Dyn*)(base + paPhdrs[i].p_vaddr);
        }
    }
    return NULL;
}

uintptr_t wafel_plugin_max_addr(uintptr_t base)
{
    Elf32_Phdr* paPhdrs = wafel_get_plugin_phdrs(base);
    u32 num_phdrs = wafel_get_plugin_num_phdrs(base);
    uintptr_t ret = 0;

    for (u32 i = 0; i < num_phdrs; i++)
    {
        uintptr_t end = base + paPhdrs[i].p_vaddr + paPhdrs[i].p_memsz;
        if (end > ret) {
            ret = end;
        }
    }
    return ALIGN_FORWARD(ret, 0x1000);
}

uintptr_t wafel_plugin_next(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    uintptr_t base_entry = base + hdr->e_entry;

    uintptr_t ret = *(u32*)(base_entry + 0x10);
    return ret;
}

uintptr_t wafel_get_symbol_addr(uintptr_t base, const char* name)
{
    const Elf32_Dyn* dyn = NULL;
    const Elf32_Rel* rel = NULL;
    const Elf32_Sym* symtab = NULL;
    const char* strtab = NULL;
    u32 relsz = 0;
    u32 numsyms = 0;
    
    dyn = wafel_get_plugin_dynamic(base);

    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_SYMTAB:
                symtab = (const Elf32_Sym*)(base + dyn->d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab = (const char*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELA:
                rel = (const Elf32_Rel*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELASZ:
                relsz = dyn->d_un.d_val / sizeof(Elf32_Rel);
                break;
        }
    }
    
    numsyms = ((void*)strtab - (void*)symtab) / sizeof(Elf32_Sym);
    
    for (int i = 0; i < numsyms; i++)
    {
        if (!strcmp(strtab + symtab[i].st_name, name) && symtab[i].st_value)
        {
            return (uintptr_t)base + symtab[i].st_value;
        }
    }

    return 0;
}

uintptr_t wafel_find_symbol(const char* name)
{
    if (!elfs) return 0;

    for (int i = 0; i < num_elfs; i++)
    {
        uintptr_t ptr = wafel_get_symbol_addr(elfs[i], name);
        if (ptr) return ptr;
    }
    
    return 0;
}

uintptr_t wafel_find_symbol_multiple(const char* name, u32 skip)
{
    if (!elfs) return 0;

    for (int i = 0; i < num_elfs; i++)
    {
        uintptr_t ptr = wafel_get_symbol_addr(elfs[i], name);
        if (ptr) {
            if (skip-- == 0) {
                return ptr;
            }
        }
    }
    
    return 0;
}

uintptr_t wafel_get_plugin_base_by_idx(u32 idx)
{
    if (!elfs) return 0;
    if (idx >= num_elfs) return 0;

    return elfs[idx];
}

void wafel_register_plugin(uintptr_t base)
{
    elfs = realloc(elfs, ++num_elfs * sizeof(void*));
    elfs[num_elfs-1] = base;
}

void wafel_link_plugin(uintptr_t base)
{
    const Elf32_Dyn* dyn = NULL;
    const Elf32_Rel* rel = NULL;
    const Elf32_Sym* symtab = NULL;
    const char* strtab = NULL;
    u32 relsz = 0;
    
    dyn = wafel_get_plugin_dynamic(base);

    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_SYMTAB:
                symtab = (const Elf32_Sym*)(base + dyn->d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab = (const char*)(base + dyn->d_un.d_ptr);
                break;
            case DT_REL:
            case DT_JMPREL:
                rel = (const Elf32_Rel*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELSZ:
                relsz += dyn->d_un.d_val / sizeof(Elf32_Rel);
                break;
            case DT_PLTRELSZ:
                relsz += dyn->d_un.d_val / sizeof(Elf32_Rel);
                break;
        }
    }

    if (rel == NULL)
    {
        debug_printf("wafel_core: plugin %08x has no DT_REL\n", base);
        return;
    }

    for (; relsz--; rel++)
    {
        if (ELF32_R_TYPE(rel->r_info) == R_ARM_RELATIVE) continue;

        uint32_t sym_idx = ELF32_R_SYM(rel->r_info);
        const char* name = strtab + symtab[sym_idx].st_name;

        uintptr_t sym_val = (uintptr_t)base + symtab[sym_idx].st_value;
        if (!symtab[sym_idx].st_value)
            sym_val = 0;

        if (!symtab[sym_idx].st_shndx && sym_idx)
            sym_val = wafel_find_symbol(name);

        uintptr_t sym_val_and_addend = sym_val;// + rel->r_addend;

        debug_printf("wafel_core: %x %08x->%08x %s\n", sym_idx, symtab[sym_idx].st_value /* + rel->r_addend*/, sym_val_and_addend, name);

        switch (ELF32_R_TYPE(rel->r_info))
        {
            case R_ARM_GLOB_DAT:
            case R_ARM_JUMP_SLOT:
            case R_ARM_ABS32:
            {
                uintptr_t* ptr = (uintptr_t*)(base + rel->r_offset);
                *ptr = sym_val_and_addend;
                break;
            }
        }
    }
}

// TODO linking/symbol lookup