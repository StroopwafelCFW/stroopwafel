#include "patch.h"
#include "ios/svc.h"
#include "dynamic.h"
#include "config.h"

extern u8 trampoline_buffer[];
u32 trampoline_next = (u32)trampoline_buffer;
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
void* extract_bl_target(uintptr_t addr){
    uintptr_t paddr = ios_elf_vaddr_to_paddr(addr);
    u32 ins = *(u32*)paddr;
    if(ins>>24 != 0xEB){
        return NULL;
    }
    u32 off = ins & 0xFFFFFF;
    off <<= 2;
    const u32 m = 1<<25;
    s32 soff = (off ^ m) - m; // sign extend magic
    void* target = (void*)(addr + soff + 8);
    debug_printf("%p: %08X -> %p\n", addr, ins, target);
    return target;
}

void install_trampoline(uintptr_t addr, void *target, void *trampoline, void *trampoline_end, void *trampoline_target, void *trampoline_chain){
    trampoline_next = align(trampoline_next, 4);
    u32 trampoline_size = trampoline_end - trampoline;
    debug_printf("Installing trampoline from %p to %p at %p size %u\n", addr, target, trampoline_next, trampoline_size);
    if(trampoline_next + trampoline_size >= (u32)trampoline_buffer_end){
        debug_printf("no trampoline space left\n");
        crash_and_burn();
    }
    memcpy16((void*) trampoline_next, trampoline, trampoline_size);
    if(trampoline_target)
        *(void**)(trampoline_next + trampoline_target - trampoline) = target;
    if(trampoline_chain)
        *(void**)(trampoline_next + trampoline_chain - trampoline) = extract_bl_target(addr);
    u32 trampoline_alt = trampoline_next;
    if(trampoline_alt>>26 != addr>>26){
        trampoline_alt = MCP_ALTBASE_ADDR(trampoline_next);
        if(trampoline_alt>>26 != addr>>26){
            trampoline_alt = FS_ALTBASE_ADDR(trampoline_next);
                if(trampoline_alt>>26 != addr>>26){
                    debug_printf("Trampoline at %p not reachable from %p\n", trampoline_next, addr);
                    crash_and_burn();
                }
        }
    }

    BL_TRAMPOLINE_K(addr, trampoline_alt);
    trampoline_next += trampoline_size;
}

extern u8 blre_trampoline_proto[];
extern u8 blre_trampoline_proto_chain[];
extern u8 blre_trampoline_proto_target[];
extern u8 blre_trampoline_proto_end[];

void blreplace_trampoline(uintptr_t addr, void *target){
    install_trampoline(addr, target, blre_trampoline_proto, blre_trampoline_proto_end, blre_trampoline_proto_target, blre_trampoline_proto_chain);
}