#include "types.h"
#include <wafel/trampoline.h>
#include <wafel/patch.h>

#define IOS_ERROR_MAX -5
#define IOS_ERROR_NOT_READY -10
#define IOS_ERROR_INVALID -4
#define IOS_ERROR_OK 0

static int writeEnabled = 0;
static int dirty = 0;

static u16 *seeprom_buffer;

extern int redseeprom_read_orig(int handle, u32 index, u16 *out_data);

int redseeprom_read_word(int handle, u32 index, u16 *out_data)
{
    //debug_printf("Read SEEPROM: handle=%d index=0x%x, outdata=%p\n", handle, index, out_data);
    index&=0xff;
    // don't redirect the drive key as it is specific for the drive on the wii u
    // the seeprom key is the same for all wiiu's it seems so nothing to re-encrypt here
    if(index >= 0x40 && index < 0x48) {
        debug_printf("Calling redseeprom_read_orig\n");
        return redseeprom_read_orig(handle, index, out_data);
    }

    *out_data = seeprom_buffer[index];

    //debug_printf("Read SEEPROM finished: %04X\n", *out_data);
    return IOS_ERROR_OK;
}

int redseeprom_write_word(int handle, int index, u16 data)
{
    //debug_printf("Write SEEPROM: handle=%d index=0x%x, data=0x%02X\n", handle, index, data);
    if(writeEnabled == 0){
        return IOS_ERROR_NOT_READY;
    }

    seeprom_buffer[index & 0xff] = data;
    dirty = 1;

    return IOS_ERROR_OK;
}

int redseeprom_write_control(int handle, int cmd)
{
    //debug_printf("SEEPROM Write Control: handle=%d cmd=%d\n", handle, cmd);
    switch(cmd){
        case 1: 
            writeEnabled = 0;
            return IOS_ERROR_OK;
        case 2:
            writeEnabled = 1;
            return IOS_ERROR_OK;
        case 3: // erase all -> skip that part...its actually never used but would be only a memset with 0xFF
        default:
            return IOS_ERROR_INVALID;
    }
}


void redseeprom_enable(void *buffer) {
    seeprom_buffer = buffer;

    ASM_PATCH_K(0xe600d08c, 
        "ldr pc, _seeprom_rd_hook\n"
        "_seeprom_rd_hook: .word redseeprom_read_word"
    );
    ASM_PATCH_K(0xe600d010, 
        "ldr pc, _seeprom_wr_hook\n"
        "_seeprom_wr_hook: .word redseeprom_write_word"
    );
    ASM_PATCH_K(0xe600cf5c, 
        "ldr pc, _seeprom_wc_hook\n"
        "_seeprom_wc_hook: .word redseeprom_write_control"
    );
}