#ifndef WAFEL_IOS_DYNAMIC_H
#define WAFEL_IOS_DYNAMIC_H

#include <elf.h>
#include <stddef.h>
#include "dynamic.h"

#define IOS_SEG_PHDRS   (0)
#define IOS_SEG_NOTES   (1)
#define IOS_SEG_ELFINFO (2)

#define UID_INVALID (0xFFFFFFFF)

#define IOS_RWX (7)
#define IOS_PHDR_FLAGS(_perms, _uid) (_perms | (_uid << 20))

typedef enum
{
    IOS_KERNEL   = 0,
    IOS_MCP      = 1,
    IOS_BSP      = 2,
    IOS_CRYPTO   = 3,
    IOS_USB      = 4,
    IOS_FS       = 5,
    IOS_PAD      = 6,
    IOS_NET      = 7,
    IOS_ACP      = 8,
    IOS_NSEC     = 9,
    IOS_AUXIL    = 10,
    IOS_NIM_BOSS = 11,
    IOS_FPD      = 12,
    IOS_TEST     = 13,
    COS_KERNEL   = 14,
    COS_ROOT     = 15,
    COS_02       = 16,
    COS_03       = 17,
    COS_OVERLAY  = 18,
    COS_HBM      = 19,
    COS_ERROR    = 20,
    COS_MASTER   = 21,
} ios_uid_t;

#if 0
typedef enum
{
    AT_ENTRY = 0x9,
    AT_UID = 0xB,
    AT_PRIORITY = 0x7D,
    AT_STACK_SIZE = 0x7E,
    AT_STACK_ADDR = 0x7F,
    AT_MEM_PERM_MASK = 0x80
} ios_auxv_type_t;
#endif

typedef u32 ios_auxv_type_t;

typedef struct
{
    ios_auxv_type_t type;
    u32 val;
} ios_auxv_t;

typedef struct
{
    Elf32_Nhdr nhdr;
    ios_auxv_t entries[];
} ios_notes_t;

LINKABLE uintptr_t ios_elf_vaddr_to_paddr(uintptr_t addr);
LINKABLE Elf32_Phdr* ios_elf_add_phdr(uintptr_t addr);
LINKABLE ios_notes_t* ios_elf_get_notes();
LINKABLE uintptr_t ios_elf_get_module_entrypoint(ios_uid_t uid);
LINKABLE void ios_elf_print_map();

#endif // WAFEL_IOS_DYNAMIC_H