#include "dynamic.h"

#include "types.h"
#include "imports.h"
#include "ios/svc.h"
#include "utils.h"

extern void debug_printf(const char *format, ...);

void __ios_dynamic(uintptr_t base, const Elf32_Dyn* dyn)
{
    const Elf32_Rela* rela = NULL;
    u64 relasz = 0;
    const Elf32_Rel* rel = NULL;
    u64 relsz = 0;

    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_RELA:
                rela = (const Elf32_Rela*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELASZ:
                relasz = dyn->d_un.d_val / sizeof(Elf32_Rela);
                break;

            case DT_REL:
                rel = (const Elf32_Rel*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELSZ:
                relsz = dyn->d_un.d_val / sizeof(Elf32_Rel);
                break;
        }
    }

    if (rela != NULL) {
        for (; relasz--; rela++)
        {
            switch (ELF32_R_TYPE(rela->r_info))
            {
                case R_ARM_RELATIVE:
                {
                    u32* ptr = (u32*)(base + rela->r_offset);
                    *ptr = base + rela->r_addend;
                    break;
                }
            }
        }
    }
    else {
        //svc_sys_write("Failed to find DT_RELA\n");
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
}

// TODO linking/symbol lookup