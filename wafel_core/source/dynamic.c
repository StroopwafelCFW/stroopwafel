#include "dynamic.h"

#include "config.h"
#include "types.h"
#include "imports.h"
#include "ios/svc.h"
#include "utils.h"
#include "ios_dynamic.h"
#include <string.h>

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
                // TODO: more relocs?
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

static uintptr_t* elfs = NULL;
static uint32_t num_elfs = 0;

Elf32_Phdr* wafel_get_plugin_phdrs(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    if (read32((uintptr_t)hdr->e_ident) != IPX_ELF_MAGIC) return NULL;

    return (Elf32_Phdr*)(base + hdr->e_phoff);
}

u32 wafel_get_plugin_num_phdrs(uintptr_t base)
{
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)base;
    if (read32((uintptr_t)hdr->e_ident) != IPX_ELF_MAGIC) return 0;

    return hdr->e_phnum;
}

Elf32_Dyn* wafel_get_plugin_dynamic(uintptr_t base)
{
    Elf32_Phdr* paPhdrs = wafel_get_plugin_phdrs(base);
    u32 num_phdrs = wafel_get_plugin_num_phdrs(base);
    if (!paPhdrs || !num_phdrs) return NULL;

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
    if (!paPhdrs || !num_phdrs) return 0; // TODO
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
    if (!dyn) return;

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
