#include "utils.h"

#include "ios/svc.h"
#include "imports.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "latte/latte.h"
#include "latte/serial.h"

void debug_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    static char buffer[0x100];

    vsnprintf(buffer, 0xFF, format, args);
    //svc_sys_write(buffer);
    serial_send_str(buffer);

    va_end(args);
}

void udelay(u32 d)
{
    // should be good to max .2% error
    u32 ticks = d * 19 / 10;

    if(ticks < 2)
        ticks = 2;

    u32 now = read32(LT_TIMER);

    u32 then = now + ticks;

    if(then < now) {
        while(read32(LT_TIMER) >= now);
        now = read32(LT_TIMER);
    }

    while(now < then) {
        now = read32(LT_TIMER);
    }
}