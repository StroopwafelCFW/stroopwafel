#include "ios_dynamic.h"

#include "config.h"
#include "types.h"
#include "imports.h"
#include "ios/svc.h"
#include "utils.h"
#include <string.h>

uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr)
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[IOS_SEG_PHDRS].p_memsz / sizeof(Elf32_Phdr);

    for (int i = IOS_SEG_ELFINFO; i < num_phdrs; i++)
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
    u32 num_phdrs = phdrs[IOS_SEG_PHDRS].p_memsz / sizeof(Elf32_Phdr);

    int i;
    for (i = IOS_SEG_ELFINFO; i < num_phdrs; i++)
    {
        if (addr < phdrs[i].p_vaddr) {
            break;
        }
    }

    memmove((void*)(phdrs[IOS_SEG_NOTES].p_vaddr+sizeof(Elf32_Phdr)), (void*)phdrs[IOS_SEG_NOTES].p_vaddr, phdrs[IOS_SEG_NOTES].p_memsz);
    phdrs[IOS_SEG_NOTES].p_vaddr += sizeof(Elf32_Phdr);
    phdrs[IOS_SEG_NOTES].p_paddr += sizeof(Elf32_Phdr);

    memmove(&phdrs[i+1], &phdrs[i], (num_phdrs-i)*sizeof(Elf32_Phdr));
    phdrs[IOS_SEG_PHDRS].p_memsz += sizeof(Elf32_Phdr);
    phdrs[IOS_SEG_PHDRS].p_filesz += sizeof(Elf32_Phdr);
    phdrs[IOS_SEG_ELFINFO].p_filesz += sizeof(Elf32_Phdr);

    debug_printf("Inserted %08x before %08x and after %08x!\n", addr, phdrs[i+1].p_vaddr, phdrs[i-1].p_vaddr);

    return &phdrs[i];
}

ios_notes_t* ios_elf_get_notes()
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    ios_notes_t* pNotes = (ios_notes_t*)phdrs[IOS_SEG_NOTES].p_vaddr;
    return pNotes;
}

uintptr_t ios_elf_get_module_entrypoint(ios_uid_t uid)
{
    ios_notes_t* pNotes = ios_elf_get_notes();
    u32 num_entries = pNotes->nhdr.n_descsz / sizeof(ios_auxv_t);

    u32 cur_uid = UID_INVALID;
    for (int i = 0; i < num_entries; i++)
    {
        if (pNotes->entries[i].type == AT_UID) {
            cur_uid = pNotes->entries[i].val;
        }
        if (pNotes->entries[i].type == AT_ENTRY && cur_uid == uid) {
            return pNotes->entries[i].val;
        }
    }
    return 0;
}

void ios_elf_print_map()
{
    Elf32_Phdr* phdrs = (Elf32_Phdr*)0x1D000000;
    u32 num_phdrs = phdrs[IOS_SEG_PHDRS].p_memsz / sizeof(Elf32_Phdr);

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
