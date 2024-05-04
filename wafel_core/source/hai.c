#include "types.h"
#include "addrs_55x.h"
#include "patch.h"
#include "trampoline.h"


static int haidev = -1;

static void c2w_patch_mlc()
{
    static const char mlc_pattern[] = { 0x00, 0xa3, 0x60, 0xb7, 0x58, 0x98, 0x21, 0x00};
    void* ios_paddr = (void*)read32(HAI_IOS_PADDR_PTR);
    void* ios_end = ios_paddr + HAI_IOS_SIZE;

    for (void* a = ios_paddr; a < ios_end; a += 2)
    {
        if (!memcmp(a, mlc_pattern, sizeof(mlc_pattern))) {
            write16(a, 0x2300);
            debug_printf("MLC stuff ptr at: 0x%08x\n", a);
            break;
        }
    }

}

void hai_redirect_mlc2sd(void){
    static const char hai_mlc_str[] = "/dev/sdio/MLC01";
    void* ios_paddr = (void*)read32(HAI_IOS_PADDR_PTR);
    void* ios_end = ios_paddr + HAI_IOS_SIZE;

    for (void* a = ios_paddr; a < ios_end; a += 2)
    {
        if (!memcmp(a, hai_mlc_str, sizeof(hai_mlc_str))) {
            strcpy(a,"/dev/sdio/slot0");
            debug_printf("HAI MLC dev str at: %p\n", a);
            //break;
        }
    }
}


void hai_companion_add_offset(uint32_t *buffer, uint32_t offset){
    uint32_t number_extends = buffer[0];
    debug_printf("number extends: %u\n", number_extends);
    uint32_t address_unit_base = (buffer[1] >> 16);
    debug_printf("address_unit_base: %u\n", address_unit_base);
    uint32_t address_unit_base_lbas = address_unit_base - 9; // 2^9 = 512byte sector
    debug_printf("address_unit_base_lbas: %u\n", address_unit_base_lbas);
    debug_printf("redmlc_off_sectors: 0x%X\n", offset);
    uint32_t offset_in_address_units = offset >> address_unit_base_lbas;
    debug_printf("offset_in_address_units: %X\n", offset_in_address_units);
    for(size_t i = 2; i < number_extends*2 + 2; i+=2){
        debug_printf("buffer[%u]: %X", i, buffer[i]);
        buffer[i]+=offset_in_address_units;
        debug_printf(" => %X\n", buffer[i]);
        debug_printf("buffer[%u]: %X\n", i+1, buffer[i+1]);
    }
}

/**
 * @brief sets the device type for hai, by hooking FSA_GetFileBlockAddress, which is called to create the compangion file
 */
static void get_block_addr_haidev_patch(trampoline_state* s){
    debug_printf("FSA GET FILE BLOCK ADDRESS\n");
    debug_printf("devid: %d\n", s->r[0]);
    haidev = s->r[0];
}

void hai_apply_getdev_patch(void){
    trampoline_hook_before(0x10707b70, get_block_addr_haidev_patch);
}

int hai_getdev(void){
    return haidev;
}