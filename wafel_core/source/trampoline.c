#include "patch.h"
#include "ios/svc.h"
#include "dynamic.h"
#include "config.h"
#include "trampoline.h"

extern u8 trampoline_buffer[];
uintptr_t trampoline_next = (u32)trampoline_buffer;
extern u8 trampoline_buffer_end[];

static u32 align(u32 x, u32 to){
    return x + (to-1) - (x-1)%to;
}

/**
 * Branch instructions contain a signed 2’s complement 24 bit offset. This is shifted left
 * two bits, sign extended to 32 bits, and added to the PC. The instruction can therefore
 * pecify a branch of +/- 32Mbytes. The branch offset must take account of the prefetch
 * operation, which causes the PC to be 2 words (8 bytes) ahead of the current instruction
*/
u32 extract_bl_target(uintptr_t addr){
    uintptr_t paddr = ios_elf_vaddr_to_paddr(addr);
    u32 ins = *(u32*)paddr;
    if(ins>>24 != 0xEB){
        return 0;
    }
    u32 off = ins & 0xFFFFFF;
    off <<= 2;
    const u32 m = 1<<25;
    s32 soff = (off ^ m) - m; // sign extend magic
    u32 target = addr + soff + 8;
    debug_printf("%p: %08X -> %p\n", addr, ins, target);
    return target;
}

void* trampoline_copy(uintptr_t addr, void *trampoline, void *trampoline_end){
    trampoline_next = align(trampoline_next, 4);
    u32 trampoline_size = trampoline_end - trampoline;
    debug_printf("Installing trampoline from %p at %p size %u\n", addr, trampoline_next, trampoline_size);
    if(trampoline_next + trampoline_size >= (u32)trampoline_buffer_end){
        debug_printf("no trampoline space left\n");
        crash_and_burn();
    }
    memcpy16((void*) trampoline_next, trampoline, trampoline_size);

    void* ret = (void*)trampoline_next;
    trampoline_next += trampoline_size;
    return ret;
}

u32 trampoline_find_mapping(uintptr_t addr, void* trampoline_addr, u32 offset_bits){
    u32 trampoline_alt = (u32)trampoline_addr;

    if(trampoline_alt>>offset_bits == addr>>offset_bits)
        return trampoline_alt;
    
    trampoline_alt = MCP_ALTBASE_ADDR((u32)trampoline_addr);
    if(trampoline_alt>>offset_bits == addr>>offset_bits)
        return trampoline_alt;
    
    trampoline_alt = FS_ALTBASE_ADDR((u32)trampoline_addr);
    if(trampoline_alt>>offset_bits == addr>>offset_bits)
        return trampoline_alt;

    trampoline_alt = NET_ALTBASE_ADDR((u32)trampoline_addr);
    if(trampoline_alt>>offset_bits == addr>>offset_bits)
        return trampoline_alt;

    trampoline_alt = NSEC_ALTBASE_ADDR((u32)trampoline_addr);
    if(trampoline_alt>>offset_bits == addr>>offset_bits)
        return trampoline_alt;

    debug_printf("Trampoline at %p not reachable from %p\n", trampoline_next, addr);
    crash_and_burn();
    return 0;
}

void* trampoline_install(uintptr_t addr, void *trampoline, void *trampoline_end){
    void *installed_trampoline = trampoline_copy(addr, trampoline, trampoline_end);
    u32 trampoline_alt = trampoline_find_mapping(addr, installed_trampoline, 26);
    BL_TRAMPOLINE_K(addr, trampoline_alt);
    return installed_trampoline;
}

extern u8 trampoline_blre_proto[];
extern u8 trampoline_blre_proto_end[];
extern u32 trampoline_blre_proto_chain[];
extern void* trampoline_blre_proto_target[];

void trampoline_blreplace(uintptr_t addr, void *target){
    trampoline_blre_proto_chain[0] = extract_bl_target(addr);
    trampoline_blre_proto_target[0] = target;
    trampoline_install(addr, trampoline_blre_proto, trampoline_blre_proto_end);
}

extern u8 trampoline_blre_regs_proto[];
extern u8 trampoline_blre_regs_proto_end[];
extern u32 trampoline_blre_regs_proto_chain[];
extern void* trampoline_blre_regs_proto_target[];

void trampoline_blreplace_with_regs(uintptr_t addr, void *target){
    trampoline_blre_regs_proto_chain[0] = extract_bl_target(addr);
    trampoline_blre_regs_proto_target[0] = target;
    trampoline_install(addr, trampoline_blre_regs_proto, trampoline_blre_regs_proto_end);
}

extern u8 trampoline_hookbefore_proto[];
extern u8 trampoline_hookbefore_proto_end[];
extern void* trampoline_hookbefore_proto_target[];
extern u32 trampoline_hookbefore_proto_orgins[];
extern u32 trampoline_hookbefore_proto_chain[];

void trampoline_hook_before(uintptr_t addr, void (*target)(trampoline_state*)){
    u32 bl_target = extract_bl_target(addr);
    u32 orgins = *(u32*)ios_elf_vaddr_to_paddr(addr);
    debug_printf("Overwriting %p: %08X -> %p\n", addr, orgins, bl_target);
    trampoline_hookbefore_proto_target[0] = target;
    void* tramp_base = trampoline_install(addr, trampoline_hookbefore_proto, trampoline_hookbefore_proto_end);
    // can't do this on the proto as that would overwrite the other case
    if(bl_target){
        *(u32*)(tramp_base + ((void*)trampoline_hookbefore_proto_chain - (void*)trampoline_hookbefore_proto)) = bl_target;
    } else {
        *(u32*)(tramp_base + ((void*)trampoline_hookbefore_proto_orgins - (void*)trampoline_hookbefore_proto)) = orgins;
    }
}

void* trampoline_t_install(uintptr_t addr, void *trampoline, void *trampoline_end){
    void *installed_trampoline = trampoline_copy(addr, trampoline, trampoline_end);
    u32 trampoline_alt = trampoline_find_mapping(addr, installed_trampoline, 23);
    BL_T_TRAMPOLINE_K(addr, trampoline_alt);
    return installed_trampoline;
}

u32 extract_bl_t_target(uintptr_t addr){
    u16 *paddr = (u16*)ios_elf_vaddr_to_paddr(addr);
    u16 ins1 = paddr[0];
    u16 ins2 = paddr[1];
    if(ins1>>11!=0b11110 || ins2>>11!=0b11111){
        return 0;
    }
    u32 off1 = ins1 << (32-11);
    u32 off2 = (ins2&0x7FF) << (32-11-11);
    s32 soff = (off1 | off2);
    soff >>=9; //arithmetic shift sign extends
    u32 target = (addr + soff + 4) | 1;
    debug_printf("%p: %04X%04X -> %p\n", addr, ins1, ins2, target);

    return target;
}

extern u8 trampoline_t_blre_proto[];
extern u8 trampoline_t_blre_proto_end[];
extern u32 trampoline_t_blre_proto_chain[];
extern void* trampoline_t_blre_proto_target[];

void trampoline_t_blreplace(uintptr_t addr, void *target){
    trampoline_t_blre_proto_chain[0] = extract_bl_t_target(addr);
    trampoline_t_blre_proto_target[0] = target;
    trampoline_t_install(addr, trampoline_t_blre_proto, trampoline_t_blre_proto_end);
}

extern u8 trampoline_t_blre_regs_proto[];
extern u8 trampoline_t_blre_regs_proto_end[];
extern u32 trampoline_t_blre_regs_proto_chain[];
extern void* trampoline_t_blre_regs_proto_target[];

void trampoline_t_blreplace_with_regs(uintptr_t addr, void *target){
    trampoline_t_blre_regs_proto_chain[0] = extract_bl_t_target(addr);
    trampoline_t_blre_regs_proto_target[0] = target;
    trampoline_t_install(addr, trampoline_t_blre_regs_proto, trampoline_t_blre_regs_proto_end);
}

extern u8 trampoline_t_hookbefore_proto[];
extern u8 trampoline_t_hookbefore_proto_end[];
extern void* trampoline_t_hookbefore_proto_target[];
extern u16 trampoline_t_hookbefore_proto_orgins[];

static void trampoline_t_hook_before_ins(uintptr_t addr, void (*target)(trampoline_t_state*)){
    u16 *orgins = (u16*)ios_elf_vaddr_to_paddr(addr);
    trampoline_t_hookbefore_proto_target[0] = target;
    //might not be aligned
    trampoline_t_hookbefore_proto_orgins[0] = orgins[0];
    trampoline_t_hookbefore_proto_orgins[1] = orgins[1];
    trampoline_t_install(addr, trampoline_t_hookbefore_proto, trampoline_t_hookbefore_proto_end);
}

extern u8 trampoline_t_hookbefore_bl_proto[];
extern u8 trampoline_t_hookbefore_bl_proto_end[];
extern void* trampoline_t_hookbefore_bl_proto_target[];
extern u32 trampoline_t_hookbefore_bl_proto_chain[];


static void trampoline_t_hook_before_bl(uintptr_t addr, void (*target)(trampoline_t_state*), u32 chain){
    u16 *orgins = (u16*)ios_elf_vaddr_to_paddr(addr);
    trampoline_t_hookbefore_bl_proto_target[0] = target;
    trampoline_t_hookbefore_bl_proto_chain[0] = chain;
    trampoline_t_install(addr, trampoline_t_hookbefore_bl_proto, trampoline_t_hookbefore_bl_proto_end);
}


void trampoline_t_hook_before(uintptr_t addr, void (*target)(trampoline_t_state*)){
    u32 bl_target = extract_bl_t_target(addr);
    if(bl_target){
        debug_printf("found bl to %p\n", bl_target);
        trampoline_t_hook_before_bl(addr, target, bl_target);
    } else {
        trampoline_t_hook_before_ins(addr, target);
    }
}