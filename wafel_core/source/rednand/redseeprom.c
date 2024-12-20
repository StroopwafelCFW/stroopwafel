#include "types.h"
#include <wafel/trampoline.h>

#define IOS_ERROR_MAX -5
#define IOS_ERROR_NOT_READY -10
#define IOS_ERROR_INVALID -4
#define IOS_ERROR_OK 0

static int writeEnabled = 0;
static int dirty = 0;

static u16 *seeprom_buffer;

static void redseeprom_read_word(trampoline_state *regs)
{
    u32 index = regs->r[12]; // and r12,r1,#0xff
    u16 *outbuf = (u16*) regs->r[2];

    // don't redirect the drive key as it is specific for the drive on the wii u
    // the seeprom key is the same for all wiiu's it seems so nothing to re-encrypt here
    if(index >= 0x40 && index < 0x48)
        return; //just continue normal flow of the original function

    // directly return, without executing the rest of the function
    regs->lr = 0xe600d0a8; // ldmia  sp!,{r4,r5,r6,r7,r8,pc}

    *outbuf = seeprom_buffer[index];

    regs->r[0] = IOS_ERROR_OK;
}

static void redseeprom_write_word(trampoline_state *regs)
{
    u32 index = regs->r[1]; // and r1,r1,#0xff
    u16 data = (u16) regs->r[3];

    regs->lr = 0xe600d038;

    if(writeEnabled == 0){
        regs->r[0] = IOS_ERROR_NOT_READY;
        return;
    }

    seeprom_buffer[index] = data;
    dirty = 1;

    regs->r[0] = IOS_ERROR_OK;
    return;
}

void redseeprom_write_control(trampoline_state *regs)
{
    int type = regs->r[1];

    regs->lr = 0xe600cf70;
    regs->r[0] = 0;
    switch(type){
        case 1: 
            writeEnabled = 0;
            break;
        case 2:
            writeEnabled = 1;
            break;
        case 3: // erase all -> skip that part...its actually never used but would be only a memset with 0xFF
        default:
            regs->r[0] = IOS_ERROR_INVALID;
    }
}


void redseeprom_enable(void *buffer) {
    seeprom_buffer = buffer;
    trampoline_hook_before(0xe600d0b0, redseeprom_read_word);
    trampoline_hook_before(0xe600d040, redseeprom_write_word);
    trampoline_hook_before(0xe600cf78, redseeprom_write_control);
}